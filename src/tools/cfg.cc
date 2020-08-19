//
// Copyright 2019 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "fmt/ostream.h"

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/str_to_u32.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_module_utils.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/sections.h"

namespace wasp {
namespace tools {
namespace cfg {

using fmt::print;

using namespace ::wasp::binary;

struct Options {
  Features features;
  string_view function;
  string_view output_filename;
};

using BBID = u32;
constexpr BBID InvalidBBID = ~0;

struct Successor {
  std::string name;
  BBID bbid;
};

struct BasicBlock {
  bool empty() const { return code.empty(); }

  SpanU8 code;
  std::vector<Successor> successors;
};

struct Label {
  Opcode opcode;
  BBID parent;
  BBID br;
  BBID next;
};

struct Tool {
  explicit Tool(SpanU8 data, Options);

  int Run();
  void DoPrepass();
  optional<Index> GetFunctionIndex();
  optional<Code> GetCode(Index);
  void CalculateCFG(Code);
  void RemoveEmptyBasicBlocks();
  void WriteDotFile();

  void PushLabel(Opcode, BBID br, BBID next);
  Label PopLabel();
  BBID NewBasicBlock();
  BasicBlock& GetBasicBlock(BBID);
  void StartBasicBlock(BBID, const u8*);
  void EndBasicBlock(const u8*);
  void MarkUnreachable(const u8*);
  void AddSuccessor(BBID, const std::string& name = std::string{});
  void AddSuccessor(BBID, BBID, const std::string& name = std::string{});
  void Br(Index, const std::string& name = std::string{});

  BinaryErrors errors;
  Options options;
  LazyModule module;
  std::map<string_view, Index> name_to_function;
  Index imported_function_count = 0;
  std::vector<Label> labels;
  std::vector<BasicBlock> cfg;
  BBID start_bbid = InvalidBBID;
  BBID current_bbid = InvalidBBID;
};

int Main(span<string_view> args) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp cfg"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add('f', "--function", "<func>", "generate CFG for <func>",
           [&](string_view arg) { options.function = arg; })
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          print(std::cerr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    print(std::cerr, "No filename given.\n");
    parser.PrintHelpAndExit(1);
  }

  if (options.function.empty()) {
    print(std::cerr, "No function given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print(std::cerr, "Error reading file {}.\n", filename);
    return 1;
  }

  SpanU8 data{*optbuf};
  Tool tool{data, options};
  int result = tool.Run();
  tool.errors.PrintTo(std::cerr);
  return result;
}

Tool::Tool(SpanU8 data, Options options)
    : errors{data},
      options{options},
      module{ReadModule(data, options.features, errors)} {}

int Tool::Run() {
  DoPrepass();
  auto index_opt = GetFunctionIndex();
  if (!index_opt) {
    print(std::cerr, "Unknown function {}\n", options.function);
    return 1;
  }
  auto code_opt = GetCode(*index_opt);
  if (!code_opt) {
    print(std::cerr, "Invalid function index {}\n", *index_opt);
    return 1;
  }
  CalculateCFG(*code_opt);
  RemoveEmptyBasicBlocks();
  WriteDotFile();
  return 0;
}

void Tool::DoPrepass() {
  ForEachFunctionName(module, [this](const IndexNamePair& pair) {
    name_to_function.insert(std::make_pair(pair.second, pair.first));
  });
  imported_function_count = GetImportCount(module, ExternalKind::Function);
}

optional<Index> Tool::GetFunctionIndex() {
  // Search by name.
  auto iter = name_to_function.find(options.function);
  if (iter != name_to_function.end()) {
    return iter->second;
  }

  // Try to convert the string to an integer and search by index.
  return StrToU32(options.function);
}

optional<Code> Tool::GetCode(Index find_index) {
  for (auto section : module.sections) {
    if (section->is_known()) {
      auto known = section->known();
      if (known->id == SectionId::Code) {
        auto section = ReadCodeSection(known, module.context);
        for (auto code : enumerate(section.sequence, imported_function_count)) {
          if (code.index == find_index) {
            return code.value;
          }
        }
      }
    }
  }
  return nullopt;
}

void Tool::CalculateCFG(Code code) {
  const u8* ptr = code.body->data.data();
  PushLabel(Opcode::Return, InvalidBBID, InvalidBBID);
  start_bbid = NewBasicBlock();
  StartBasicBlock(start_bbid, ptr);

  const u8* prev_ptr = ptr;
  auto instrs = ReadExpression(code.body, module.context);
  for (auto it = instrs.begin(), end = instrs.end(); it != end;
       ++it, prev_ptr = ptr) {
    const auto& instr = *it;
    ptr = it.data().data();
    switch (instr->opcode) {
      case Opcode::Unreachable:
        MarkUnreachable(ptr);
        break;

      case Opcode::Block: {
        auto next = NewBasicBlock();
        PushLabel(instr->opcode, next, next);
        break;
      }

      case Opcode::Loop: {
        auto loop = NewBasicBlock();
        auto next = NewBasicBlock();
        AddSuccessor(loop);
        PushLabel(instr->opcode, loop, next);
        StartBasicBlock(loop, prev_ptr);
        break;
      }

      case Opcode::If: {
        auto true_ = NewBasicBlock();
        auto next = NewBasicBlock();
        AddSuccessor(true_, "T");
        PushLabel(instr->opcode, next, next);
        StartBasicBlock(true_, ptr);
        break;
      }

      case Opcode::Else: {
        auto top = PopLabel();
        AddSuccessor(top.next);
        auto false_ = NewBasicBlock();
        AddSuccessor(top.parent, false_, "F");
        PushLabel(instr->opcode, top.next, top.next);
        StartBasicBlock(false_, ptr);
        break;
      }

      case Opcode::End: {
        auto top = PopLabel();
        AddSuccessor(top.next);
        if (top.opcode == Opcode::If) {
          AddSuccessor(top.parent, top.next, "F");
        }
        StartBasicBlock(top.next, ptr);
        break;
      }

      case Opcode::Br:
        Br(instr->index_immediate());
        MarkUnreachable(ptr);
        break;

      case Opcode::BrIf: {
        Br(instr->index_immediate(), "T");
        auto next = NewBasicBlock();
        AddSuccessor(next, "F");
        StartBasicBlock(next, ptr);
        break;
      }

      case Opcode::BrTable: {
        const auto& immediate = instr->br_table_immediate();
        u32 value = 0;
        for (const auto& target : immediate->targets) {
          Br(target, format("{}", value++));
        }
        Br(immediate->default_target, "default");
        MarkUnreachable(ptr);
        break;
      }

      case Opcode::Return:
      case Opcode::ReturnCall:
      case Opcode::ReturnCallIndirect:
        MarkUnreachable(ptr);
        break;

      default:
        break;
    }
  }
}

void Tool::RemoveEmptyBasicBlocks() {
  std::map<BBID, BBID> empty_map;
  // Map each empty bb to its successor.
  for (const auto& bb: enumerate(cfg)) {
    if (bb.value.empty()) {
      BBID next = bb.value.successors.empty() ? InvalidBBID
                                              : bb.value.successors[0].bbid;
      empty_map.emplace(bb.index, next);
    }
  }

  for (auto& pair: empty_map) {
    // Follow the chain of empty bbs to the first non-empty one (or
    // InvalidBBID).
    BBID last = pair.second;
    auto it = empty_map.find(last);
    if (it != empty_map.end()) {
      while (it != empty_map.end()) {
        last = it->second;
        it = empty_map.find(last);
      }

      // Replace all successors in the chain with `last`.
      BBID next = pair.second;
      pair.second = last;
      it = empty_map.find(next);
      while (it != empty_map.end()) {
        next = it->second;
        it->second = last;
        it = empty_map.find(next);
      }
    }
  }

  // Update all non-empty bbs that have empty successors.
  for (auto& bb: cfg) {
    if (!bb.empty()) {
      for (auto& succ: bb.successors) {
        auto it = empty_map.find(succ.bbid);
        if (it != empty_map.end()) {
          succ.bbid = it->second;
        }
      }
    }
  }
}

bool IsExtraneousInstruction(const At<Instruction>& instr) {
  auto opcode = instr->opcode;
  return opcode == Opcode::Block || opcode == Opcode::Else ||
         opcode == Opcode::End || opcode == Opcode::Br;
}

void Tool::WriteDotFile() {
  const int kMaxSuccessors = 64;

  std::ofstream fstream;
  std::ostream* stream = &std::cout;
  if (!options.output_filename.empty()) {
    fstream = std::ofstream{std::string{options.output_filename}};
    if (fstream) {
      stream = &fstream;
    }
  }

  print(*stream, "strict digraph {{\n");

  // Write nodes.
  for (const auto& bb: enumerate(cfg)) {
    if (!bb.value.empty()) {
      auto colspan = std::max<int>(
          1, std::min<int>(bb.value.successors.size(), kMaxSuccessors));
      print(*stream,
            "  {} [shape=none;margin=0;label=<"
            "<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\"><TR>"
            "<TD BORDER=\"0\" ALIGN=\"LEFT\" COLSPAN=\"{}\">",
            bb.index, colspan);
      auto instrs = ReadExpression(bb.value.code, module.context);
      for (const auto& instr: instrs) {
        if (IsExtraneousInstruction(instr)) {
          continue;
        } else if (instr->opcode == Opcode::BrTable) {
          print(*stream, "{}...", instr->opcode);
        } else {
          print(*stream, "{}", *instr);
        }
        print(*stream, "<BR ALIGN=\"LEFT\"/>");
      }
      print(*stream, "</TD></TR>");
      // Add ports.
      if (bb.value.successors.size() > 1) {
        print(*stream, "<TR>");
        string_view sides = "T";
        for (const auto& succ: enumerate(bb.value.successors)) {
          if (succ.index < kMaxSuccessors) {
            assert(!succ.value.name.empty());
            print(*stream, "<TD PORT=\"{}\" SIDES=\"{}\">{}</TD>",
                  succ.value.name, sides, succ.value.name);
          } else {
            print(*stream, "<TD PORT=\"trunc\" SIDES=\"TL\">...</TD>");
            break;
          }
          sides = "TL";
        }
        print(*stream, "</TR>");
      }
      print(*stream, "</TABLE>>]\n");
    }
  }

  // Write edges.
  print(*stream, "  start -> {}\n", start_bbid);
  for (const auto& bb: enumerate(cfg)) {
    if (!bb.value.empty()) {
      for (const auto& succ : enumerate(bb.value.successors)) {
        if (succ.value.bbid == InvalidBBID) {
          print(*stream, "  {} -> end", bb.index);
        } else {
          print(*stream, "  {}", bb.index);
          if (!succ.value.name.empty()) {
            if (succ.index < kMaxSuccessors) {
              print(*stream, ":{}", succ.value.name);
            } else {
              print(*stream, ":trunc");
            }
          }
          print(*stream, " -> {}", succ.value.bbid);
          if (succ.index >= kMaxSuccessors && !succ.value.name.empty()) {
            print(*stream, " [headlabel=\"{}\"]", succ.value.name);
          }
          print(*stream, "\n");
        }
      }
    }
  }

  print(*stream, "}}\n");
  stream->flush();
}

void Tool::PushLabel(Opcode opcode, BBID br, BBID next) {
  labels.push_back({opcode, current_bbid, br, next});
}

Label Tool::PopLabel() {
  assert(!labels.empty());
  Label top = labels.back();
  labels.pop_back();
  return top;
}

BBID Tool::NewBasicBlock() {
  cfg.emplace_back();
  return static_cast<BBID>(cfg.size() - 1);
}

BasicBlock& Tool::GetBasicBlock(BBID bbid) {
  assert(bbid < cfg.size());
  return cfg[bbid];
}

void Tool::StartBasicBlock(BBID bbid, const u8* start) {
  if (current_bbid != InvalidBBID) {
    EndBasicBlock(start);
  }
  current_bbid = bbid;
  if (current_bbid != InvalidBBID) {
    GetBasicBlock(current_bbid).code = SpanU8{start, start};
  }
}

void Tool::EndBasicBlock(const u8* end) {
  auto& bb = GetBasicBlock(current_bbid);
  const u8* start = bb.code.data();
  bb.code = SpanU8{start, end};

  auto instrs = ReadExpression(bb.code, module.context);
  if (std::all_of(instrs.begin(), instrs.end(), IsExtraneousInstruction)) {
    bb.code = SpanU8{};
  }
}

void Tool::MarkUnreachable(const u8* ptr) {
  StartBasicBlock(NewBasicBlock(), ptr);
}

void Tool::AddSuccessor(BBID bbid, const std::string& name) {
  AddSuccessor(current_bbid, bbid, name);
}

void Tool::AddSuccessor(BBID from, BBID to, const std::string& name) {
  GetBasicBlock(from).successors.push_back({name, to});
}

void Tool::Br(Index index, const std::string& name) {
  if (index < labels.size()) {
    AddSuccessor(labels[labels.size() - index - 1].br, name);
  } else {
    print(std::cerr, "Invalid branch depth: {}\n", index);
  }
}

}  // namespace cfg
}  // namespace tools
}  // namespace wasp

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

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/str_to_u32.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_module_utils.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/sections.h"

namespace wasp::tools::callgraph {

using absl::Format;

using namespace ::wasp::binary;

enum class Mode {
  All,
  Calls,
  Callers,
};

struct Options {
  Features features;
  string_view output_filename;
  optional<string_view> function;
  optional<Index> function_index;
  Mode mode = Mode::All;
};

struct Tool {
  explicit Tool(SpanU8 data, Options);

  int Run();
  void DoPrepass();
  void GetFunctionIndex();
  void CalculateCallGraph();
  void WriteDotFile();

  optional<string_view> GetFunctionName(Index) const;

  BinaryErrors errors;
  Options options;
  LazyModule module;
  std::map<Index, string_view> function_names;
  std::map<string_view, Index> name_to_function;
  Index imported_function_count = 0;
  std::set<std::pair<Index, Index>> call_graph;
};

int Main(span<const string_view> args) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp callgraph"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add("--calls", "<func>", "find all functions called by func",
           [&](string_view arg) {
             options.function = arg;
             options.mode = Mode::Calls;
           })
      .Add("--callers", "<func>", "find all functions that call func",
           [&](string_view arg) {
             options.function = arg;
             options.mode = Mode::Callers;
           })
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          Format(&std::cerr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    Format(&std::cerr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    Format(&std::cerr, "Error reading file %s.\n", filename);
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
      module{ReadLazyModule(data, options.features, errors)} {}

int Tool::Run() {
  DoPrepass();
  GetFunctionIndex();
  if (options.mode != Mode::All && !options.function_index) {
    Format(&std::cerr, "Unknown function %s.\n", *options.function);
    return 1;
  }
  CalculateCallGraph();
  WriteDotFile();
  return 0;
}

void Tool::DoPrepass() {
  ForEachFunctionName(module, [this](const IndexNamePair& pair) {
    name_to_function.insert(std::make_pair(pair.second, pair.first));
  });

  CopyFunctionNames(module,
                    std::inserter(function_names, function_names.end()));
  imported_function_count = GetImportCount(module, ExternalKind::Function);
}

void Tool::GetFunctionIndex() {
  if (!options.function) {
    options.function_index = nullopt;
    return;
  }
  // Search by name.
  auto iter = name_to_function.find(*options.function);
  if (iter != name_to_function.end()) {
    options.function_index = iter->second;
    return;
  }

  // Try to convert the string to an integer and search by index.
  options.function_index = StrToU32(*options.function);
}

void Tool::CalculateCallGraph() {
  std::multimap<Index, Index> full_graph;

  for (auto section : module.sections) {
    if (section->is_known()) {
      auto known = section->known();
      if (known->id == SectionId::Code) {
        auto section = ReadCodeSection(known, module.context);
        for (auto code : enumerate(section.sequence, imported_function_count)) {
          for (const auto& instr :
               ReadExpression(code.value->body, module.context)) {
            if (instr->opcode == Opcode::Call) {
              assert(instr->has_index_immediate());
              auto callee_index = instr->index_immediate();
              if (options.mode == Mode::Callers) {
                full_graph.emplace(callee_index, code.index);
              } else {
                full_graph.emplace(code.index, callee_index);
              }
            }
          }
        }
      }
    }
  }

  // Calculate subgraph that only includes calls/callers.
  switch (options.mode) {
    case Mode::All:
      for (auto pair : full_graph) {
        call_graph.insert(pair);
      }
      break;

    case Mode::Calls:
    case Mode::Callers: {
      std::set<Index> seen;
      std::set<Index> next = {*options.function_index};
      while (!next.empty()) {
        std::set<Index> new_next;
        for (auto caller : next) {
          if (seen.count(caller)) {
            continue;
          }
          seen.insert(caller);

          auto range = full_graph.equal_range(caller);
          for (auto iter = range.first; iter != range.second; ++iter) {
            Index callee = iter->second;
            new_next.insert(callee);

            if (options.mode == Mode::Callers) {
              call_graph.insert(std::make_pair(callee, caller));
            } else {
              call_graph.insert(std::make_pair(caller, callee));
            }
          }
        }
        next = std::move(new_next);
      }
      break;
    }
  }
}

void Tool::WriteDotFile() {
  std::ofstream fstream;
  std::ostream* stream = &std::cout;
  if (!options.output_filename.empty()) {
    fstream = std::ofstream{std::string{options.output_filename}};
    if (fstream) {
      stream = &fstream;
    }
  }

  Format(stream, "strict digraph {\n");
  Format(stream, "  rankdir = LR;\n");

  // Write nodes.
  std::set<Index> functions;
  for (auto pair : call_graph) {
    functions.insert(pair.first);
    functions.insert(pair.second);
  }

  for (auto function : functions) {
    Format(stream, "  %d", function);
    auto name = GetFunctionName(function);
    if (name) {
      Format(stream, " [label = \"%s\"]", *name);
    } else {
      Format(stream, " [label = \"f%d\"]", function);
    }
    Format(stream, ";\n");
  }

  // Write edges.
  for (auto pair : call_graph) {
    Index caller = pair.first;
    Index callee = pair.second;
    Format(stream, "  %d -> %d;\n", caller, callee);
  }

  Format(stream, "}\n");
  stream->flush();
}

optional<string_view> Tool::GetFunctionName(Index index) const {
  auto it = function_names.find(index);
  if (it != function_names.end()) {
    return it->second;
  } else {
    return nullopt;
  }
}

}  // namespace wasp::tools::callgraph

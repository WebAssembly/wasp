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

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "src/tools/argparser.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_module_utils.h"
#include "wasp/binary/sections.h"
#include "wasp/binary/sections_name.h"

namespace wasp {
namespace tools {
namespace callgraph {

using namespace ::wasp::binary;

struct Options {
  Features features;
  string_view output_filename;
};

struct Tool {
  explicit Tool(SpanU8 data, Options);

  void Run();
  void DoPrepass();
  void CalculateCallGraph();
  void WriteDotFile();

  optional<string_view> GetFunctionName(Index) const;

  ErrorsNop errors;
  Options options;
  LazyModule module;
  std::map<Index, string_view> function_names;
  Index imported_function_count = 0;
  std::set<std::pair<Index, Index>> call_graph;
};

int Main(span<string_view> args) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp callgraph"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          print(stderr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    print(stderr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print(stderr, "Error reading file {}.\n", filename);
    return 1;
  }

  SpanU8 data{*optbuf};
  Tool tool{data, options};
  tool.Run();

  return 0;
}

Tool::Tool(SpanU8 data, Options options)
    : options{options}, module{ReadModule(data, options.features, errors)} {}

void Tool::Run() {
  DoPrepass();
  CalculateCallGraph();
  WriteDotFile();
}

void Tool::DoPrepass() {
  CopyFunctionNames(module,
                    std::inserter(function_names, function_names.end()));
  imported_function_count = GetImportCount(module, ExternalKind::Function);
}

void Tool::CalculateCallGraph() {
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      if (known.id == SectionId::Code) {
        auto section = ReadCodeSection(known, module.context);
        for (auto code : enumerate(section.sequence, imported_function_count)) {
          for (const auto& instr :
               ReadExpression(code.value.body, module.context)) {
            if (instr.opcode == Opcode::Call) {
              assert(instr.has_index_immediate());
              auto callee_index = instr.index_immediate();
              call_graph.insert(std::make_pair(code.index, callee_index));
            }
          }
        }
      }
    }
  }
}

void Tool::WriteDotFile() {
  std::ofstream fstream;
  std::ostream* stream = &std::cout;
  if (!options.output_filename.empty()) {
    fstream = std::ofstream{options.output_filename.to_string()};
    if (fstream) {
      stream = &fstream;
    }
  }

  print(*stream, "strict digraph {{\n");
  print(*stream, "  rankdir = LR;\n");

  // Write nodes.
  std::set<Index> functions;
  for (auto pair : call_graph) {
    functions.insert(pair.first);
    functions.insert(pair.second);
  }

  for (auto function : functions) {
    print(*stream, "  {}", function);
    auto name = GetFunctionName(function);
    if (name) {
      print(*stream, " [label = \"{}\"]", *name);
    } else {
      print(*stream, " [label = \"f{}\"]", function);
    }
    print(*stream, ";\n");
  }

  // Write edges.
  for (auto pair : call_graph) {
    Index caller = pair.first;
    Index callee = pair.second;
    print(*stream, "  {} -> {};\n", caller, callee);
  }

  print(*stream, "}}\n");
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

}  // namespace callgraph
}  // namespace tools
}  // namespace wasp

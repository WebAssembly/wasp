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
#include <map>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/lazy_code_section.h"
#include "wasp/binary/lazy_export_section.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_function_names_subsection.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_name_section.h"

namespace wasp {
namespace tools {
namespace callgraph {

using namespace ::wasp::binary;

using ErrorsType = ErrorsNop;

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

  void InsertFunctionName(Index, string_view name);
  optional<string_view> GetFunctionName(Index) const;

  ErrorsType errors;
  Options options;
  LazyModule<ErrorsType> module;
  std::map<Index, string_view> function_names;
  Index imported_function_count = 0;
  std::set<std::pair<Index, Index>> call_graph;
};

int Main(int argc, char** argv) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  for (int i = 0; i < argc; ++i) {
    string_view arg = argv[i];
    if (arg[0] == '-') {
      switch (arg[1]) {
        case 'o': options.output_filename = argv[++i]; break;
        case '-':
          if (arg == "--output") {
            options.output_filename = argv[++i];
          } else {
            print("Unknown long argument {}\n", arg);
          }
          break;
        default:
          print("Unknown short argument {}\n", arg[0]);
          break;
      }
    } else {
      if (filename.empty()) {
        filename = arg;
      } else {
        print("Filename already given\n");
      }
    }
  }

  if (filename.empty()) {
    print("No filenames given.\n");
    return 1;
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print("Error reading file {}.\n", filename);
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
  const Features& features = options.features;
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      switch (known.id) {
        case SectionId::Import:
          for (auto import :
               ReadImportSection(known, features, errors).sequence) {
            if (import.kind() == ExternalKind::Function) {
              InsertFunctionName(imported_function_count++, import.name);
            }
          }
          break;

        case SectionId::Export:
          for (auto export_ :
               ReadExportSection(known, features, errors).sequence) {
            if (export_.kind == ExternalKind::Function) {
              InsertFunctionName(export_.index, export_.name);
            }
          }
          break;

        default:
          break;
      }
    } else if (section.is_custom()) {
      auto custom = section.custom();
      if (custom.name == "name") {
        for (auto subsection : ReadNameSection(custom, features, errors)) {
          if (subsection.id == NameSubsectionId::FunctionNames) {
            for (auto name_assoc :
                 ReadFunctionNamesSubsection(subsection, features, errors)
                     .sequence) {
              InsertFunctionName(name_assoc.index, name_assoc.name);
            }
          }
        }
      }
    }
  }
}

void Tool::CalculateCallGraph() {
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      if (known.id == SectionId::Code) {
        auto section = ReadCodeSection(known, options.features, errors);
        Index caller_index = imported_function_count;
        for (auto code : section.sequence) {
          auto instrs = ReadExpression(code.body, options.features, errors);
          for (const auto& instr : instrs) {
            if (instr.opcode == Opcode::Call) {
              assert(instr.has_index_immediate());
              auto callee_index = instr.index_immediate();
              call_graph.insert(std::make_pair(caller_index, callee_index));
            }
          }
          ++caller_index;
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

void Tool::InsertFunctionName(Index index, string_view name) {
  function_names.insert(std::make_pair(index, name));
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

//
// Copyright 2020 WebAssembly Community Group participants
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

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/hash.h"
#include "wasp/base/hashmap.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/visitor.h"

namespace wasp {
namespace tools {
namespace stats {

using namespace ::wasp::binary;

using Count = u64;

struct Options {
  Features features;
};

struct Statistics {
  Count section_count = 0;

  Count type_count = 0;
  Count longest_function_type_param_count = 0;
  Count longest_function_type_result_count = 0;

  Count import_count = 0;
  std::string longest_import_module;
  std::string longest_import_name;

  Count imported_function_count = 0;
  Count imported_table_count = 0;
  Count imported_memory_count = 0;
  Count imported_global_count = 0;
  Count imported_event_count = 0;

  Count defined_function_count = 0;
  Count defined_table_count = 0;
  Count defined_memory_count = 0;
  Count defined_global_count = 0;
  Count defined_event_count = 0;

  Count export_count = 0;
  std::string longest_export_name;

  Count start_count = 0;

  Count element_segment_count = 0;
  Count element_count = 0;
  Count largest_element_segment_count = 0;

  Count data_segment_count = 0;
  Count data_byte_size = 0;
  Count largest_data_segment_byte_size = 0;

  Count code_count = 0;
  Count largest_code_byte_size = 0;
  Count largest_local_count = 0;

  Count instruction_count = 0;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);

  void Run();
  void Print();

  struct Visitor : visit::Visitor {
    explicit Visitor(Tool&);

    visit::Result OnSection(At<Section>);
    visit::Result BeginTypeSection(LazyTypeSection);
    visit::Result OnType(const At<TypeEntry>&);
    visit::Result BeginImportSection(LazyImportSection);
    visit::Result OnImport(const At<Import>&);
    visit::Result BeginFunctionSection(LazyFunctionSection);
    visit::Result BeginTableSection(LazyTableSection);
    visit::Result BeginMemorySection(LazyMemorySection);
    visit::Result BeginGlobalSection(LazyGlobalSection);
    visit::Result BeginEventSection(LazyEventSection);
    visit::Result BeginExportSection(LazyExportSection);
    visit::Result OnExport(const At<Export>&);
    visit::Result BeginStartSection(StartSection);
    visit::Result BeginElementSection(LazyElementSection);
    visit::Result OnElement(const At<ElementSegment>&);
    visit::Result BeginCodeSection(LazyCodeSection);
    visit::Result BeginCode(const At<Code>&);
    visit::Result OnInstruction(const At<Instruction>&);
    visit::Result BeginDataSection(LazyDataSection);
    visit::Result OnData(const At<DataSegment>&);

    Tool& tool;
  };

  std::string filename;
  Options options;
  SpanU8 data;
  BinaryErrors errors;
  LazyModule module;
  Statistics stats;
};

int Main(span<string_view> args) {
  std::vector<string_view> filenames;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp stats"};
  parser
      .Add("--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add("<filenames...>", "input wasm files",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    print(std::cerr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  for (auto filename : filenames) {
    auto optbuf = ReadFile(filename);
    if (!optbuf) {
      print(std::cerr, "Error reading file {}.\n", filename);
      continue;
    }

    SpanU8 data{*optbuf};
    Tool tool{filename, data, options};
    tool.Run();
    tool.errors.PrintTo(std::cerr);
  }

  return 0;
}

Tool::Tool(string_view filename, SpanU8 data, Options options)
    : filename{filename},
      options{options},
      data{data},
      errors{data},
      module{ReadModule(data, options.features, errors)} {}

void Tool::Run() {
  Visitor visitor{*this};
  visit::Visit(module, visitor);
  Print();
}

void Tool::Print() {
  print("section_count: {}\n", stats.section_count);
  print("type_count: {}\n", stats.type_count);
  print("longest_function_type_param_count: {}\n", stats.longest_function_type_param_count);
  print("longest_function_type_result_count: {}\n", stats.longest_function_type_result_count);
  print("import_count: {}\n", stats.import_count);
  print("longest_import_module: {}\n", stats.longest_import_module);
  print("longest_import_name: {}\n", stats.longest_import_name);
  print("imported_function_count: {}\n", stats.imported_function_count);
  print("imported_table_count: {}\n", stats.imported_table_count);
  print("imported_memory_count: {}\n", stats.imported_memory_count);
  print("imported_global_count: {}\n", stats.imported_global_count);
  print("imported_event_count: {}\n", stats.imported_event_count);
  print("defined_function_count: {}\n", stats.defined_function_count);
  print("defined_table_count: {}\n", stats.defined_table_count);
  print("defined_memory_count: {}\n", stats.defined_memory_count);
  print("defined_global_count: {}\n", stats.defined_global_count);
  print("defined_event_count: {}\n", stats.defined_event_count);
  print("export_count: {}\n", stats.export_count);
  print("longest_export_name: {}\n", stats.longest_export_name);
  print("start_count: {}\n", stats.start_count);
  print("element_segment_count: {}\n", stats.element_segment_count);
  print("element_count: {}\n", stats.element_count);
  print("largest_element_segment_count: {}\n", stats.largest_element_segment_count);
  print("data_segment_count: {}\n", stats.data_segment_count);
  print("data_byte_size: {}\n", stats.data_byte_size);
  print("largest_data_segment_byte_size: {}\n", stats.largest_data_segment_byte_size);
  print("code_count: {}\n", stats.code_count);
  print("largest_code_byte_size: {}\n", stats.largest_code_byte_size);
  print("largest_local_count: {}\n", stats.largest_local_count);
  print("instruction_count: {}\n", stats.instruction_count);
}

template <typename T>
void Max(Count& max_count, T count) {
  if (count > max_count) {
    max_count = count;
  }
}

void MaxString(std::string& max_string, string_view str) {
  if (str.size() > max_string.size()) {
    max_string = str;
  }
}

Tool::Visitor::Visitor(Tool& tool) : tool{tool} {}

visit::Result Tool::Visitor::OnSection(At<Section> section) {
  tool.stats.section_count++;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginTypeSection(LazyTypeSection section) {
  tool.stats.type_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnType(const At<TypeEntry>& type) {
  Max(tool.stats.longest_function_type_param_count, type->type->param_types.size());
  Max(tool.stats.longest_function_type_result_count, type->type->result_types.size());
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginImportSection(LazyImportSection section) {
  tool.stats.import_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnImport(const At<Import>& import) {
  MaxString(tool.stats.longest_import_module, import->module.value());
  MaxString(tool.stats.longest_import_name, import->name.value());

  switch (import->kind()) {
    case ExternalKind::Function:
      tool.stats.imported_function_count++;
      break;
    case ExternalKind::Table:
      tool.stats.imported_table_count++;
      break;
    case ExternalKind::Memory:
      tool.stats.imported_memory_count++;
      break;
    case ExternalKind::Global:
      tool.stats.imported_global_count++;
      break;
    case ExternalKind::Event:
      tool.stats.imported_event_count++;
      break;
  }
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginFunctionSection(LazyFunctionSection section) {
  tool.stats.defined_function_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginTableSection(LazyTableSection section) {
  tool.stats.defined_table_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginMemorySection(LazyMemorySection section) {
  tool.stats.defined_memory_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginGlobalSection(LazyGlobalSection section) {
  tool.stats.defined_global_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginEventSection(LazyEventSection section) {
  tool.stats.defined_event_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginExportSection(LazyExportSection section) {
  tool.stats.export_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnExport(const At<Export>& export_) {
  MaxString(tool.stats.longest_export_name, export_->name.value());
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginStartSection(StartSection section) {
  tool.stats.start_count = 1;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginElementSection(LazyElementSection section) {
  tool.stats.element_segment_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnElement(const At<ElementSegment>& element) {
  Count this_count = 0;
  if (element->has_indexes()) {
    this_count = element->indexes().list.size();
  } else if (element->has_expressions()) {
    this_count = element->expressions().list.size();
  }
  tool.stats.element_count += this_count;
  Max(tool.stats.largest_element_segment_count, this_count);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginCodeSection(LazyCodeSection section) {
  tool.stats.code_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginCode(const At<Code>& code) {
  Max(tool.stats.largest_code_byte_size, code.loc().size());

  Count this_local_count = 0;
  for (auto&& locals : code->locals) {
    this_local_count += locals->count.value();
  }
  Max(tool.stats.largest_local_count, this_local_count);

  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnInstruction(const At<Instruction>&) {
  tool.stats.instruction_count++;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginDataSection(LazyDataSection section) {
  tool.stats.data_segment_count = section.count.value_or(0);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnData(const At<DataSegment>& segment) {
  Count this_size = segment->init.size();
  tool.stats.data_byte_size += this_size;
  Max(tool.stats.largest_data_segment_byte_size, this_size);
  return visit::Result::Ok;
}

}  // namespace stats
}  // namespace tools
}  // namespace wasp

//
// Copyright 2018 WebAssembly Community Group participants
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
#include <cctype>
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
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/base/str_to_u32.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/linking_section/formatters.h"
#include "wasp/binary/linking_section/sections.h"
#include "wasp/binary/name_section/formatters.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/sections.h"
#include "wasp/binary/visitor.h"

namespace wasp {
namespace tools {
namespace dump {

using fmt::format;
using fmt::print;

using namespace ::wasp::binary;

enum class Pass {
  Headers,
  Details,
  Disassemble,
  RawData,
};

struct Options {
  Features features;
  bool print_headers = false;
  bool print_details = false;
  bool print_disassembly = false;
  bool print_raw_data = false;
  string_view section_name;
  optional<string_view> function;
  optional<u32> func_index;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);

  using SectionIndex = u32;

  void Run();
  void DoPrepass();
  void DoPass(Pass);
  bool SectionMatches(Section) const;
  void DoSectionHeader(Pass, Section);
  void DoCustomSection(Pass, SectionIndex, CustomSection);

  struct Visitor : visit::Visitor {
    explicit Visitor(Tool&, Pass);

    visit::Result OnSection(At<Section>);
    visit::Result BeginTypeSection(LazyTypeSection);
    visit::Result OnType(const At<DefinedType>&);
    visit::Result BeginImportSection(LazyImportSection);
    visit::Result OnImport(const At<Import>&);
    visit::Result BeginFunctionSection(LazyFunctionSection);
    visit::Result OnFunction(const At<Function>&);
    visit::Result BeginTableSection(LazyTableSection);
    visit::Result OnTable(const At<Table>&);
    visit::Result BeginMemorySection(LazyMemorySection);
    visit::Result OnMemory(const At<Memory>&);
    visit::Result BeginGlobalSection(LazyGlobalSection);
    visit::Result OnGlobal(const At<Global>&);
    visit::Result BeginEventSection(LazyEventSection);
    visit::Result OnEvent(const At<Event>&);
    visit::Result BeginExportSection(LazyExportSection);
    visit::Result OnExport(const At<Export>&);
    visit::Result BeginStartSection(StartSection);
    visit::Result BeginElementSection(LazyElementSection);
    visit::Result OnElement(const At<ElementSegment>&);
    visit::Result BeginDataCountSection(DataCountSection);
    visit::Result BeginCodeSection(LazyCodeSection);
    visit::Result BeginCode(const At<Code>&);
    visit::Result BeginDataSection(LazyDataSection);
    visit::Result OnData(const At<DataSegment>&);

    visit::Result SkipUnless(bool);

    Tool& tool;
    Pass pass;
    SectionIndex section_index = 0;
    Index index = 0;
    Index function_count = 0;
    Index table_count = 0;
    Index memory_count = 0;
    Index global_count = 0;
    Index event_count = 0;
  };

  void DoNameSection(Pass, SectionIndex, LazyNameSection);
  void DoLinkingSection(Pass, SectionIndex, LinkingSection);
  void DoRelocationSection(Pass, SectionIndex, RelocationSection);

  void DoCount(Pass, optional<Index> count);

  void Disassemble(SectionIndex, Index func_index, Code);

  void InsertFunctionName(Index, string_view name);
  void InsertGlobalName(Index, string_view name);
  optional<DefinedType> GetDefinedType(Index) const;
  optional<Function> GetFunction(Index) const;
  optional<FunctionType> GetFunctionType(Index) const;
  optional<string_view> GetFunctionName(Index) const;
  optional<string_view> GetGlobalName(Index) const;
  optional<string_view> GetSectionName(Index) const;
  optional<string_view> GetSymbolName(Index) const;
  optional<Index> GetI32Value(const ConstantExpression&);

  using RelocationEntries = std::vector<RelocationEntry>;
  optional<RelocationEntries> GetRelocationEntries(SectionIndex);

  enum class PrintChars { No, Yes };

  bool ShouldPrintDetails(Pass) const;
  template <typename... Args>
  void PrintDetails(Pass, const char* format, const Args&...);
  void PrintFunctionName(Index func_index);
  void PrintGlobalName(Index global_index);
  void PrintMemory(SpanU8 data,
                   Index offset,
                   PrintChars print_chars = PrintChars::Yes,
                   string_view prefix = "",
                   int octets_per_line = 16,
                   int octets_per_group = 2);
  void PrintFunctionHeader(Index func_index, Code);
  void PrintInstruction(const Instruction&,
                        SpanU8 data,
                        SpanU8 post_data,
                        int indent);
  void PrintRelocation(const RelocationEntry& entry, size_t file_offset);

  size_t file_offset(SpanU8 data);

  struct Symbol {
    SymbolInfoKind kind;
    std::string name;
    Index index;
  };

  static constexpr int max_octets_per_line = 9;

  std::string filename;
  Options options;
  SpanU8 data;
  BinaryErrors errors;
  LazyModule module;
  std::vector<DefinedType> defined_types;
  std::vector<Function> functions;
  std::map<Index, string_view> function_names;
  std::map<Index, string_view> global_names;
  std::map<Index, Symbol> symbol_table;
  std::map<SectionIndex, std::string> section_names;
  std::map<SectionIndex, size_t> section_starts;
  std::map<SectionIndex, RelocationEntries> section_relocations;
  bool should_print_details = true;
  Index imported_function_count = 0;
  Index imported_table_count = 0;
  Index imported_memory_count = 0;
  Index imported_global_count = 0;
  Index imported_event_count = 0;
};

// static
constexpr int Tool::max_octets_per_line;

int Main(span<string_view> args) {
  std::vector<string_view> filenames;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp dump"};
  parser
      .Add("--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('h', "--headers", "print section headers",
           [&]() { options.print_headers = true; })
      .Add('d', "--disassemble", "print disassembly",
           [&]() { options.print_disassembly = true; })
      .Add('x', "--details", "print section details",
           [&]() { options.print_details = true; })
      .Add('s', "--full-contents", "print raw contents of the section",
           [&]() { options.print_raw_data = true; })
      .Add('j', "--section", "<section>",
           "print only the contents of <section>",
           [&](string_view arg) { options.section_name = arg; })
      .Add('f', "--function", "<func>", "only print information for <func>",
           [&](string_view arg) { options.function = arg; })
      .Add("<filenames...>", "input wasm files",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    print(std::cerr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  if (!(options.print_headers || options.print_disassembly ||
        options.print_details || options.print_raw_data)) {
    print(std::cerr, "At least one of the following switches must be given:\n");
    print(std::cerr, " -d/--disassemble\n");
    print(std::cerr, " -h/--headers\n");
    print(std::cerr, " -x/--details\n");
    print(std::cerr, " -s/--full-contents\n");
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
    : filename(filename),
      options{options},
      data{data},
      errors{data},
      module{ReadModule(data, options.features, errors)} {}

void Tool::Run() {
  if (!(module.magic && module.version)) {
    return;
  }

  print("\n{}:\tfile format wasm {}\n", filename,
        FormatWrapper{*module.version});
  DoPrepass();
  // If we haven't found a function with the given name, try interpreting it as
  // an index.
  if (options.function && !options.func_index) {
    options.func_index = StrToU32(*options.function);
    if (!options.func_index) {
      print(std::cerr, "unknown function {}\n", *options.function);
      return;
    }
  }
  if (options.print_headers) {
    DoPass(Pass::Headers);
  }
  if (options.print_details) {
    DoPass(Pass::Details);
  }
  if (options.print_disassembly) {
    DoPass(Pass::Disassemble);
  }
  if (options.print_raw_data) {
    DoPass(Pass::RawData);
  }
}

void Tool::DoPrepass() {
  for (auto section : enumerate(module.sections)) {
    section_starts[section.index] = file_offset(section.value->data());
    if (section.value->is_known()) {
      auto known = section.value->known();
      section_names[section.index] = format("{}", known->id);
      switch (known->id) {
        case SectionId::Type: {
          auto seq = ReadTypeSection(known, module.context).sequence;
          std::copy(seq.begin(), seq.end(), std::back_inserter(defined_types));
          break;
        }

        case SectionId::Import: {
          for (auto import :
               ReadImportSection(known, module.context).sequence) {
            switch (import->kind()) {
              case ExternalKind::Function:
                functions.push_back(Function{import->index()});
                InsertFunctionName(imported_function_count++, import->name);
                break;

              case ExternalKind::Table:
                imported_table_count++;
                break;

              case ExternalKind::Memory:
                imported_memory_count++;
                break;

              case ExternalKind::Global:
                InsertGlobalName(imported_global_count++, import->name);
                break;

              case ExternalKind::Event:
                imported_event_count++;
                break;

              default:
                break;
            }
          }
          break;
        }

        case SectionId::Function: {
          auto seq = ReadFunctionSection(known, module.context).sequence;
          std::copy(seq.begin(), seq.end(), std::back_inserter(functions));
          break;
        }

        case SectionId::Export: {
          for (auto export_ :
               ReadExportSection(known, module.context).sequence) {
            switch (export_->kind) {
              case ExternalKind::Function:
                InsertFunctionName(export_->index, export_->name);
                break;

              case ExternalKind::Global:
                InsertGlobalName(export_->index, export_->name);
                break;

              default:
                break;
            }
          }
        }

        default:
          break;
      }
    } else if (section.value->is_custom()) {
      auto custom = section.value->custom();
      section_names[section.index] = custom->name;
      if (*custom->name == "name") {
        for (auto subsection : ReadNameSection(custom, module.context)) {
          if (subsection->id == NameSubsectionId::FunctionNames) {
            for (auto name_assoc :
                 ReadFunctionNamesSubsection(subsection, module.context)
                     .sequence) {
              InsertFunctionName(name_assoc->index, name_assoc->name);
            }
          }
        }
      } else if (*custom->name == "linking") {
        for (auto subsection :
             ReadLinkingSection(custom, module.context).subsections) {
          if (subsection->id == LinkingSubsectionId::SymbolTable) {
            for (auto symbol_pair :
                 enumerate(ReadSymbolTableSubsection(subsection, module.context)
                               .sequence)) {
              auto symbol_index = symbol_pair.index;
              auto symbol = symbol_pair.value;
              auto kind = symbol->kind();
              auto name_opt = symbol->name();
              auto name = std::string{name_opt.value_or("")};
              if (symbol->is_base()) {
                auto item_index = symbol->base().index;
                if (name_opt) {
                  if (kind == SymbolInfoKind::Function) {
                    InsertFunctionName(item_index, *name_opt);
                  } else if (kind == SymbolInfoKind::Global) {
                    InsertGlobalName(item_index, *name_opt);
                  }
                }
                symbol_table[symbol_index] = Symbol{kind, name, item_index};
              } else if (symbol->is_data()) {
                symbol_table[symbol_index] = Symbol{kind, name, 0};
              } else if (symbol->is_section()) {
                symbol_table[symbol_index] =
                    Symbol{kind, name, symbol->section().section};
              }
            }
          }
        }
      } else if (starts_with(*custom->name, "reloc.")) {
        auto sec = ReadRelocationSection(custom, module.context);
        if (sec.section_index) {
          section_relocations[*sec.section_index] =
              RelocationEntries{sec.entries.begin(), sec.entries.end()};
        }
      }
    }
  }
}

void Tool::DoPass(Pass pass) {
  switch (pass) {
    case Pass::Headers:
      print("\nSections:\n\n");
      break;

    case Pass::Details:
      print("\nSection Details:\n\n");
      break;

    case Pass::Disassemble:
      print("\nCode Disassembly:\n\n");
      break;

    case Pass::RawData:
      break;
  }

  Visitor visitor{*this, pass};
  visit::Visit(module, visitor);
}

Tool::Visitor::Visitor(Tool& tool, Pass pass) : tool{tool}, pass{pass} {}

visit::Result Tool::Visitor::OnSection(At<Section> section) {
  auto this_idx = section_index++;
  if (tool.SectionMatches(section)) {
    tool.DoSectionHeader(pass, section);
    if (section->is_custom()) {
      tool.DoCustomSection(pass, this_idx, section->custom());
    }
    return visit::Result::Ok;
  } else {
    return visit::Result::Skip;
  }
}

namespace {

template <typename S1, typename S2>
bool StringsAreEqualCaseInsensitive(const S1& s1, const S2& s2) {
  auto it1 = s1.begin(), end1 = s1.end();
  auto it2 = s2.begin(), end2 = s2.end();
  for (; it1 != end1 && it2 != end2; ++it1, ++it2) {
    if (std::tolower(*it1) != std::tolower(*it2)) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool Tool::SectionMatches(Section section) const {
  if (options.section_name.empty()) {
    return true;
  }

  std::string name;
  if (section.is_known()) {
    name = format("{}", section.known()->id);
  } else if (section.is_custom()) {
    name = section.custom()->name;
  }
  return StringsAreEqualCaseInsensitive(name, options.section_name);
}

void Tool::DoCustomSection(Pass pass,
                           SectionIndex section_index,
                           CustomSection custom) {
  switch (pass) {
    case Pass::Headers:
      print("\"{}\"\n", custom.name);
      break;

    case Pass::Details:
      print(":\n - name: \"{}\"\n", custom.name);
      if (*custom.name == "name") {
        DoNameSection(pass, section_index,
                      ReadNameSection(custom, module.context));
      } else if (*custom.name == "linking") {
        DoLinkingSection(pass, section_index,
                         ReadLinkingSection(custom, module.context));
      } else if (starts_with(*custom.name, "reloc.")) {
        DoRelocationSection(pass, section_index,
                            ReadRelocationSection(custom, module.context));
      }
      break;

    default:
      break;
  }
}

void Tool::DoSectionHeader(Pass pass,
                           Section section) {
  auto id = section.is_known() ? section.known()->id : At{SectionId::Custom};
  auto data = section.data();
  auto offset = data.begin() - module.data.begin();
  auto size = data.size();
  switch (pass) {
    case Pass::Headers: {
      print("{:>9} start={:#010x} end={:#010x} (size={:#010x}) ", id, offset,
            offset + size, size);
      break;
    }

    case Pass::Details:
      print("{}", id);
      break;

    case Pass::Disassemble:
      break;

    case Pass::RawData: {
      if (section.is_custom()) {
        print("\nContents of custom section ({}):\n", section.custom()->name);
      } else {
        print("\nContents of section {}:\n", id);
      }
      PrintMemory(data, static_cast<Index>(offset), PrintChars::Yes);
      break;
    }
  }
}

visit::Result Tool::Visitor::BeginTypeSection(LazyTypeSection section) {
  index = 0;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnType(const At<DefinedType>& defined_type) {
  print(" - type[{}] {}\n", index++, defined_type);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginImportSection(LazyImportSection section) {
  function_count = 0;
  table_count = 0;
  memory_count = 0;
  global_count = 0;
  event_count = 0;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnImport(const At<Import>& import) {
  switch (import->kind()) {
    case ExternalKind::Function: {
      print(" - func[{}] sig={}", function_count, import->index());
      tool.PrintFunctionName(function_count);
      ++function_count;
      break;
    }

    case ExternalKind::Table: {
      print(" - table[{}] {}", table_count, import->table_type());
      ++table_count;
      break;
    }

    case ExternalKind::Memory: {
      print(" - memory[{}] {}", memory_count, import->memory_type());
      ++memory_count;
      break;
    }

    case ExternalKind::Global: {
      print(" - global[{}] {}", global_count, import->global_type());
      ++global_count;
      break;
    }

    case ExternalKind::Event: {
      print(" - event[{}] {}", event_count, import->event_type());
      ++event_count;
      break;
    }
  }
  print(" <- {}.{}\n", import->module, import->name);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginFunctionSection(LazyFunctionSection section) {
  index = tool.imported_function_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnFunction(const At<Function>& func) {
  if (!tool.options.func_index || index == tool.options.func_index) {
    print(" - func[{}] sig={}", index, func->type_index);
    tool.PrintFunctionName(index);
    print("\n");
  }
  ++index;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginTableSection(LazyTableSection section) {
  index = tool.imported_table_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnTable(const At<Table>& table) {
  print(" - table[{}] {}\n", index++, table->table_type);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginMemorySection(LazyMemorySection section) {
  index = tool.imported_memory_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnMemory(const At<Memory>& memory) {
  print(" - memory[{}] {}\n", index++, memory->memory_type);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginGlobalSection(LazyGlobalSection section) {
  index = tool.imported_global_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnGlobal(const At<Global>& global) {
  print(" - global[{}] {} - {}\n", index++, global->global_type, global->init);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginEventSection(LazyEventSection section) {
  index = tool.imported_event_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnEvent(const At<Event>& event) {
  print(" - event[{}] {}\n", index++, event->event_type);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginExportSection(LazyExportSection section) {
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnExport(const At<Export>& export_) {
  print(" - {}[{}]", export_->kind, export_->index);
  if (export_->kind == ExternalKind::Function) {
    tool.PrintFunctionName(export_->index);
  }
  print(" -> \"{}\"\n", export_->name);
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginStartSection(StartSection section) {
  if (section) {
    auto start = *section;
    if (pass == Pass::Headers) {
      print("start: {}\n", start->func_index);
    } else {
      tool.PrintDetails(pass, " - start function: {}\n", start->func_index);
    }
  }
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginElementSection(LazyElementSection section) {
  index = 0;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnElement(const At<ElementSegment>& segment) {
  print(" - segment[{}] {}", index, segment->type);
  if (segment->table_index) {
    print(" table={}", *segment->table_index);
  }

  if (segment->has_indexes()) {
    print(" kind={} count={}", segment->indexes().kind,
          segment->indexes().list.size());
  } else if (segment->has_expressions()) {
    print(" elemtype={} count={}", segment->expressions().elemtype,
          segment->expressions().list.size());
  }

  Index offset = 0;
  if (segment->offset) {
    offset = tool.GetI32Value(*segment->offset).value_or(0);
    print(" - init {}", offset);
  }
  print("\n");

  if (segment->has_indexes()) {
    for (auto item : enumerate(segment->indexes().list)) {
      print("  - elem[{}] = {}\n", offset + item.index, item.value);
    }
  } else if (segment->has_expressions()) {
    for (auto item : enumerate(segment->expressions().list)) {
      print("  - elem[{}] = {}\n", offset + item.index, item.value);
    }
  }

  ++index;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginDataCountSection(DataCountSection section) {
  if (section) {
    auto data_count = *section;
    if (pass == Pass::Headers) {
      print("count: {}\n", data_count->count);
    } else {
      tool.PrintDetails(pass, " - data count: {}\n", data_count->count);
    }
  }
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginCodeSection(LazyCodeSection section) {
  index = tool.imported_function_count;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass) || pass == Pass::Disassemble);
}

visit::Result Tool::Visitor::BeginCode(const At<Code>& code) {
  if (!tool.options.func_index || index == tool.options.func_index) {
    if (pass == Pass::Details) {
      print(" - func[{}] size={}\n", index, code->body->data.size());
    } else {
      tool.Disassemble(section_index, index, code);
    }
  }
  ++index;
  // Skip iterating over instructions.
  return visit::Result::Skip;
}

visit::Result Tool::Visitor::BeginDataSection(LazyDataSection section) {
  index = 0;
  tool.DoCount(pass, section.count);
  return SkipUnless(tool.ShouldPrintDetails(pass));
}

visit::Result Tool::Visitor::OnData(const At<DataSegment>& segment) {
  print(" - segment[{}] {}", index, segment->type);
  if (segment->memory_index) {
    print(" memory={}", *segment->memory_index);
  }
  print(" size={}", segment->init.size());
  Index offset = 0;
  if (segment->offset) {
    offset = tool.GetI32Value(*segment->offset).value_or(0);
    print(" - init {}", offset);
  }
  print("\n");
  tool.PrintMemory(segment->init, offset, PrintChars::Yes, "  - ");
  ++index;
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::SkipUnless(bool b) {
  return b ? visit::Result::Ok : visit::Result::Skip;
}

void Tool::DoNameSection(Pass pass,
                         SectionIndex section_index,
                         LazyNameSection section) {
  for (auto subsection : section) {
    switch (subsection->id) {
      case NameSubsectionId::ModuleName: {
        auto module_name =
            ReadModuleNameSubsection(subsection->data, module.context);
        print("  module name: {}\n", module_name.value_or(""));
        break;
      }

      case NameSubsectionId::FunctionNames: {
        auto function_names_subsection =
            ReadFunctionNamesSubsection(subsection->data, module.context);
        print("  function names[{}]:\n",
              function_names_subsection.count.value_or(0));
        for (auto name_assoc : enumerate(function_names_subsection.sequence)) {
          print("   - [{}]: func[{}] name=\"{}\"\n", name_assoc.index,
                name_assoc.value->index, name_assoc.value->name);
        }
        break;
      }

      case NameSubsectionId::LocalNames: {
        auto local_names_subsection =
            ReadLocalNamesSubsection(subsection->data, module.context);
        print("  local names[{}]:\n", local_names_subsection.count.value_or(0));
        for (auto indirect_name_assoc :
             enumerate(local_names_subsection.sequence)) {
          print("   - [{}]: func[{}] count={}\n", indirect_name_assoc.index,
                indirect_name_assoc.value->index,
                indirect_name_assoc.value->name_map.size());
          for (auto name_assoc :
               enumerate(indirect_name_assoc.value->name_map)) {
            print("     - [{}]: local[{}] name=\"{}\"\n", name_assoc.index,
                  name_assoc.value->index, name_assoc.value->name);
          }
        }
        break;
      }
    }
  }
}

void Tool::DoLinkingSection(Pass pass,
                            SectionIndex section_index,
                            LinkingSection section) {
  for (auto subsection : section.subsections) {
    switch (subsection->id) {
      case LinkingSubsectionId::SegmentInfo: {
        if (ShouldPrintDetails(pass)) {
          auto segment_infos =
              ReadSegmentInfoSubsection(subsection->data, module.context);
          print(" - segment info [count={}]\n",
                segment_infos.count.value_or(0));
          for (auto segment_info : enumerate(segment_infos.sequence)) {
            print("  - {}: {} p2align={} flags={:#x}\n", segment_info.index,
                  segment_info.value->name, segment_info.value->align_log2,
                  segment_info.value->flags);
          }
        }
        break;
      }

      case LinkingSubsectionId::InitFunctions: {
        if (ShouldPrintDetails(pass)) {
          auto init_functions =
              ReadInitFunctionsSubsection(subsection->data, module.context);
          print(" - init functions [count={}]\n",
                init_functions.count.value_or(0));
          for (auto init_function : init_functions.sequence) {
            print("  - {}: priority={}\n", init_function->index,
                  init_function->priority);
          }
        }
        break;
      }

      case LinkingSubsectionId::ComdatInfo: {
        if (ShouldPrintDetails(pass)) {
          auto comdats = ReadComdatSubsection(subsection->data, module.context);
          print(" - comdat [count={}]\n", comdats.count.value_or(0));
          for (auto comdat : enumerate(comdats.sequence)) {
            print("  - {}: \"{}\" flags={:#x} [count={}]\n", comdat.index,
                  comdat.value->name, comdat.value->flags,
                  comdat.value->symbols.size());
            for (auto symbol : enumerate(comdat.value->symbols)) {
              print("   - {}: {} index={}\n", symbol.index, symbol.value->kind,
                    symbol.value->index);
            }
          }
        }
        break;
      }

      case LinkingSubsectionId::SymbolTable: {
        if (ShouldPrintDetails(pass)) {
          auto print_symbol_flags = [&](SymbolInfo::Flags flags) {
            if (flags.undefined == SymbolInfo::Flags::Undefined::Yes) {
              print(" {}", flags.undefined);
            }
            print(" binding={} vis={}", flags.binding, flags.visibility);
            if (flags.explicit_name == SymbolInfo::Flags::ExplicitName::Yes) {
              print(" {}", flags.explicit_name);
            }
          };

          auto symbol_table =
              ReadSymbolTableSubsection(subsection->data, module.context);
          print(" - symbol table [count={}]\n",
                symbol_table.count.value_or(0));
          for (auto symbol : enumerate(symbol_table.sequence)) {
            switch (symbol.value->kind()) {
              case SymbolInfoKind::Function: {
                const auto& base = symbol.value->base();
                print("  - {}: F <{}> func={}", symbol.index,
                      base.name.value_or(
                          GetFunctionName(base.index).value_or("")),
                      base.index);
                print_symbol_flags(symbol.value->flags);
                break;
              }

              case SymbolInfoKind::Global: {
                const auto& base = symbol.value->base();
                print(
                    "  - {}: G <{}> global={}", symbol.index,
                    base.name.value_or(GetGlobalName(base.index).value_or("")),
                    base.index);
                print_symbol_flags(symbol.value->flags);
                break;
              }

              case SymbolInfoKind::Event: {
                const auto& base = symbol.value->base();
                // TODO GetEventName.
                print("  - {}: E <{}> event={}", symbol.index,
                      base.name.value_or(""_sv), base.index);
                print_symbol_flags(symbol.value->flags);
                break;
              }

              case SymbolInfoKind::Data: {
                const auto& data = symbol.value->data();
                print("  - {}: D <{}>", symbol.index, data.name);
                if (data.defined) {
                  print(" segment={} offset={} size={}", data.defined->index,
                        data.defined->offset, data.defined->size);
                }
                print_symbol_flags(symbol.value->flags);
                break;
              }

              case SymbolInfoKind::Section: {
                auto section_index = symbol.value->section().section;
                print("  - {}: S <{}> section={}", symbol.index,
                      GetSectionName(section_index).value_or(""),
                      section_index);
                print_symbol_flags(symbol.value->flags);
                break;
              }
            }
            print("\n");
          }
        }
        break;
      }
    }
  }
}

void Tool::DoRelocationSection(Pass pass,
                               SectionIndex section_index,
                               RelocationSection section) {
  auto reloc_section_index = section.section_index.value_or(-1);
  PrintDetails(pass, " - relocations for section {} ({}) [{}]\n",
               reloc_section_index,
               GetSectionName(reloc_section_index).value_or(""),
               section.count.value_or(0));
  for (auto entry : section.entries) {
    size_t total_offset = entry->offset;
    auto start = section_starts.find(*section.section_index);
    if (start != section_starts.end()) {
      total_offset += start->second;
    }
    if (ShouldPrintDetails(pass)) {
      print("   - {:18s} offset={:#08x}(file={:#08x}) ", entry->type,
            entry->offset, total_offset);
      if (entry->type == RelocationType::TypeIndexLEB) {
        print("type={}", entry->index);
      } else {
        print("symbol={} <{}>", entry->index,
              GetSymbolName(entry->index).value_or(""));
      }
      if (entry->addend && *entry->addend != 0) {
        print("{:+#x}", *entry->addend);
      }
      print("\n");
    }
  }
}

void Tool::DoCount(Pass pass, optional<Index> count) {
  if (pass == Pass::Headers) {
    print("count: {}\n", count.value_or(0));
  } else {
    PrintDetails(pass, "[{}]:\n", count.value_or(0));
  }
}

void Tool::Disassemble(SectionIndex section_index,
                       Index func_index,
                       Code code) {
  PrintFunctionHeader(func_index, code);
  int indent = 0;
  auto section_start = section_starts[section_index];
  auto section_offset = [&](SpanU8 data) {
    return file_offset(data) - section_start;
  };
  auto last_data = code.body->data;
  auto relocs =
      GetRelocationEntries(section_index).value_or(RelocationEntries{});
  auto reloc_it =
      std::lower_bound(relocs.begin(), relocs.end(), section_offset(last_data),
                       [&](const RelocationEntry& lhs, size_t offset) {
                         return lhs.offset < offset;
                       });
  auto instrs = ReadExpression(*code.body, module.context);
  for (auto it = instrs.begin(), end = instrs.end(); it != end; ++it) {
    const auto& instr = *it;
    auto opcode = instr->opcode;
    if (opcode == Opcode::Else || opcode == Opcode::Catch ||
        opcode == Opcode::End) {
      indent = std::max(indent - 2, 0);
    }
    PrintInstruction(instr, last_data, it.data(), indent);
    last_data = it.data();
    for (; reloc_it < relocs.end() &&
           reloc_it->offset < section_offset(it.data());
         ++reloc_it) {
      PrintRelocation(*reloc_it, section_start + reloc_it->offset);
    }
    if (opcode == Opcode::Block || opcode == Opcode::If ||
        opcode == Opcode::Loop || opcode == Opcode::Else ||
        opcode == Opcode::Catch || opcode == Opcode::Try) {
      indent += 2;
    }
  }
}

void Tool::InsertFunctionName(Index index, string_view name) {
  function_names.insert(std::make_pair(index, name));
  if (options.function == name) {
    options.func_index = index;
  }
}

void Tool::InsertGlobalName(Index index, string_view name) {
  global_names.insert(std::make_pair(index, name));
}

optional<DefinedType> Tool::GetDefinedType(Index type_index) const {
  if (type_index >= defined_types.size()) {
    return nullopt;
  }
  return defined_types[type_index];
}

optional<Function> Tool::GetFunction(Index func_index) const {
  if (func_index >= functions.size()) {
    return nullopt;
  }
  return functions[func_index];
}

optional<FunctionType> Tool::GetFunctionType(Index func_index) const {
  if (auto func = GetFunction(func_index)) {
    if (auto defined_type = GetDefinedType(func->type_index)) {
      if (defined_type->is_function_type()) {
        return defined_type->function_type();
      }
    }
  }
  return nullopt;
}

optional<string_view> Tool::GetFunctionName(Index index) const {
  auto it = function_names.find(index);
  if (it != function_names.end()) {
    return it->second;
  } else {
    return nullopt;
  }
}

optional<string_view> Tool::GetGlobalName(Index index) const {
  auto it = global_names.find(index);
  if (it != global_names.end()) {
    return it->second;
  } else {
    return nullopt;
  }
}

optional<string_view> Tool::GetSectionName(Index index) const {
  auto it = section_names.find(index);
  if (it != section_names.end()) {
    return it->second;
  } else {
    return nullopt;
  }
}

optional<string_view> Tool::GetSymbolName(Index index) const {
  auto it = symbol_table.find(index);
  if (it != symbol_table.end()) {
    const auto& symbol = it->second;
    switch (symbol.kind) {
      case SymbolInfoKind::Function:
        return GetFunctionName(symbol.index);

      case SymbolInfoKind::Data:
        return symbol.name;

      case SymbolInfoKind::Global:
        return GetGlobalName(symbol.index);

      case SymbolInfoKind::Section:
        return GetSectionName(symbol.index);

      case SymbolInfoKind::Event:
        return "";  // XXX
    }
  }
  return nullopt;
}

optional<Index> Tool::GetI32Value(const ConstantExpression& expr) {
  if (expr.instructions.size() != 1) {
    return nullopt;
  }
  if (expr.instructions[0]->opcode != Opcode::I32Const) {
    return nullopt;
  }
  return expr.instructions[0]->s32_immediate();
}

optional<Tool::RelocationEntries> Tool::GetRelocationEntries(
    SectionIndex section_index) {
  auto it = section_relocations.find(section_index);
  if (it != section_relocations.end()) {
    return it->second;
  } else {
    return nullopt;
  }
}

bool Tool::ShouldPrintDetails(Pass pass) const {
  return pass == Pass::Details && should_print_details;
}

template <typename... Args>
void Tool::PrintDetails(Pass pass, const char* format, const Args&... args) {
  if (ShouldPrintDetails(pass)) {
    vprint(format, fmt::make_format_args(args...));
  }
}

void Tool::PrintFunctionName(Index func_index) {
  if (auto name = GetFunctionName(func_index)) {
    print(" <{}>", *name);
  }
}

void Tool::PrintGlobalName(Index func_index) {
  if (auto name = GetGlobalName(func_index)) {
    print(" <{}>", *name);
  }
}

void Tool::PrintMemory(SpanU8 start,
                       Index offset,
                       PrintChars print_chars,
                       string_view prefix,
                       int octets_per_line,
                       int octets_per_group) {
  SpanU8 data = start;
  while (!data.empty()) {
    auto line_size = std::min<size_t>(data.size(), octets_per_line);
    const SpanU8 line = data.subspan(0, line_size);
    print("{}", prefix);
    print("{:07x}: ", (line.begin() - start.begin()) + offset);
    for (int i = 0; i < octets_per_line;) {
      for (int j = 0; j < octets_per_group; ++j, ++i) {
        if (i < static_cast<int>(line_size)) {
          print("{:02x}", line[i]);
        } else {
          print("  ");
        }
      }
      print(" ");
    }

    if (print_chars == PrintChars::Yes) {
      print(" ");
      for (int c : line) {
        print("{:c}", isprint(c) ? static_cast<char>(c) : '.');
      }
    }
    print("\n");
    remove_prefix(&data, line_size);
  }
}

void Tool::PrintFunctionHeader(Index func_index, Code code) {
  auto func_type = GetFunctionType(func_index);
  size_t param_count = 0;
  print("func[{}]", func_index);
  PrintFunctionName(func_index);
  print(":");
  if (func_type) {
    print(" {}\n", *func_type);
    param_count = func_type->param_types.size();
  } else {
    print("\n");
  }
  size_t local_count = param_count;
  for (auto locals : code.locals) {
    print(" {:{}s} | locals[{}", "", 7 + max_octets_per_line * 3, local_count);
    if (locals->count != 1) {
      print("..{}", local_count + locals->count - 1);
    }
    print("] type={}\n", locals->type);
    local_count += locals->count;
  }
}

void Tool::PrintInstruction(const Instruction& instr,
                            SpanU8 data,
                            SpanU8 post_data,
                            int indent) {
  bool first_line = true;
  while (data.begin() < post_data.begin()) {
    print(" {:06x}:", file_offset(data));
    int line_octets =
        std::min<int>(max_octets_per_line, static_cast<int>(post_data.begin() - data.begin()));
    for (int i = 0; i < line_octets; ++i) {
      print(" {:02x}", data[i]);
    }
    remove_prefix(&data, line_octets);
    print("{:{}s} |", "", (max_octets_per_line - line_octets) * 3);
    if (first_line) {
      first_line = false;
      print(" {:{}s}{}", "", indent, instr);

      if (instr.opcode == Opcode::Call) {
        PrintFunctionName(instr.index_immediate());
      } else if (instr.opcode == Opcode::GlobalGet ||
                 instr.opcode == Opcode::GlobalSet) {
        PrintGlobalName(instr.index_immediate());
      } else if (instr.has_block_type_immediate()) {
        auto block_type = instr.block_type_immediate();
        if (block_type->is_index()) {
          auto defined_type_opt = GetDefinedType(block_type->index());
          if (defined_type_opt) {
            print(" <{}>", defined_type_opt->type);
          }
        }
      }
    }
    print("\n");
  }
}

void Tool::PrintRelocation(const RelocationEntry& entry, size_t file_offset) {
  print("           {:06x}: {:18s} {}", file_offset, entry.type, entry.index);
  if (entry.addend && *entry.addend) {
    print(" {:+d}", *entry.addend);
  }
  if (entry.type != RelocationType::TypeIndexLEB) {
    print(" <{}>", GetSymbolName(entry.index).value_or(""));
  }
  print("\n");
}

size_t Tool::file_offset(SpanU8 data) {
  return data.begin() - module.data.begin();
}

}  // namespace dump
}  // namespace tools
}  // namespace wasp

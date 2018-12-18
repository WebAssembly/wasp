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

#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>

#include "src/view/view_main.h"

#include "imgui.h"

#include "wasp/base/file.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/external_kind.h"
#include "wasp/binary/function.h"
#include "wasp/binary/global.h"
#include "wasp/binary/lazy_code_section.h"
#include "wasp/binary/lazy_data_section.h"
#include "wasp/binary/lazy_element_section.h"
#include "wasp/binary/lazy_export_section.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_global_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_memory_section.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_table_section.h"
#include "wasp/binary/lazy_type_section.h"
#include "wasp/binary/memory.h"
#include "wasp/binary/start_section.h"
#include "wasp/binary/table.h"

namespace wasp {
namespace view {

using namespace ::wasp::binary;

static std::string s_filename;
static std::vector<u8> s_buffer;

// TODO This should be a named value, external kind count.
static Index s_import_count[4];
static std::map<Index, Index> s_instr_count;

static Features s_features;
static std::vector<Section> s_sections;
static std::vector<Code> s_codes;
static Index s_code_selected;

template <typename Sequence>
typename Sequence::const_iterator FindSection(const Sequence&, SectionId);

void SectionWindow(KnownSection known);

template <typename T>
void DumpSection(T section, string_view name);
template <>
void DumpSection<LazyCodeSection<ErrorsNop>>(LazyCodeSection<ErrorsNop>,
                                             string_view name);

void CodeWindow(Index declared_index);
void CodeWindow(const std::string& title, Index declared_index);

void ViewInit(int argc, char** argv) {
  argc--;
  argv++;
  if (argc == 0) {
    fprintf(stderr, "No files.\n");
    exit(1);
  }

  s_filename = argv[0];
  auto optbuf = ReadFile(s_filename);
  if (!optbuf) {
    fprintf(stderr, "Error reading file.\n");
    exit(1);
  }

  s_buffer = std::move(*optbuf);
  SpanU8 data{s_buffer};

  ErrorsNop errors;
  auto module = ReadModule(data, s_features, errors);
  std::copy(module.sections.begin(), module.sections.end(),
            std::back_inserter(s_sections));

  auto code_section = FindSection(s_sections, SectionId::Code);
  if (code_section != s_sections.end()) {
    const auto& sec =
        ReadCodeSection(code_section->known().data, s_features, errors);
    std::copy(sec.sequence.begin(), sec.sequence.end(),
              std::back_inserter(s_codes));
  }

  auto import_section = FindSection(s_sections, SectionId::Import);
  if (import_section != s_sections.end()) {
    const auto& sec =
        ReadImportSection(import_section->known().data, s_features, errors);
    for (auto import : sec.sequence) {
      s_import_count[static_cast<size_t>(import.kind())]++;
    }
  }
}

void ViewMain() {
  for (auto section : s_sections) {
    if (section.is_known()) {
      SectionWindow(section.known());
    }
  }

  CodeWindow("Code preview", s_code_selected);
}

void SectionWindow(KnownSection known) {
  ErrorsNop errors;
  if (ImGui::Begin(format("{}", known.id).c_str())) {
    switch (known.id) {
      case SectionId::Custom:
        WASP_UNREACHABLE();
        break;

      case SectionId::Type:
        DumpSection(ReadTypeSection(known, s_features, errors), "Type");
        break;

      case SectionId::Import:
        DumpSection(ReadImportSection(known, s_features, errors), "Import");
        break;

      case SectionId::Function:
        DumpSection(ReadFunctionSection(known, s_features, errors), "Func");
        break;

      case SectionId::Table:
        DumpSection(ReadTableSection(known, s_features, errors), "Table");
        break;

      case SectionId::Memory:
        DumpSection(ReadMemorySection(known, s_features, errors), "Memory");
        break;

      case SectionId::Global:
        DumpSection(ReadGlobalSection(known, s_features, errors), "Global");
        break;

      case SectionId::Export:
        DumpSection(ReadExportSection(known, s_features, errors), "Export");
        break;

      case SectionId::Start: {
        auto section = ReadStartSection(known, s_features, errors);
        size_t count = section ? 1 : 0;
        print("  Start[{}]\n", count);
        if (count > 0) {
          print("    [0]: {}\n", *section);
        }
        break;
      }

      case SectionId::Element:
        DumpSection(ReadElementSection(known, s_features, errors), "Element");
        break;

      case SectionId::Code:
        DumpSection(ReadCodeSection(known, s_features, errors), "Code");
        break;

      case SectionId::Data:
        DumpSection(ReadDataSection(known, s_features, errors), "Data");
        break;
    }
  }
  ImGui::End();
}

Index ImportCount(ExternalKind kind) {
  return s_import_count[static_cast<size_t>(kind)];
}

template <typename T>
Index InitialCount() { return 0; }

template <>
Index InitialCount<Function>() {
  return ImportCount(ExternalKind::Function);
}

template <>
Index InitialCount<Table>() {
  return ImportCount(ExternalKind::Table);
}

template <>
Index InitialCount<Memory>() {
  return ImportCount(ExternalKind::Memory);
}

template <>
Index InitialCount<Global>() {
  return ImportCount(ExternalKind::Global);
}

template <typename Sequence>
typename Sequence::const_iterator FindSection(const Sequence& seq,
                                              SectionId id) {
  return std::find_if(s_sections.begin(), s_sections.end(),
                      [&](const Section& sec) {
                        return sec.is_known() && sec.known().id == id;
                      });
}

template <typename T>
void DumpSection(T section, string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("{}[{}]\n", name, *section.count).c_str());
    Index initial_count = InitialCount<typename T::sequence_type::value_type>();
    ImGuiListClipper clipper(*section.count);
    while (clipper.Step()) {
      auto it = section.sequence.begin(), end = section.sequence.end();
      std::advance(it, clipper.DisplayStart);
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && it != end;
           ++it, ++i) {
        ImGui::Text("%s",
                    format("  [{}]: {}\n", initial_count + i, *it).c_str());
      }
    }
  }
}

template <>
void DumpSection<LazyCodeSection<ErrorsNop>>(LazyCodeSection<ErrorsNop> section,
                                             string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("{}[{}]\n", name, *section.count).c_str());
    Index initial_count = ImportCount(ExternalKind::Function);
    ImGuiListClipper clipper(*section.count);
    while (clipper.Step()) {
      auto it = section.sequence.begin(), end = section.sequence.end();
      std::advance(it, clipper.DisplayStart);
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && it != end;
           ++it, ++i) {
        Index declared_index = i;
        Index code_index = initial_count + i;
        auto code = *it;

        ImGui::PushID(i);
        if (ImGui::Selectable(
                format("  [{}]: {} bytes", code_index, code.body.data.size())
                    .c_str(),
                declared_index == s_code_selected)) {
          s_code_selected = declared_index;
        }
        ImGui::PopID();

      }
    }
  }
}

template <typename Errors>
Index GetInstrCount(Index code_index, const LazyExpression<Errors>& expr) {
  auto iter = s_instr_count.find(code_index);
  if (iter == s_instr_count.end()) {
    auto count = std::distance(expr.begin(), expr.end());
    s_instr_count[code_index] = count;
    return count;
  } else {
    return iter->second;
  }
}

void CodeWindow(Index declared_index) {
  Index code_index = InitialCount<Function>() + declared_index;
  CodeWindow(format("Code {}", code_index), declared_index);
}

void CodeWindow(const std::string& title, Index declared_index) {
  bool is_open = true;
  if (ImGui::Begin(title.c_str(), &is_open)) {
    ErrorsNop errors;
    auto expr =
        ReadExpression(s_codes[declared_index].body.data, s_features, errors);
    Index instr_count = GetInstrCount(declared_index, expr);

    ImGuiListClipper clipper(instr_count);
    while (clipper.Step()) {
      auto it = expr.begin(), end = expr.end();
      int indent = 0;
      for (int i = 0; i < clipper.DisplayEnd && it != end; ++it, ++i) {
        auto instr = *it;
        if (instr.opcode == Opcode::End || instr.opcode == Opcode::Else) {
          indent = std::max(0, indent - 2);
        }

        if (i >= clipper.DisplayStart) {
          ImGui::Text("%*.s%s", indent, "", format("{}\n", instr).c_str());
        }

        if (instr.opcode == Opcode::Block || instr.opcode == Opcode::Loop ||
            instr.opcode == Opcode::If || instr.opcode == Opcode::Else) {
          indent += 2;
        }
      }
    }
  }
  ImGui::End();
}

}  // namespace view
}  // namespace wasp

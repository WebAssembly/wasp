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
#include "wasp/binary/lazy_function_names_subsection.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_global_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_local_names_subsection.h"
#include "wasp/binary/lazy_memory_section.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/lazy_module_name_subsection.h"
#include "wasp/binary/lazy_name_section.h"
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
static size_t s_section_selected;
static Index s_code_selected;

template <typename Sequence>
typename Sequence::const_iterator FindSection(const Sequence&, SectionId);

void ModuleWindow();

void SectionWindow(const std::string& title, size_t section_index);

template <typename T>
void SectionContents(T section, string_view name);
template <>
void SectionContents<LazyCodeSection<ErrorsNop>>(LazyCodeSection<ErrorsNop>,
                                                 string_view name);
template <>
void SectionContents<StartSection>(StartSection section, string_view name);
template <>
void SectionContents<ModuleNameSubsection>(ModuleNameSubsection section,
                                           string_view name);

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
  ModuleWindow();
  SectionWindow("Section preview", s_section_selected);
  CodeWindow("Code preview", s_code_selected);
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

void ModuleWindow() {
  if (ImGui::Begin("Module")) {
    size_t count = 0;
    for (auto section : s_sections) {
      std::string text;
      if (section.is_known()) {
        auto known = section.known();
        text = format("[{}] {}: {} bytes", count, known.id, known.data.size());
      } else if (section.is_custom()) {
        auto custom = section.custom();
        text = format("[{}] \"{}\": {} bytes", count, custom.name,
                      custom.data.size());
      }

      if (ImGui::Selectable(text.c_str(), count == s_section_selected)) {
        s_section_selected = count;
      }

      ++count;
    }
  }
  ImGui::End();
}

void SectionWindow(const std::string& title, size_t section_index) {
  auto section = s_sections[section_index];
  auto window_flags = ImGuiWindowFlags_HorizontalScrollbar;

  ErrorsNop errors;
  if (ImGui::Begin(title.c_str(), nullptr, window_flags)) {
    if (section.is_known()) {
      auto known = section.known();
      switch (known.id) {
        case SectionId::Custom:
          WASP_UNREACHABLE();
          break;

        case SectionId::Type:
          SectionContents(ReadTypeSection(known, s_features, errors), "Type");
          break;

        case SectionId::Import:
          SectionContents(ReadImportSection(known, s_features, errors),
                          "Import");
          break;

        case SectionId::Function:
          SectionContents(ReadFunctionSection(known, s_features, errors),
                          "Func");
          break;

        case SectionId::Table:
          SectionContents(ReadTableSection(known, s_features, errors), "Table");
          break;

        case SectionId::Memory:
          SectionContents(ReadMemorySection(known, s_features, errors),
                          "Memory");
          break;

        case SectionId::Global:
          SectionContents(ReadGlobalSection(known, s_features, errors),
                          "Global");
          break;

        case SectionId::Export:
          SectionContents(ReadExportSection(known, s_features, errors),
                          "Export");
          break;

        case SectionId::Start:
          SectionContents(ReadStartSection(known, s_features, errors), "Start");
          break;

        case SectionId::Element:
          SectionContents(ReadElementSection(known, s_features, errors),
                          "Element");
          break;

        case SectionId::Code:
          SectionContents(ReadCodeSection(known, s_features, errors), "Code");
          break;

        case SectionId::Data:
          SectionContents(ReadDataSection(known, s_features, errors), "Data");
          break;
      }
    } else if (section.is_custom()) {
      auto custom = section.custom();
      if (custom.name == "name") {
        auto name_section = ReadNameSection(custom.data, s_features, errors);
        for (auto subsection : name_section) {
          switch (subsection.id) {
            case NameSubsectionId::ModuleName:
              SectionContents(
                  ReadModuleNameSubsection(subsection.data, s_features, errors),
                  "ModuleName");
              break;

            case NameSubsectionId::FunctionNames:
              SectionContents(ReadFunctionNamesSubsection(subsection.data,
                                                          s_features, errors),
                              "FunctionNames");
              break;

            case NameSubsectionId::LocalNames:
              SectionContents(
                  ReadLocalNamesSubsection(subsection.data, s_features, errors),
                  "LocalNames");
              break;
          }
        }
      } else {
        // TODO
      }
    }
  }
  ImGui::End();
}

template <typename T>
void SectionContents(T section, string_view name) {
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
void SectionContents<LazyCodeSection<ErrorsNop>>(
    LazyCodeSection<ErrorsNop> section,
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

template <>
void SectionContents<StartSection>(StartSection section, string_view name) {
  size_t count = section ? 1 : 0;
  ImGui::Text("%s", format("{}[{}]\n", name, count).c_str());
  if (count > 0) {
    ImGui::Text("%s", format("  [0]: {}\n", 0, *section).c_str());
  }
}

template <>
void SectionContents<ModuleNameSubsection>(ModuleNameSubsection section,
                                           string_view name) {
  ImGui::Text("%s", format("{}\n", name).c_str());
  if (section) {
    ImGui::Text("%s", format("  [0]: {}\n", 0, *section).c_str());
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
  auto window_flags = ImGuiWindowFlags_HorizontalScrollbar;
  bool is_open = true;
  if (ImGui::Begin(title.c_str(), &is_open, window_flags)) {
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

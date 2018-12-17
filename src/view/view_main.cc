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
static std::vector<Index> s_instr_count;

static Features s_features;

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

template <typename T>
void DumpSection(T section, string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("  {}[{}]\n", name, *section.count).c_str());
    Index initial_count = InitialCount<typename T::sequence_type::value_type>();
    ImGuiListClipper clipper(*section.count);
    while (clipper.Step()) {
      auto it = section.sequence.begin(), end = section.sequence.end();
      std::advance(it, clipper.DisplayStart);
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && it != end;
           ++it, ++i) {
        ImGui::Text("%s",
                    format("    [{}]: {}\n", initial_count + i, *it).c_str());
      }
    }
  }
}

void DumpCode(Index i, Code code) {
  ErrorsNop errors;
  auto expr = ReadExpression(code.body.data, s_features, errors);
  int indent = 0;
  ImGuiListClipper clipper(s_instr_count[i]);
  auto it = expr.begin(), end = expr.end();
  while (clipper.Step()) {
    for (int i = 0; i < clipper.DisplayEnd && it != end; ++it, ++i) {
      auto instr = *it;
      if (instr.opcode == Opcode::End || instr.opcode == Opcode::Else) {
        indent -= 2;
      }

      if (i >= clipper.DisplayStart) {
        ImGui::Text("%*.s%s", indent, "", format("    {}\n", instr).c_str());
      }

      if (instr.opcode == Opcode::Block || instr.opcode == Opcode::Loop ||
          instr.opcode == Opcode::If || instr.opcode == Opcode::Else) {
        indent += 2;
      }
    }
  }
}

template <>
void DumpSection<LazyCodeSection<ErrorsNop>>(LazyCodeSection<ErrorsNop> section,
                                             string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("  {}[{}]\n", name, *section.count).c_str());
    Index initial_count = ImportCount(ExternalKind::Function);
    ImGuiListClipper clipper(*section.count);
    while (clipper.Step()) {
      auto it = section.sequence.begin(), end = section.sequence.end();
      std::advance(it, clipper.DisplayStart);
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd && it != end;
           ++it, ++i) {
        auto code = *it;
        if (ImGui::TreeNode(reinterpret_cast<void*>(i), "%s",
                            format("    [{}]: {} bytes\n", initial_count + i,
                                   code.body.data.size())
                                .c_str())) {
          DumpCode(i, code);
          ImGui::TreePop();
        }
      }
    }
  }
}

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
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      if (known.id == SectionId::Import) {
        auto sec = ReadImportSection(known, s_features, errors);
        for (auto import : sec.sequence) {
          s_import_count[static_cast<size_t>(import.kind())]++;
        }
      } else if (known.id == SectionId::Code) {
        auto sec = ReadCodeSection(known, s_features, errors);
        for (auto code : sec.sequence) {
          auto expr = ReadExpression(code.body.data, s_features, errors);
          auto size = std::distance(expr.begin(), expr.end());
          s_instr_count.push_back(size);
        }
      }
    }
  }
}

void ViewSection(KnownSection known, ErrorsNop& errors) {
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

void ViewMain() {
  SpanU8 data{s_buffer};
  ErrorsNop errors;
  s_features.EnableAll();
  auto module = ReadModule(data, s_features, errors);

  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      ViewSection(known, errors);
    }
  }
}

}  // namespace view
}  // namespace wasp

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

#include "src/base/file.h"
#include "src/binary/errors_nop.h"
#include "src/binary/errors_vector.h"
#include "src/binary/lazy_expr.h"
#include "src/binary/lazy_module.h"
#include "src/binary/lazy_section.h"
#include "src/binary/types.h"

namespace wasp {
namespace view {

using namespace ::wasp::binary;

static std::string s_filename;
static std::vector<u8> s_buffer;

template <typename T>
void DumpSection(T section, string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("  {}[{}]\n", name, *section.count).c_str());
    Index count = 0;
    for (auto item : section.sequence) {
      ImGui::Text("%s", format("    [{}]: {}\n", count++, item).c_str());
    }
  }
}

template <>
void DumpSection<LazyCodeSection<ErrorsVector>>(
    LazyCodeSection<ErrorsVector> section,
    string_view name) {
  if (section.count) {
    ImGui::Text("%s", format("  {}[{}]\n", name, *section.count).c_str());
    Index count = 0;
    for (auto item : section.sequence) {
      if (ImGui::TreeNode(
              reinterpret_cast<void*>(count), "%s",
              format("    [{}]: {} bytes\n", count, item.body.data.size())
                  .c_str())) {
        ErrorsNop errors;
        auto expr = ReadExpr(item.body.data, errors);
        int indent = 0;
        for (auto instr : expr) {
          if (instr.opcode == Opcode::End || instr.opcode == Opcode::Else) {
            indent -= 2;
          }

          ImGui::Text("%*.s%s", indent, "", format("    {}\n", instr).c_str());

          if (instr.opcode == Opcode::Block || instr.opcode == Opcode::Loop ||
              instr.opcode == Opcode::If || instr.opcode == Opcode::Else) {
            indent += 2;
          }
        }
        ImGui::TreePop();
      }
      ++count;
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
}

void ViewMain() {
  SpanU8 data{s_buffer};
  ErrorsVector errors;
  auto module = ReadModule(data, errors);

  if (ImGui::Begin("View")) {
    intptr_t i = 0;
    for (auto section : module.sections) {
      if (section.is_known()) {
        auto known = section.known();
        if (ImGui::TreeNode(
                reinterpret_cast<void*>(i), "%s",
                format("section {}: {} bytes", known.id, known.data.size())
                    .c_str())) {
          switch (known.id) {
            case SectionId::Custom:
              WASP_UNREACHABLE();
              break;

            case SectionId::Type:
              DumpSection(ReadTypeSection(known, errors), "Type");
              break;

            case SectionId::Import:
              DumpSection(ReadImportSection(known, errors), "Import");
              break;

            case SectionId::Function:
              DumpSection(ReadFunctionSection(known, errors), "Func");
              break;

            case SectionId::Table:
              DumpSection(ReadTableSection(known, errors), "Table");
              break;

            case SectionId::Memory:
              DumpSection(ReadMemorySection(known, errors), "Memory");
              break;

            case SectionId::Global:
              DumpSection(ReadGlobalSection(known, errors), "Global");
              break;

            case SectionId::Export:
              DumpSection(ReadExportSection(known, errors), "Export");
              break;

            case SectionId::Start: {
              auto section = ReadStartSection(known, errors);
              size_t count = section.start() ? 1 : 0;
              print("  Start[{}]\n", count);
              if (count > 0) {
                print("    [0]: {}\n", *section.start());
              }
              break;
            }

            case SectionId::Element:
              DumpSection(ReadElementSection(known, errors), "Element");
              break;

            case SectionId::Code:
              DumpSection(ReadCodeSection(known, errors), "Code");
              break;

            case SectionId::Data:
              DumpSection(ReadDataSection(known, errors), "Data");
              break;
          }
          ImGui::TreePop();
        }
      }
      ++i;
    }
  }
  ImGui::End();
}

}  // namespace view
}  // namespace wasp

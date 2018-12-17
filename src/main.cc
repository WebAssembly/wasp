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

#include <string>

#include "src/base/file.h"
#include "src/base/formatters.h"
#include "src/base/macros.h"
#include "src/base/types.h"
#include "src/binary/errors_vector.h"
#include "src/binary/formatters.h"
#include "src/binary/lazy_module.h"
#include "src/binary/lazy_name_section.h"
#include "src/binary/lazy_section.h"
#include "src/binary/types.h"

using namespace ::wasp;
using namespace ::wasp::binary;

template <typename T>
void PrintSection(T section, string_view name) {
  if (section.count) {
    print("  {}[{}]\n", name, *section.count);
    Index count = 0;
    for (auto item : section.sequence) {
      print("    [{}]: {}\n", count++, item);
    }
  }
}

int main(int argc, char** argv) {
  argc--;
  argv++;
  if (argc == 0) {
    print("No files.\n");
    return 1;
  }

  std::string filename{argv[0]};
  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print("Error reading file.\n");
    return 1;
  }

  SpanU8 data{*optbuf};

  ErrorsVector errors;
  auto module = ReadModule(data, errors);
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      print("section {}: {} bytes\n", known.id, known.data.size());
      switch (known.id) {
        case SectionId::Custom:
          WASP_UNREACHABLE();

        case SectionId::Type:
          PrintSection(ReadTypeSection(known, errors), "Type");
          break;

        case SectionId::Import:
          PrintSection(ReadImportSection(known, errors), "Import");
          break;

        case SectionId::Function:
          PrintSection(ReadFunctionSection(known, errors), "Func");
          break;

        case SectionId::Table:
          PrintSection(ReadTableSection(known, errors), "Table");
          break;

        case SectionId::Memory:
          PrintSection(ReadMemorySection(known, errors), "Memory");
          break;

        case SectionId::Global:
          PrintSection(ReadGlobalSection(known, errors), "Global");
          break;

        case SectionId::Export:
          PrintSection(ReadExportSection(known, errors), "Export");
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
          PrintSection(ReadElementSection(known, errors), "Element");
          break;

        case SectionId::Code:
          PrintSection(ReadCodeSection(known, errors), "Code");
          break;

        case SectionId::Data:
          PrintSection(ReadDataSection(known, errors), "Data");
          break;
      }
    } else if (section.is_custom()) {
      auto custom = section.custom();
      print("section \"{}\": {} bytes\n", custom.name, custom.data.size());
      if (custom.name == "name") {
        auto section = ReadNameSection(custom, errors);
        for (auto subsection : section) {
          switch (subsection.id) {
            case NameSubsectionId::ModuleName: {
              print("  ModuleName\n");
              auto opt_name = ReadModuleNameSubsection(subsection.data, errors);
              if (opt_name) {
                print("    \"{}\"\n", *opt_name);
              }
              break;
            }

            case NameSubsectionId::FunctionNames:
              PrintSection(ReadFunctionNamesSubsection(subsection, errors),
                           "FunctionNames");
              break;

            case NameSubsectionId::LocalNames:
              PrintSection(ReadLocalNamesSubsection(subsection, errors),
                           "LocalNames");
              break;
          }
        }
      } else {
        // Unknown custom section; print raw.
        print("  {}", custom);
      }
    }
  }

  return 0;
}

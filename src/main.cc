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
#include "src/base/types.h"
#include "src/binary/encoding.h"
#include "src/binary/formatters.h"
#include "src/binary/types.h"
#include "src/binary/reader.h"

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
        case encoding::Section::Type:
          PrintSection(ReadTypeSection(known, errors), "Type");
          break;

        case encoding::Section::Import:
          PrintSection(ReadImportSection(known, errors), "Import");
          break;

        case encoding::Section::Function:
          PrintSection(ReadFunctionSection(known, errors), "Func");
          break;

        case encoding::Section::Table:
          PrintSection(ReadTableSection(known, errors), "Table");
          break;

        case encoding::Section::Memory:
          PrintSection(ReadMemorySection(known, errors), "Memory");
          break;

        case encoding::Section::Global:
          PrintSection(ReadGlobalSection(known, errors), "Global");
          break;

        case encoding::Section::Export:
          PrintSection(ReadExportSection(known, errors), "Export");
          break;

        case encoding::Section::Element:
          PrintSection(ReadElementSection(known, errors), "Element");
          break;
      }
    }
  }

  return 0;
}

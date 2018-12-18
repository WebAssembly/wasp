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

// Dummy function defined so we can use the macro below. The return value must
// have the same structure as a lazy section.
template <typename Errors>
LazyTypeSection<Errors> ReadCustomSection(KnownSection section,
                                          Errors& errors) {
  WASP_UNREACHABLE();
}

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

template <>
void PrintSection<StartSection>(StartSection section, string_view name) {
  size_t count = section ? 1 : 0;
  print("  {}[{}]\n", name, count);
  if (count > 0) {
    print("    [0]: {}\n", *section);
  }
}

template <>
void PrintSection<ModuleNameSubsection>(ModuleNameSubsection section,
                                        string_view name) {
  size_t count = section ? 1 : 0;
  print("  {}[{}]\n", name, count);
  if (count > 0) {
    print("    [0]: \"{}\"\n", *section);
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
#define WASP_V(id, Name, str)                                \
  case SectionId::Name:                                      \
    PrintSection(Read##Name##Section(known, errors), #Name); \
    break;
        WASP_FOREACH_SECTION(WASP_V)
#undef WASP_V
      }
    } else if (section.is_custom()) {
      auto custom = section.custom();
      print("section \"{}\": {} bytes\n", custom.name, custom.data.size());
      if (custom.name == "name") {
        auto section = ReadNameSection(custom, errors);
        for (auto subsection : section) {
          switch (subsection.id) {
#define WASP_V(id, Name, str)                                        \
  case NameSubsectionId::Name:                                       \
    PrintSection(Read##Name##Subsection(subsection, errors), #Name); \
    break;
            WASP_FOREACH_NAME_SUBSECTION_ID(WASP_V)
#undef WASP_V
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

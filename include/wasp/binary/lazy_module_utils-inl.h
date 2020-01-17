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

#include <utility>

#include "wasp/binary/sections.h"
#include "wasp/binary/sections_name.h"

namespace wasp {
namespace binary {

template <typename F>
void ForEachFunctionName(LazyModule& module,
                         F&& f,
                         const Features& features,
                         Errors& errors) {
  Index imported_function_count = 0;
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      switch (known.id) {
        case SectionId::Import:
          for (auto import :
               ReadImportSection(known, features, errors).sequence) {
            if (import.kind() == ExternalKind::Function) {
              f(IndexNamePair{imported_function_count++, import.name});
            }
          }
          break;

        case SectionId::Export:
          for (auto export_ :
               ReadExportSection(known, features, errors).sequence) {
            if (export_.kind == ExternalKind::Function) {
              f(IndexNamePair{export_.index, export_.name});
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
              f(IndexNamePair{name_assoc.index, name_assoc.name});
            }
          }
        }
      }
    }
  }
}

template <typename Iterator>
Iterator CopyFunctionNames(LazyModule& module,
                           Iterator out,
                           const Features& features,
                           Errors& errors) {
  ForEachFunctionName(module,
                      [&out](const IndexNamePair& pair) { *out++ = pair; },
                      features, errors);
  return out;
}

inline Index GetImportCount(LazyModule& module,
                            ExternalKind kind,
                            const Features& features,
                            Errors& errors) {
  Index count = 0;
  for (auto section : module.sections) {
    if (section.is_known()) {
      auto known = section.known();
      if (known.id == SectionId::Import) {
        for (auto import :
             ReadImportSection(known, features, errors).sequence) {
          if (import.kind() == kind) {
            count++;
          }
        }
      }
    }
  }
  return count;
}

}  // namespace binary
}  // namespace wasp

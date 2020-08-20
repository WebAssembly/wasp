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

#include "wasp/base/errors_nop.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/read/context.h"
#include "wasp/binary/sections.h"

namespace wasp::binary {

template <typename F>
void ForEachFunctionName(LazyModule& module, F&& f) {
  ErrorsNop errors;
  LazyModule copy{module.data, module.context.features, errors};

  Index imported_function_count = 0;
  for (auto section : module.sections) {
    if (section->is_known()) {
      auto known = section->known();
      switch (known->id) {
        case SectionId::Import:
          for (auto import : ReadImportSection(known, copy.context).sequence) {
            if (import->kind() == ExternalKind::Function) {
              f(IndexNamePair{imported_function_count++, import->name});
            }
          }
          break;

        case SectionId::Export:
          for (auto export_ : ReadExportSection(known, copy.context).sequence) {
            if (export_->kind == ExternalKind::Function) {
              f(IndexNamePair{export_->index, export_->name});
            }
          }
          break;

        default:
          break;
      }
    } else if (section->is_custom()) {
      auto custom = section->custom();
      if (*custom->name == "name") {
        for (auto subsection : ReadNameSection(custom, copy.context)) {
          if (subsection->id == NameSubsectionId::FunctionNames) {
            for (auto name_assoc :
                 ReadFunctionNamesSubsection(*subsection, copy.context)
                     .sequence) {
              f(IndexNamePair{name_assoc->index, name_assoc->name});
            }
          }
        }
      }
    }
  }
}

template <typename Iterator>
Iterator CopyFunctionNames(LazyModule& module, Iterator out) {
  ForEachFunctionName(module,
                      [&out](const IndexNamePair& pair) { *out++ = pair; });
  return out;
}

inline Index GetImportCount(LazyModule& module, ExternalKind kind) {
  ErrorsNop errors;
  LazyModule copy{module.data, module.context.features, errors};

  Index count = 0;
  for (auto section : copy.sections) {
    if (section->is_known()) {
      auto known = section->known();
      if (known->id == SectionId::Import) {
        for (auto import : ReadImportSection(known, copy.context).sequence) {
          if (import->kind() == kind) {
            count++;
          }
        }
      }
    }
  }
  return count;
}

}  // namespace wasp::binary

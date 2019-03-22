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

#ifndef WASP_VALID_VALIDATE_IMPORT_H_
#define WASP_VALID_VALIDATE_IMPORT_H_

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/binary/import.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_global_type.h"
#include "wasp/valid/validate_index.h"
#include "wasp/valid/validate_memory_type.h"
#include "wasp/valid/validate_table_type.h"

namespace wasp {
namespace valid {

template <typename Errors>
bool Validate(const binary::Import& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, "import"};
  bool valid = true;
  switch (value.kind()) {
    case binary::ExternalKind::Function:
      context.functions.push_back(value.index());
      return ValidateIndex(value.index(), context.types.size(),
                           "function type index", errors);

    case binary::ExternalKind::Table:
      context.tables.push_back(value.table_type());
      valid &= Validate(value.table_type(), context, features, errors);
      if (context.tables.size() > 1 && !features.reference_types_enabled()) {
        errors.OnError("Too many tables, must be 1 or fewer");
        valid = false;
      }
      break;

    case binary::ExternalKind::Memory:
      context.memories.push_back(value.memory_type());
      valid &= Validate(value.memory_type(), context, features, errors);
      if (context.memories.size() > 1) {
        errors.OnError("Too many memories, must be 1 or fewer");
        valid = false;
      }
      break;

    case binary::ExternalKind::Global:
      context.globals.push_back(value.global_type());
      context.imported_global_count++;
      valid &= Validate(value.global_type(), context, features, errors);
      break;

    default:
      WASP_UNREACHABLE();
      break;
  }
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_IMPORT_H_

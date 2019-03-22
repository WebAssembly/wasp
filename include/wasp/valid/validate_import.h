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
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_function.h"
#include "wasp/valid/validate_global_type.h"
#include "wasp/valid/validate_index.h"
#include "wasp/valid/validate_memory.h"
#include "wasp/valid/validate_table.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::Import& value,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "import"};
  bool valid = true;
  switch (value.kind()) {
    case binary::ExternalKind::Function:
      valid &=
          Validate(binary::Function{value.index()}, context, features, errors);
      context.imported_function_count++;
      break;

    case binary::ExternalKind::Table:
      valid &= Validate(binary::Table{value.table_type()}, context, features,
                        errors);
      break;

    case binary::ExternalKind::Memory:
      valid &= Validate(binary::Memory{value.memory_type()}, context, features,
                        errors);
      break;

    case binary::ExternalKind::Global:
      context.globals.push_back(value.global_type());
      context.imported_global_count++;
      valid &= Validate(value.global_type(), context, features, errors);
      if (value.global_type().mut == binary::Mutability::Var &&
          !features.mutable_globals_enabled()) {
        errors.OnError("Mutable globals cannot be imported");
        valid = false;
      }
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

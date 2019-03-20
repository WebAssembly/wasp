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

#ifndef WASP_VALID_VALIDATE_EXPORT_H_
#define WASP_VALID_VALIDATE_EXPORT_H_

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/binary/export.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_index.h"

namespace wasp {
namespace valid {

template <typename Errors>
bool Validate(const binary::Export& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, "export"};
  switch (value.kind) {
    case binary::ExternalKind::Function:
      return ValidateIndex(value.index, context.functions.size(),
                           "function index", errors);

    case binary::ExternalKind::Table:
      return ValidateIndex(value.index, context.tables.size(), "table index",
                           errors);

    case binary::ExternalKind::Memory:
      return ValidateIndex(value.index, context.memories.size(), "memory index",
                           errors);

    case binary::ExternalKind::Global: {
      if (!ValidateIndex(value.index, context.globals.size(), "global index",
                        errors)) {
        return false;
      }

      const auto& global = context.globals[value.index];
      if (global.mut == binary::Mutability::Var &&
          !features.mutable_globals_enabled()) {
        errors.OnError("Mutable globals cannot be exported");
        return false;
      }
      return true;
    }

    default:
      WASP_UNREACHABLE();
      return false;
  }
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_EXPORT_H_

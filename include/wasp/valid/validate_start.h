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

#ifndef WASP_VALID_VALIDATE_START_H_
#define WASP_VALID_VALIDATE_START_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/binary/start.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_index.h"

namespace wasp {
namespace valid {

bool Validate(const binary::Start& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard guard{errors, "start"};
  if (!ValidateIndex(value.func_index, context.functions.size(),
                     "function index", errors)) {
    return false;
  }

  bool valid = true;
  auto function = context.functions[value.func_index];
  if (function.type_index < context.types.size()) {
    const auto& type_entry = context.types[function.type_index];
    if (type_entry.type.param_types.size() != 0) {
      errors.OnError(format("Expected start function to have 0 params, got {}",
                            type_entry.type.param_types.size()));
      valid = false;
    }

    if (type_entry.type.result_types.size() != 0) {
      errors.OnError(format("Expected start function to have 0 results, got {}",
                            type_entry.type.result_types.size()));
      valid = false;
    }
  }
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_START_H_

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

#ifndef WASP_VALID_VALIDATE_FUNCTION_TYPE_H_
#define WASP_VALID_VALIDATE_FUNCTION_TYPE_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/binary/function_type.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_function_type.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::FunctionType& value,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "function type"};
  if (value.result_types.size() > 1 && !features.multi_value_enabled()) {
    errors.OnError(format("Expected result type count of 0 or 1, got {}",
                          value.result_types.size()));
    return false;
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_FUNCTION_TYPE_H_

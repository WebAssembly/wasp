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

#ifndef WASP_VALID_VALIDATE_FUNCTION_H_
#define WASP_VALID_VALIDATE_FUNCTION_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/binary/function.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_index.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::Function& value,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "function"};
  context.functions.push_back(value);
  return ValidateIndex(value.type_index, context.types.size(),
                       "function type index", errors);
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_FUNCTION_H_

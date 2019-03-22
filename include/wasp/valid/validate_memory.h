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

#ifndef WASP_VALID_VALIDATE_MEMORY_H_
#define WASP_VALID_VALIDATE_MEMORY_H_

#include "wasp/base/features.h"
#include "wasp/binary/memory.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_memory_type.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::Memory& value,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "memory"};
  context.memories.push_back(value.memory_type);
  bool valid = Validate(value.memory_type, context, features, errors);
  if (context.memories.size() > 1) {
    errors.OnError("Too many memories, must be 1 or fewer");
    valid = false;
  }
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_MEMORY_H_

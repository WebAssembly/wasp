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

#ifndef WASP_VALID_VALIDATE_LOCALS_H_
#define WASP_VALID_VALIDATE_LOCALS_H_

#include <limits>

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/base/types.h"
#include "wasp/binary/locals.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::Locals& value,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "locals"};
  const size_t old_count = context.locals.size();
  const Index max = std::numeric_limits<Index>::max();
  if (old_count > max - value.count) {
    errors.OnError(format("Too many locals; max is {}, got {}", max,
                          static_cast<u64>(old_count) + value.count));
    return false;
  }
  const size_t new_count = old_count + value.count;
  context.locals.reserve(new_count);
  for (Index i = 0; i < value.count; ++i) {
    context.locals.push_back(value.type);
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_LOCALS_H_

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

#ifndef WASP_VALID_VALIDATE_INDEX_H_
#define WASP_VALID_VALIDATE_INDEX_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/types.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"

namespace wasp {
namespace valid {

inline bool ValidateIndex(Index index,
                          Index max,
                          string_view desc,
                          Errors& errors) {
  if (index >= max) {
    errors.OnError(
        format("Invalid {} {}, must be less than {}", desc, index, max));
    return false;
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_INDEX_H_

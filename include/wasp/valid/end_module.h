//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_VALID_END_MODULE_H_
#define WASP_VALID_END_MODULE_H_

#include "wasp/base/format.h"
#include "wasp/binary/errors.h"
#include "wasp/valid/context.h"

namespace wasp {
namespace valid {

inline bool EndModule(Context& context,
                      const Features& features,
                      Errors& errors) {
  auto defined_function_count =
      context.functions.size() - context.imported_function_count;
  // TODO: This should be a binary reader error (not a validation error), but
  // currently there's not enough context to do it there.
  if (defined_function_count != context.code_count) {
    errors.OnError(format("Expected code count of {}, but got {}",
                          defined_function_count, context.code_count));
    return false;
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_END_MODULE_H_

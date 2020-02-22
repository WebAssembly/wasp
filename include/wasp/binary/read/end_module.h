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

#ifndef WASP_BINARY_END_MODULE_H_
#define WASP_BINARY_END_MODULE_H_

#include "wasp/base/format.h"
#include "wasp/binary/errors.h"
#include "wasp/binary/read/context.h"

namespace wasp {
namespace binary {

inline bool EndModule(SpanU8* data, Context& context) {
  if (context.defined_function_count != context.code_count) {
    context.errors.OnError(
        *data, format("Expected code count of {}, but got {}",
                      context.defined_function_count, context.code_count));
    return false;
  }
  if (context.declared_data_count &&
      *context.declared_data_count != context.data_count) {
    context.errors.OnError(
        *data, format("Expected data count of {}, but got {}",
                      *context.declared_data_count, context.data_count));
  }
  return true;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_END_MODULE_H_

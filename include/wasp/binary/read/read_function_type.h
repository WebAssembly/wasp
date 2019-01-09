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

#ifndef WASP_BINARY_READ_READ_FUNCTION_TYPE_H_
#define WASP_BINARY_READ_READ_FUNCTION_TYPE_H_

#include "wasp/base/features.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/function_type.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_value_type.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<FunctionType> Read(SpanU8* data,
                            const Features& features,
                            Errors& errors,
                            Tag<FunctionType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "function type"};
  WASP_TRY_READ(param_types,
                ReadVector<ValueType>(data, features, errors, "param types"));
  WASP_TRY_READ(result_types,
                ReadVector<ValueType>(data, features, errors, "result types"));
  return FunctionType{std::move(param_types), std::move(result_types)};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_FUNCTION_TYPE_H_

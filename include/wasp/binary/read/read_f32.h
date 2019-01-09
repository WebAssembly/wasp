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

#ifndef WASP_BINARY_READ_READ_F32_H_
#define WASP_BINARY_READ_READ_F32_H_

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/types.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_bytes.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<f32> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard<Errors> guard{errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), features, errors));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_F32_H_

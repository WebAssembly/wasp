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

#ifndef WASP_BINARY_READ_READ_LIMITS_H_
#define WASP_BINARY_READ_READ_LIMITS_H_

#include "wasp/base/features.h"
#include "wasp/binary/limits.h"
#include "wasp/binary/encoding/limits_flags_encoding.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_u8.h"
#include "wasp/binary/read/read_u32.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Limits> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Limits>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "limits"};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, features, errors), "flags");
  auto decoded = encoding::LimitsFlags::Decode(flags, features);
  if (!decoded) {
    errors.OnError(*data, format("Invalid flags value: {}", flags));
    return nullopt;
  }

  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, features, errors), "min");

  if (decoded->has_max == encoding::HasMax::No) {
    return Limits{min};
  } else {
    WASP_TRY_READ_CONTEXT(max, Read<u32>(data, features, errors), "max");
    return Limits{min, max, decoded->shared};
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_LIMITS_H_

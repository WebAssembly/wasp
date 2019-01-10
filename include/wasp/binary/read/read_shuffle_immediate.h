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

#ifndef WASP_BINARY_READ_READ_SHUFFLE_IMMEDIATE_H_
#define WASP_BINARY_READ_READ_SHUFFLE_IMMEDIATE_H_

#include "wasp/base/features.h"
#include "wasp/binary/shuffle_immediate.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_u8.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<ShuffleImmediate> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<ShuffleImmediate>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "shuffle immediate"};
  ShuffleImmediate immediate;
  for (int i = 0; i < 16; ++i) {
    WASP_TRY_READ(byte, Read<u8>(data, features, errors));
    immediate[i] = byte;
  }
  return immediate;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_SHUFFLE_IMMEDIATE_H_

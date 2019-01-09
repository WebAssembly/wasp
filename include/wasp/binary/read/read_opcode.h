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

#ifndef WASP_BINARY_READ_READ_OPCODE_H_
#define WASP_BINARY_READ_READ_OPCODE_H_

#include "wasp/base/features.h"
#include "wasp/binary/opcode.h"
#include "wasp/binary/encoding/opcode_encoding.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_u8.h"
#include "wasp/binary/read/read_u32.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Opcode> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Opcode>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "opcode"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));

  if (encoding::Opcode::IsPrefixByte(val, features)) {
    WASP_TRY_READ(code, Read<u32>(data, features, errors));
    auto decoded = encoding::Opcode::Decode(val, code, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown opcode: {} {}", val, code));
      return nullopt;
    }
    return decoded;
  } else {
    auto decoded = encoding::Opcode::Decode(val, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown opcode: {}", val));
      return nullopt;
    }
    return decoded;
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_OPCODE_H_

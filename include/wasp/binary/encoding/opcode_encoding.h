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

#ifndef WASP_BINARY_OPCODE_ENCODING_H
#define WASP_BINARY_OPCODE_ENCODING_H

#include "wasp/base/types.h"
#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/binary/opcode.h"

namespace wasp {
namespace binary {
namespace encoding {

struct Opcode {
#define WASP_V(prefix, val, Name, str, ...) static constexpr u8 Name = val;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V

  static optional<::wasp::binary::Opcode> Decode(u8, const Features&);
};

// static
inline optional<::wasp::binary::Opcode> Opcode::Decode(
    u8 val,
    const Features& features) {
  switch (val) {
#define WASP_V(prefix, val, Name, str) \
  case Name:                           \
    return ::wasp::binary::Opcode::Name;
#define WASP_FEATURE_V(prefix, val, Name, str, feature) \
  case Name:                                            \
    if (features.feature##_enabled()) {                 \
      return ::wasp::binary::Opcode::Name;              \
    }
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_OPCODE_ENCODING_H

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
  static constexpr u8 MiscPrefix = 0xfc;
  static constexpr u8 SimdPrefix = 0xfd;

  static bool IsPrefixByte(u8, const Features&);
  static optional<::wasp::binary::Opcode> Decode(u8 code, const Features&);
  static optional<::wasp::binary::Opcode> Decode(u8 prefix,
                                                 u32 code,
                                                 const Features&);
};

// static
inline bool Opcode::IsPrefixByte(u8 code, const Features& features) {
  switch (code) {
    case MiscPrefix:
      return features.saturating_float_to_int_enabled() ||
             features.bulk_memory_enabled();

    case SimdPrefix:
      return features.simd_enabled();

    default:
      return false;
  }
}

// static
inline optional<::wasp::binary::Opcode> Opcode::Decode(
    u8 code,
    const Features& features) {
  switch (code) {
#define WASP_V(prefix, code, Name, str) \
  case code:                            \
    return ::wasp::binary::Opcode::Name;
#define WASP_FEATURE_V(prefix, code, Name, str, feature) \
  case code:                                             \
    if (features.feature##_enabled()) {                  \
      return ::wasp::binary::Opcode::Name;               \
    }                                                    \
    break;
#define WASP_PREFIX_V(...) /* Invalid. */
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      break;
  }
  return nullopt;
}

namespace {

constexpr u64 MakePrefixCode(u8 prefix, u32 code) {
  return (u64{prefix} << 32) | code;
}

}  // namespace

// static
inline optional<::wasp::binary::Opcode> Opcode::Decode(
    u8 prefix,
    u32 code,
    const Features& features) {
  switch (MakePrefixCode(prefix, code)) {
#define WASP_V(...) /* Invalid. */
#define WASP_FEATURE_V(...) /* Invalid. */
#define WASP_PREFIX_V(prefix, code, Name, str, feature) \
  case MakePrefixCode(prefix, code):                    \
    if (features.feature##_enabled()) {                 \
      return ::wasp::binary::Opcode::Name;              \
    }                                                   \
    break;
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      break;
  }
  return nullopt;
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_OPCODE_ENCODING_H

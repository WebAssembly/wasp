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

#ifndef WASP_BINARY_WRITE_WRITE_FIXED_VAR_INT_H_
#define WASP_BINARY_WRITE_WRITE_FIXED_VAR_INT_H_

#include <type_traits>

#include "wasp/base/types.h"
#include "wasp/binary/var_int.h"
#include "wasp/binary/write/write_u8.h"

namespace wasp {
namespace binary {

// Unsigned integers.
template <typename T, typename Iterator>
typename std::enable_if<!std::is_signed<T>::value, Iterator>::type
WriteFixedVarInt(T value, Iterator out, size_t length = VarInt<T>::kMaxBytes) {
  using V = VarInt<T>;
  assert(length <= V::kMaxBytes);
  for (size_t i = 0; i < length - 1; ++i) {
    out = Write(static_cast<u8>((value & V::kByteMask) | V::kExtendBit), out);
    value >>= V::kBitsPerByte;
  }
  out = Write(static_cast<u8>(value & V::kByteMask), out);
  assert((value >> V::kBitsPerByte) == 0);
  return out;
}

// Signed integers.
template <typename T, typename Iterator>
typename std::enable_if<std::is_signed<T>::value, Iterator>::type
WriteFixedVarInt(T value, Iterator out, size_t length = VarInt<T>::kMaxBytes) {
  using V = VarInt<T>;
  assert(length <= V::kMaxBytes);
  for (size_t i = 0; i < length - 1; ++i) {
    out = Write(static_cast<u8>((value & V::kByteMask) | V::kExtendBit), out);
    value >>= V::kBitsPerByte;
  }
  out = Write(static_cast<u8>(value & V::kByteMask), out);
  assert((value >> V::kBitsPerByte) == 0 || (value >> V::kBitsPerByte) == -1);
  return out;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_FIXED_VAR_INT_H_

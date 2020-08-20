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

#ifndef WASP_BINARY_VAR_INT_H_
#define WASP_BINARY_VAR_INT_H_

#include "wasp/base/types.h"

namespace wasp::binary {

template <typename T>
struct VarInt {
  static constexpr int kByteMask = 0x7f;
  static constexpr int kExtendBit = 0x80;
  static constexpr int kSignBit = 0x40;

  static constexpr int kBitsPerByte = 7;

  static constexpr int kMaxBytes =
      (sizeof(T) * 8 + (kBitsPerByte - 1)) / kBitsPerByte;
  static constexpr int kUsedBitsInLastByte =
      sizeof(T) * 8 - kBitsPerByte * (kMaxBytes - 1);
};

}  // namespace wasp::binary

#endif  // WASP_BINARY_VAR_INT_H_

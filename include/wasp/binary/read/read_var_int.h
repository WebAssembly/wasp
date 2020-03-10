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

#ifndef WASP_BINARY_READ_READ_VAR_INT_H_
#define WASP_BINARY_READ_READ_VAR_INT_H_

#include <type_traits>

#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/read.h"
#include "wasp/binary/read/context.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/var_int.h"

namespace wasp {
namespace binary {

struct Context;

template <typename S>
S SignExtend(std::make_unsigned_t<S> x, int N) {
  constexpr size_t kNumBits = sizeof(S) * 8;
  return static_cast<S>(x << (kNumBits - N - 1)) >> (kNumBits - N - 1);
}

template <typename T>
OptAt<T> ReadVarInt(SpanU8* data, Context& context, string_view desc) {
  using U = std::make_unsigned_t<T>;
  constexpr bool is_signed = std::is_signed_v<T>;
  constexpr int kByteMask = VarInt<T>::kByteMask;
  constexpr int kLastByteMaskBits =
      VarInt<T>::kUsedBitsInLastByte - (is_signed ? 1 : 0);
  constexpr u8 kLastByteMask = ~((1 << kLastByteMaskBits) - 1);
  constexpr u8 kLastByteOnes = kLastByteMask & kByteMask;

  ErrorsContextGuard error_guard{context.errors, *data, desc};
  LocationGuard guard{data};

  U result{};
  for (int i = 0;;) {
    WASP_TRY_READ(byte, Read<u8>(data, context));

    const int shift = i * 7;
    result |= U(byte & kByteMask) << shift;

    if (++i == VarInt<T>::kMaxBytes) {
      if ((byte & kLastByteMask) == 0 ||
          (is_signed && (byte & kLastByteMask) == kLastByteOnes)) {
        return MakeAt(guard.range(data), T(result));
      }
      const u8 zero_ext = byte & ~kLastByteMask & kByteMask;
      const u8 one_ext = (byte | kLastByteOnes) & kByteMask;
      if (is_signed) {
        context.errors.OnError(
            *data, format("Last byte of {} must be sign "
                          "extension: expected {:#2x} or {:#2x}, got {:#2x}",
                          desc, zero_ext, one_ext, byte));
      } else {
        context.errors.OnError(*data,
                               format("Last byte of {} must be zero "
                                      "extension: expected {:#2x}, got {:#2x}",
                                      desc, zero_ext, byte));
      }
      return nullopt;
    } else if ((byte & VarInt<T>::kExtendBit) == 0) {
      return MakeAt(guard.range(data),
                    is_signed ? SignExtend<T>(result, 6 + shift) : T(result));
    }
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_VAR_INT_H_

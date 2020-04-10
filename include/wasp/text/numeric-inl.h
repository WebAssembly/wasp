//
// Copyright 2020 WebAssembly Community Group participants
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

#include <algorithm>
#include <cassert>
#include <charconv>
#include <limits>

#include "third_party/gdtoa/gdtoa.h"
#include "wasp/base/bitcast.h"

namespace wasp {
namespace text {

template <int base>
bool IsDigit(u8 c);

template <>
inline bool IsDigit<10>(u8 c) {
  return c >= '0' && c <= '9';
}

template <>
inline bool IsDigit<16>(u8 c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

template <typename T, int base>
auto ParseInteger(SpanU8 span) -> optional<T> {
  static constexpr const u8 DigitToValue[256] = {
      0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  //
      0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 10, 11, 12, 13, 14, 15,
  };

  constexpr const T max_div_base = std::numeric_limits<T>::max() / base;
  constexpr const T max_mod_base = std::numeric_limits<T>::max() % base;
  T value = 0;
  for (auto c : span) {
    if (c == '_') {
      continue;
    }
    assert(IsDigit<base>(c));
    T digit = DigitToValue[c];
    if (value > max_div_base ||
        (value == max_div_base && digit > max_mod_base)) {
      return nullopt;
    }
    value = value * base + digit;
  }
  return value;
}

template <typename T>
auto StrToNat(LiteralInfo info, SpanU8 span) -> optional<T> {
  static_assert(!std::is_signed<T>::value, "T must be unsigned");
  if (info.base == Base::Decimal) {
    return ParseInteger<T, 10>(span);
  } else {
    auto is_hex_prefix = [](SpanU8 span) {
      return span.size() > 2 && span[0] == '0' &&
             (span[1] == 'x' || span[1] == 'X');
    };

    assert(info.base == Base::Hex && is_hex_prefix(span));
    return ParseInteger<T, 16>(span.subspan(2));
  }
}

inline void RemoveSign(SpanU8& span, Sign sign) {
  if (sign != Sign::None) {
    remove_prefix(&span, 1);  // Remove + or -.
  }
}

template <typename T>
auto StrToInt(LiteralInfo info, SpanU8 span) -> optional<T> {
  using U = typename std::make_unsigned<T>::type;
  using S = typename std::make_signed<T>::type;

  RemoveSign(span, info.sign);

  auto value_opt = StrToNat<U>(info, span);
  if (!value_opt) {
    return nullopt;
  }
  U value = *value_opt;

  // The signed range is [-2**N, 2**N-1], so the maximum is larger for negative
  // numbers than positive numbers.
  auto max =
      U{std::numeric_limits<S>::max()} + (info.sign == Sign::Minus ? 1 : 0);
  if (value > max) {
    return nullopt;
  }
  if (info.sign == Sign::Minus) {
    value = ~value + 1;  // ~N + 1 is equivalent to -N.
  }
  return value;
}

inline void RemoveUnderscores(SpanU8 span, std::vector<u8>& out) {
  std::copy_if(span.begin(), span.end(), std::back_inserter(out),
               [](u8 c) { return c != '_'; });
}

template <typename T>
auto StrToR(const char* str, T* value) -> int;

template <>
inline auto StrToR<f32>(const char* str, f32* value) -> int {
  return strtorf(str, nullptr, FPI_Round_near, value);
}

template <>
inline auto StrToR<f64>(const char* str, f64* value) -> int {
  return strtord(str, nullptr, FPI_Round_near, value);
}

template <typename T>
auto ParseFloat(SpanU8 span) -> optional<T> {
  T value;
  int result = StrToR(reinterpret_cast<const char*>(span.begin()), &value);
  if ((result & STRTOG_Retmask) == STRTOG_NoNumber ||
      (result & STRTOG_Overflow) != 0) {
    return nullopt;
  }
  return value;
}

template <typename Float>
struct FloatTraits {};

template <>
struct FloatTraits<f32> {
  using Int = u32;
  static constexpr Int signbit = 0x8000'0000u;
  static constexpr int exp_shift = 23;
  static constexpr int exp_bias = 128;
  static constexpr int exp_min = -exp_bias;
  static constexpr int exp_max = 127;
  static constexpr Int significand_mask = 0x7f'ffff;
  static constexpr Int canonical_nan = 0x40'0000;
};

template <>
struct FloatTraits<f64> {
  using Int = u64;
  static constexpr Int signbit = 0x8000'0000'0000'0000ull;
  static constexpr int exp_shift = 52;
  static constexpr int exp_bias = 1024;
  static constexpr int exp_min = -exp_bias;
  static constexpr int exp_max = 1023;
  static constexpr Int significand_mask = 0xf'ffff'ffff'ffffull;
  static constexpr Int canonical_nan = 0x8'0000'0000'0000ull;
};

template <typename T>
auto MakeFloat(Sign sign, int exp, typename FloatTraits<T>::Int significand)
    -> T {
  using Traits = FloatTraits<T>;
  using Int = typename Traits::Int;
  assert(exp >= Traits::exp_min && exp <= Traits::exp_max);
  assert(significand <= Traits::significand_mask);
  Int result = (Int(Traits::exp_bias + exp) << Traits::exp_shift) | significand;
  if (sign == Sign::Minus) {
    result |= Traits::signbit;
  }
  return Bitcast<T>(result);
}

template <typename T>
auto MakeInfinity(Sign sign) -> T {
  return MakeFloat<T>(sign, FloatTraits<T>::exp_max, 0);
}

template <typename T>
auto MakeNan(Sign sign) -> T {
  return MakeFloat<T>(sign, FloatTraits<T>::exp_max,
                      FloatTraits<T>::canonical_nan);
}

template <typename T>
auto MakeNanPayload(Sign sign, typename FloatTraits<T>::Int payload) -> T {
  assert(payload != 0);  // 0 payload is used for infinity.
  return MakeFloat<T>(sign, FloatTraits<T>::exp_max, payload);
}

template <typename T>
auto StrToFloat(LiteralInfo info, SpanU8 span) -> optional<T> {
  switch (info.kind) {
    case LiteralKind::Normal: {
      std::vector<u8> vec;
      RemoveUnderscores(span, vec);  // Always need to copy, to null-terminate.
      vec.push_back(0);
      return ParseFloat<T>(vec);
    }

    case LiteralKind::Nan:
      return MakeNan<T>(info.sign);

    case LiteralKind::NanPayload: {
      using Traits = FloatTraits<T>;
      using Int = typename Traits::Int;
      RemoveSign(span, info.sign);
      auto payload = ParseInteger<Int, 16>(span.subspan(6));  // Skip "nan:0x".
      if (!payload || *payload > Traits::significand_mask) {
        return nullopt;
      }
      return MakeNanPayload<T>(info.sign, *payload);
    }

    case LiteralKind::Infinity:
      return MakeInfinity<T>(info.sign);
  }
}

}  // namespace text
}  // namespace wasp

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
#include <array>
#include <cassert>
#include <charconv>
#include <limits>

#include "third_party/gdtoa/gdtoa.h"
#include "wasp/base/bitcast.h"
#include "wasp/base/buffer.h"
#include "wasp/base/macros.h"

namespace wasp::text {

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
  static_assert(!std::is_signed_v<T>, "T must be unsigned");
  if (info.base == Base::Decimal) {
    return ParseInteger<T, 10>(span);
  } else {
#ifndef NDEBUG
    auto is_hex_prefix = [](SpanU8 span) {
      return span.size() > 2 && span[0] == '0' &&
             (span[1] == 'x' || span[1] == 'X');
    };
#endif

    assert(info.base == Base::Hex && is_hex_prefix(span));
    return ParseInteger<T, 16>(span.subspan(2));
  }
}

inline void RemoveSign(SpanU8& span, Sign sign) {
  if (sign != Sign::None) {
    span.remove_prefix(1);  // Remove + or -.
  }
}

template <typename T>
auto StrToInt(LiteralInfo info, SpanU8 span) -> optional<T> {
  using U = std::make_unsigned_t<T>;
  using S = std::make_signed_t<T>;

  RemoveSign(span, info.sign);

  auto value_opt = StrToNat<U>(info, span);
  if (!value_opt) {
    return nullopt;
  }
  U value = *value_opt;

  if (info.sign == Sign::Minus) {
    // The signed range is [-2**N, 2**N-1], so the maximum is larger for
    // negative numbers than positive numbers.
    auto max = U{std::numeric_limits<S>::max()} + 1;
    if (value > max) {
      return nullopt;
    }
    value = ~value + 1;  // ~N + 1 is equivalent to -N.
  }
  return value;
}

inline void RemoveUnderscores(SpanU8 span, Buffer& out) {
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
  static constexpr int exp_bias = 127;
  static constexpr int exp_min = -exp_bias;
  static constexpr int exp_max = 128;
  static constexpr Int significand_mask = 0x7f'ffff;
  static constexpr Int exp_mask = 0x7f80'0000;
  static constexpr Int canonical_nan = 0x40'0000;
};

template <>
struct FloatTraits<f64> {
  using Int = u64;
  static constexpr Int signbit = 0x8000'0000'0000'0000ull;
  static constexpr int exp_shift = 52;
  static constexpr int exp_bias = 1023;
  static constexpr int exp_min = -exp_bias;
  static constexpr int exp_max = 1024;
  static constexpr Int significand_mask = 0xf'ffff'ffff'ffffull;
  static constexpr Int exp_mask = 0x7ff0'0000'0000'0000ull;
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
      Buffer vec;
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
      if (!payload || *payload == 0 || *payload > Traits::significand_mask) {
        return nullopt;
      }
      return MakeNanPayload<T>(info.sign, *payload);
    }

    case LiteralKind::Infinity:
      return MakeInfinity<T>(info.sign);

  }
  return nullopt;
}

template <typename T>
auto NatToStr(T value, Base base) -> std::string {
  static_assert(!std::is_signed_v<T>, "T must be unsigned");
  // 2**64-1 = dec 18446744073709551614 (20 chars)
  // 2**64-1 = hex 0xffffffffffffffff   (18 chars)
  constexpr size_t max_chars = 20;
  std::array<char, max_chars> buffer;
  char* begin = buffer.data();
  char* end = buffer.data() + buffer.size();

  if (base == Base::Decimal) {
    begin = std::to_chars(begin, end, value).ptr;
  } else {
    *begin++ = '0';
    *begin++ = 'x';
    begin = std::to_chars(begin, end, value, 16).ptr;
  }
  return std::string(buffer.data(), begin);
}

template <typename T>
auto IntToStr(T value, Base base) -> std::string {
  using U = std::make_unsigned_t<T>;
  U unsignedval = U(value);
  constexpr U signbit = U(1) << (sizeof(U) * 8 - 1);

  // +2**63-1 = dec 9223372036854775807  (19 chars)
  // -2**63-1 = dec -9223372036854775807 (20 chars)
  // +2**63-1 = hex 0x7fffffffffffffff   (18 chars)
  // -2**63-1 = hex -0x7fffffffffffffff  (19 chars)
  constexpr size_t max_chars = 20;
  std::array<char, max_chars> buffer;
  char* begin = buffer.data();
  char* end = buffer.data() + buffer.size();

  if (unsignedval & signbit) {
    *begin++ = '-';
    unsignedval = ~unsignedval + 1;
  }

  if (base == Base::Decimal) {
    begin = std::to_chars(begin, end, unsignedval).ptr;
  } else {
    *begin++ = '0';
    *begin++ = 'x';
    begin = std::to_chars(begin, end, unsignedval, 16).ptr;
  }
  return std::string(buffer.data(), begin);
}

template <typename T>
struct FloatInfo {
  Sign sign;
  LiteralKind kind;
  typename FloatTraits<T>::Int payload;  // Set when kind == NanPayload.
};

template <typename T>
auto ClassifyFloat(T value) -> FloatInfo<T> {
  using Traits = FloatTraits<T>;
  using Int = typename Traits::Int;

  Int bits = Bitcast<Int>(value);

  FloatInfo<T> info{};
  info.sign = (bits & Traits::signbit) ? Sign::Minus : Sign::Plus;

  if ((bits & Traits::exp_mask) == Traits::exp_mask) {
    // NaN or infinity.
    Int sig_bits = bits & Traits::significand_mask;
    if (sig_bits == 0) {
      info.kind = LiteralKind::Infinity;
    } else if (sig_bits == Traits::canonical_nan) {
      info.kind = LiteralKind::Nan;
    } else {
      info.kind = LiteralKind::NanPayload;
      info.payload = sig_bits;
    }
  } else {
    // Normal.
    info.kind = LiteralKind::Normal;
  }
  return info;
}

template <typename T>
auto GFmt(T value, char* first, char* last) -> char*;

template <>
inline auto GFmt<f32>(f32 value, char* first, char* last) -> char* {
  return g_ffmt(first, &value, 0, static_cast<unsigned>(last - first));
}

template <>
inline auto GFmt<f64>(f64 value, char* first, char* last) -> char* {
  return g_dfmt(first, &value, 0, static_cast<unsigned>(last - first));
}

template <typename T>
auto FloatToStr(T value, Base base) -> std::string {
  // Not sure exactly how many characters are needed, but this should be enough.
  constexpr size_t max_chars = 40;
  std::array<char, max_chars> buffer;
  char* begin = buffer.data();
  char* end = buffer.data() + buffer.size();

  auto info = ClassifyFloat(value);
  if (info.kind != LiteralKind::Normal) {
    if (info.sign == Sign::Minus) {
      *begin++ = '-';
    }

    string_view keyword;
    switch (info.kind) {
      case LiteralKind::Nan:        keyword = "nan"; break;
      case LiteralKind::NanPayload: keyword = "nan:0x"; break;
      case LiteralKind::Infinity:   keyword = "inf"; break;
      case LiteralKind::Normal:
        WASP_UNREACHABLE();
    }
    begin = std::copy(keyword.begin(), keyword.end(), begin);

    if (info.kind == LiteralKind::NanPayload) {
      begin = std::to_chars(begin, end, info.payload, 16).ptr;
    }
  } else {
    if (base == Base::Decimal) {
      begin = GFmt(value, begin, end);
    } else {
      // Hex.
      using Traits = FloatTraits<T>;
      using Int = typename Traits::Int;

      if (info.sign == Sign::Minus) {
        *begin++ = '-';
      }
      *begin++ = '0';
      *begin++ = 'x';

      Int bits = Bitcast<Int>(value);
      Int sig = bits & Traits::significand_mask;
      int exp = int((bits & ~Traits::signbit) >> Traits::exp_shift) -
                Traits::exp_bias;

      if (exp != Traits::exp_min) {
        // Not subnormal, so include implicit 1 in mantissa.
        sig |= Traits::significand_mask + 1;
      } else {
        exp++;
      }

      // Remove trailing zeroes in mantissa.
      while ((sig & 1) == 0) {
        sig >>= 1;
        exp++;
      }

      begin = std::to_chars(begin, end, sig, 16).ptr;
      *begin++ = 'p';
      begin = std::to_chars(begin, end, exp - Traits::exp_shift).ptr;
    }
  }
  return std::string(buffer.data(), begin);
}

}  // namespace wasp::text

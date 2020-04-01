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

#include "third_party/gdtoa/gdtoa.h"
#include "wasp/base/bitcast.h"

namespace wasp {
namespace text {

namespace {

void RemoveUnderscores(SpanU8 span, std::vector<u8>& out) {
  std::copy_if(span.begin(), span.end(), std::back_inserter(out),
               [](u8 c) { return c != '_'; });
}

struct FixNum {
  explicit FixNum(LiteralInfo info, SpanU8 orig) : info{info}, span{orig} {}

  size_t size() const { return span.size(); }
  const char* begin() const { return ToStringView(span).begin(); }
  const char* end() const { return ToStringView(span).end(); }

  bool FixUnderscores() {
    if (info.has_underscores == HasUnderscores::Yes) {
      RemoveUnderscores(span, vec);
      span = vec;
      return true;
    } else {
      return false;
    }
  }

  LiteralInfo info;
  SpanU8 span;
  std::vector<u8> vec;
};

struct FixNat : FixNum {
  explicit FixNat(LiteralInfo info, SpanU8 orig) : FixNum{info, orig} {
    FixUnderscores();
  }
};

struct FixHexNat : FixNum {
  explicit FixHexNat(LiteralInfo info, SpanU8 orig) : FixNum{info, orig} {
    FixUnderscores();
    remove_prefix(&span, 2);  // Skip "0x".
  }
};

struct FixInt : FixNum {
  explicit FixInt(LiteralInfo info, SpanU8 orig) : FixNum(info, orig) {
    FixUnderscores();
    assert(orig.size() >= 1);
    if (info.sign == Sign::Plus) {
      remove_prefix(&span, 1);  // Skip "+", not allowed by std::from_chars.
    }
  }
};

struct FixHexInt : FixNum {
  explicit FixHexInt(LiteralInfo info, SpanU8 orig) : FixNum(info, orig) {
    assert(orig.size() >= 1);
    switch (info.sign) {
      case Sign::Plus:
        FixUnderscores();
        remove_prefix(&span, 3);  // Skip "+0x", not allowed by std::from_chars.
        break;

      case Sign::Minus:
        // Have to remove "-0x" and replace with "-", but need to keep the "-"
        // at the beginning.
        vec.push_back('-');
        RemoveUnderscores(orig.subspan(3), vec);
        span = vec;
        break;

      default:
        FixUnderscores();
        remove_prefix(&span, 2);  // Skip "0x".
        break;
    }
  }
};

struct FixFloat : FixNum {
  explicit FixFloat(LiteralInfo info, SpanU8 orig) : FixNum(info, orig) {
    if (!FixUnderscores()) {
      // Copy to vec to null-terminate.
      std::copy(orig.begin(), orig.end(), std::back_inserter(vec));
    }
    // Null-terminate.
    vec.push_back(0);
    span = vec;
  }
};

}  // namespace

template <typename T>
auto StrToNat(LiteralInfo info, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (info.base == Base::Decimal) {
      FixNat str{info, orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(info.base == Base::Hex);
      FixHexNat str{info, orig};
      return std::from_chars(str.begin(), str.end(), value, 16);
    }
  })();

  if (result.ec != std::errc{}) {
    return {};
  }
  return MakeAt(orig, value);
}

template <typename T>
auto StrToInt(LiteralInfo info, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (info.base == Base::Decimal) {
      FixInt str{info, orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(info.base == Base::Hex);
      FixHexInt str{info, orig};
      return std::from_chars(str.begin(), str.end(), value, 16);
    }
  })();

  if (result.ec != std::errc{}) {
    return {};
  }
  return MakeAt(orig, value);
}

template <typename T>
auto StrTo(const char* str, char** end, T* value) -> int;

template <>
auto StrTo<f32>(const char* str, char** end, f32* value) -> int {
  return strtorf(str, end, FPI_Round_near, value);
}

template <>
auto StrTo<f64>(const char* str, char** end, f64* value) -> int {
  return strtord(str, end, FPI_Round_near, value);
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
auto StrToFloat(LiteralInfo info, SpanU8 orig) -> OptAt<T> {
  switch (info.kind) {
    case LiteralKind::Normal: {
      T value;
      FixFloat str{info, orig};
      int result = StrTo(str.begin(), nullptr, &value);
      if ((result & STRTOG_Retmask) == STRTOG_NoNumber ||
          (result & STRTOG_Overflow) != 0) {
        return {};
      }
      return MakeAt(orig, value);
    }

    case LiteralKind::Nan:
      return MakeAt(orig, MakeNan<T>(info.sign));

    case LiteralKind::NanPayload: {
      using Traits = FloatTraits<T>;
      typename Traits::Int payload;
      if (info.sign != Sign::None) {
        remove_prefix(&orig, 1);  // Remove sign.
      }
      FixHexNat str{info, orig.subspan(4)};  // Skip "nan:".
      std::from_chars_result result =
          std::from_chars(str.begin(), str.end(), payload, 16);
      if (result.ec != std::errc{}) {
        return {};
      }
      if (payload == 0 || payload > Traits::significand_mask) {
        return {};
      }
      return MakeAt(orig, MakeNanPayload<T>(info.sign, payload));
    }

    case LiteralKind::Infinity:
      return MakeAt(orig, MakeInfinity<T>(info.sign));
  }
}


}  // namespace text
}  // namespace wasp

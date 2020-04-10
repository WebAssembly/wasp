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

#include "wasp/text/numeric.h"

#include <cstring>

#include "gtest/gtest.h"
#include "wasp/base/bitcast.h"

using namespace ::wasp;
using namespace ::wasp::text;

using LI = LiteralInfo;
using HU = HasUnderscores;

TEST(NumericTest, StrToNat_u8) {
  struct {
    SpanU8 span;
    LiteralInfo info;
    u8 value;
  } tests[] = {
      {"0"_su8, LI::Nat(HU::No), 0},
      {"1"_su8, LI::Nat(HU::No), 1},
      {"29"_su8, LI::Nat(HU::No), 29},
      {"38"_su8, LI::Nat(HU::No), 38},
      {"167"_su8, LI::Nat(HU::No), 167},
      {"245"_su8, LI::Nat(HU::No), 245},
      {"255"_su8, LI::Nat(HU::No), 255},
      {"0_1_2"_su8, LI::Nat(HU::Yes), 12},
      {"1_34"_su8, LI::Nat(HU::Yes), 134},
      {"24_8"_su8, LI::Nat(HU::Yes), 248},

      {"0x1"_su8, LI::HexNat(HU::No), 0x01},
      {"0x23"_su8, LI::HexNat(HU::No), 0x23},
      {"0x45"_su8, LI::HexNat(HU::No), 0x45},
      {"0x67"_su8, LI::HexNat(HU::No), 0x67},
      {"0x89"_su8, LI::HexNat(HU::No), 0x89},
      {"0xAb"_su8, LI::HexNat(HU::No), 0xab},
      {"0xcD"_su8, LI::HexNat(HU::No), 0xcd},
      {"0xEf"_su8, LI::HexNat(HU::No), 0xef},
      {"0xff"_su8, LI::HexNat(HU::No), 0xff},
      {"0x0_0_0_0"_su8, LI::HexNat(HU::Yes), 0},
      {"0x0_1_1"_su8, LI::HexNat(HU::Yes), 17},
      {"0xf_f"_su8, LI::HexNat(HU::Yes), 255},
  };
  for (auto test : tests) {
    EXPECT_EQ(test.value, StrToNat<u8>(test.info, test.span));
  }
}

TEST(NumericTest, StrToNat_u16) {
  struct {
    SpanU8 span;
    LiteralInfo info;
    u16 value;
  } tests[] = {
      {"0"_su8, LI::Nat(HU::No), 0},
      {"1"_su8, LI::Nat(HU::No), 1},
      {"23"_su8, LI::Nat(HU::No), 23},
      {"345"_su8, LI::Nat(HU::No), 345},
      {"4567"_su8, LI::Nat(HU::No), 4567},
      {"56789"_su8, LI::Nat(HU::No), 56789},
      {"65535"_su8, LI::Nat(HU::No), 65535},
      {"0_0"_su8, LI::Nat(HU::Yes), 0},
      {"0_0_1"_su8, LI::Nat(HU::Yes), 1},
      {"2_3"_su8, LI::Nat(HU::Yes), 23},
      {"34_5"_su8, LI::Nat(HU::Yes), 345},
      {"4_5_6_7"_su8, LI::Nat(HU::Yes), 4567},
      {"5678_9"_su8, LI::Nat(HU::Yes), 56789},

      {"0x12"_su8, LI::HexNat(HU::No), 0x12},
      {"0x345"_su8, LI::HexNat(HU::No), 0x345},
      {"0x6789"_su8, LI::HexNat(HU::No), 0x6789},
      {"0xAbcD"_su8, LI::HexNat(HU::No), 0xabcd},
      {"0xEf01"_su8, LI::HexNat(HU::No), 0xef01},
      {"0x0_1_2"_su8, LI::HexNat(HU::Yes), 0x12},
      {"0x34_5"_su8, LI::HexNat(HU::Yes), 0x345},
      {"0x6_78_9"_su8, LI::HexNat(HU::Yes), 0x6789},
      {"0xaB_cD"_su8, LI::HexNat(HU::Yes), 0xabcd},
      {"0xe_F_01"_su8, LI::HexNat(HU::Yes), 0xef01},
  };
  for (auto test : tests) {
    EXPECT_EQ(test.value, StrToNat<u16>(test.info, test.span));
  }
}

TEST(NumericTest, StrToNat_u32) {
  struct {
    SpanU8 span;
    LiteralInfo info;
    u32 value;
  } tests[] = {
      {"0"_su8, LI::Nat(HU::No), 0},
      {"12"_su8, LI::Nat(HU::No), 12},
      {"2345"_su8, LI::Nat(HU::No), 2345},
      {"345678"_su8, LI::Nat(HU::No), 345678},
      {"45678901"_su8, LI::Nat(HU::No), 45678901},
      {"3456789012"_su8, LI::Nat(HU::No), 3456789012},
      {"4294967295"_su8, LI::Nat(HU::No), 4294967295},
      {"1_2"_su8, LI::Nat(HU::Yes), 12},
      {"2_34_5"_su8, LI::Nat(HU::Yes), 2345},
      {"34_56_78"_su8, LI::Nat(HU::Yes), 345678},
      {"4567_8901"_su8, LI::Nat(HU::Yes), 45678901},
      {"345_6_789_012"_su8, LI::Nat(HU::Yes), 3456789012},
      {"4_294_967_295"_su8, LI::Nat(HU::Yes), 4294967295},

      {"0x123"_su8, LI::HexNat(HU::No), 0x123},
      {"0x234567"_su8, LI::HexNat(HU::No), 0x234567},
      {"0x3456789a"_su8, LI::HexNat(HU::No), 0x3456789a},
      {"0x89abcdef"_su8, LI::HexNat(HU::No), 0x89abcdef},
      {"0xffffffff"_su8, LI::HexNat(HU::No), 0xffffffff},
      {"0x1_23"_su8, LI::HexNat(HU::Yes), 0x123},
      {"0x23_45_67"_su8, LI::HexNat(HU::Yes), 0x234567},
      {"0x345_678_9a"_su8, LI::HexNat(HU::Yes), 0x3456789a},
      {"0x8_9ab_cdef"_su8, LI::HexNat(HU::Yes), 0x89abcdef},
      {"0xff_ff_ff_ff"_su8, LI::HexNat(HU::Yes), 0xffffffff},
  };
  for (auto test : tests) {
    EXPECT_EQ(test.value, StrToNat<u32>(test.info, test.span));
  }
}

TEST(NumericTest, StrToInt_s32) {
  struct {
    SpanU8 span;
    LiteralInfo info;
    s32 value;
  } tests[] = {
      {"0"_su8, LI::Number(Sign::None, HU::No), 0},
      {"12"_su8, LI::Number(Sign::None, HU::No), 12},
      {"2345"_su8, LI::Number(Sign::None, HU::No), 2345},
      {"345678"_su8, LI::Number(Sign::None, HU::No), 345678},
      {"45678901"_su8, LI::Number(Sign::None, HU::No), 45678901},
      {"2147483647"_su8, LI::Number(Sign::None, HU::No), 2147483647},
      {"1_2"_su8, LI::Number(Sign::None, HU::Yes), 12},
      {"2_34_5"_su8, LI::Number(Sign::None, HU::Yes), 2345},
      {"34_56_78"_su8, LI::Number(Sign::None, HU::Yes), 345678},
      {"4567_8901"_su8, LI::Number(Sign::None, HU::Yes), 45678901},
      {"2_147_483_647"_su8, LI::Number(Sign::None, HU::Yes), 2147483647},

      {"+0"_su8, LI::Number(Sign::Plus, HU::No), 0},
      {"+12"_su8, LI::Number(Sign::Plus, HU::No), 12},
      {"+2345"_su8, LI::Number(Sign::Plus, HU::No), 2345},
      {"+345678"_su8, LI::Number(Sign::Plus, HU::No), 345678},
      {"+45678901"_su8, LI::Number(Sign::Plus, HU::No), 45678901},
      {"+2147483647"_su8, LI::Number(Sign::Plus, HU::No), 2147483647},
      {"+1_2"_su8, LI::Number(Sign::Plus, HU::Yes), 12},
      {"+2_34_5"_su8, LI::Number(Sign::Plus, HU::Yes), 2345},
      {"+34_56_78"_su8, LI::Number(Sign::Plus, HU::Yes), 345678},
      {"+4567_8901"_su8, LI::Number(Sign::Plus, HU::Yes), 45678901},
      {"+2_147_483_647"_su8, LI::Number(Sign::Plus, HU::Yes), 2147483647},

      {"-0"_su8, LI::Number(Sign::Minus, HU::No), 0},
      {"-12"_su8, LI::Number(Sign::Minus, HU::No), -12},
      {"-2345"_su8, LI::Number(Sign::Minus, HU::No), -2345},
      {"-345678"_su8, LI::Number(Sign::Minus, HU::No), -345678},
      {"-45678901"_su8, LI::Number(Sign::Minus, HU::No), -45678901},
      {"-2147483648"_su8, LI::Number(Sign::Minus, HU::No), -2147483648},
      {"-1_2"_su8, LI::Number(Sign::Minus, HU::Yes), -12},
      {"-2_34_5"_su8, LI::Number(Sign::Minus, HU::Yes), -2345},
      {"-34_56_78"_su8, LI::Number(Sign::Minus, HU::Yes), -345678},
      {"-4567_8901"_su8, LI::Number(Sign::Minus, HU::Yes), -45678901},
      {"-2_147_483_648"_su8, LI::Number(Sign::Minus, HU::Yes), -2147483648},

      {"0x123"_su8, LI::HexNumber(Sign::None, HU::No), 0x123},
      {"0x234567"_su8, LI::HexNumber(Sign::None, HU::No), 0x234567},
      {"0x3456789a"_su8, LI::HexNumber(Sign::None, HU::No), 0x3456789a},
      {"0x789abcde"_su8, LI::HexNumber(Sign::None, HU::No), 0x789abcde},
      {"0x7fffffff"_su8, LI::HexNumber(Sign::None, HU::No), 0x7fffffff},
      {"0x1_23"_su8, LI::HexNumber(Sign::None, HU::Yes), 0x123},
      {"0x23_45_67"_su8, LI::HexNumber(Sign::None, HU::Yes), 0x234567},
      {"0x345_678_9a"_su8, LI::HexNumber(Sign::None, HU::Yes), 0x3456789a},
      {"0x7_89a_bcde"_su8, LI::HexNumber(Sign::None, HU::Yes), 0x789abcde},
      {"0x7f_ff_ff_ff"_su8, LI::HexNumber(Sign::None, HU::Yes), 0x7fffffff},

      {"+0x123"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x123},
      {"+0x234567"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x234567},
      {"+0x3456789a"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x3456789a},
      {"+0x789abcde"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x789abcde},
      {"+0x7fffffff"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x7fffffff},
      {"+0x1_23"_su8, LI::HexNumber(Sign::Plus, HU::Yes), 0x123},
      {"+0x23_45_67"_su8, LI::HexNumber(Sign::Plus, HU::Yes), 0x234567},
      {"+0x345_678_9a"_su8, LI::HexNumber(Sign::Plus, HU::Yes), 0x3456789a},
      {"+0x7_89a_bcde"_su8, LI::HexNumber(Sign::Plus, HU::Yes), 0x789abcde},
      {"+0x7f_ff_ff_ff"_su8, LI::HexNumber(Sign::Plus, HU::Yes), 0x7fffffff},

      {"-0x123"_su8, LI::HexNumber(Sign::Minus, HU::No), -0x123},
      {"-0x234567"_su8, LI::HexNumber(Sign::Minus, HU::No), -0x234567},
      {"-0x3456789a"_su8, LI::HexNumber(Sign::Minus, HU::No), -0x3456789a},
      {"-0x789abcde"_su8, LI::HexNumber(Sign::Minus, HU::No), -0x789abcde},
      {"-0x80000000"_su8, LI::HexNumber(Sign::Minus, HU::No), -0x7fffffff - 1},
      {"-0x1_23"_su8, LI::HexNumber(Sign::Minus, HU::Yes), -0x123},
      {"-0x23_45_67"_su8, LI::HexNumber(Sign::Minus, HU::Yes), -0x234567},
      {"-0x345_678_9a"_su8, LI::HexNumber(Sign::Minus, HU::Yes), -0x3456789a},
      {"-0x7_89a_bcde"_su8, LI::HexNumber(Sign::Minus, HU::Yes), -0x789abcde},
      {"-0x80_00_00_00"_su8, LI::HexNumber(Sign::Minus, HU::Yes),
       -0x7fffffff - 1},
  };
  for (auto test : tests) {
    EXPECT_EQ(test.value, StrToInt<s32>(test.info, test.span));
  }
}

template <typename Float, typename Int>
void ExpectFloat(SpanU8 span, LiteralInfo info, Int expected) {
  static_assert(sizeof(Float) == sizeof(Int), "size mismatch");
  auto value_opt = StrToFloat<Float>(info, span);
  ASSERT_TRUE(value_opt.has_value());
  Int actual = Bitcast<Int>(*value_opt);
  EXPECT_EQ(expected, actual) << "expected " << expected << " got " << actual
                              << " (\"" << ToStringView(span) << "\")";
}

TEST(NumericTest, StrToFloat_f32) {
  struct {
    SpanU8 span;
    LiteralInfo info;
    u32 value_bits;
  } tests[] = {
      {"0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},
      {"0.0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0.0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0.0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},
      {"0.0e0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0.0e0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0.0e0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},
      {"0.0e+0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0.0e+0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0.0e+0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},
      {"0.0e-0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0.0e-0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0.0e-0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},
      {"0.0E0"_su8, LI::Number(Sign::None, HU::No), 0x00000000},
      {"+0.0E+0"_su8, LI::Number(Sign::Plus, HU::No), 0x00000000},
      {"-0.0E-0"_su8, LI::Number(Sign::Minus, HU::No), 0x80000000},

      {"1234.5"_su8, LI::Number(Sign::None, HU::No), 0x449a5000},
      {"+1234.5"_su8, LI::Number(Sign::Plus, HU::No), 0x449a5000},
      {"-1234.5"_su8, LI::Number(Sign::Minus, HU::No), 0xc49a5000},
      {"1.5e1"_su8, LI::Number(Sign::None, HU::No), 0x41700000},
      {"+1.5e1"_su8, LI::Number(Sign::Plus, HU::No), 0x41700000},
      {"-1.5e1"_su8, LI::Number(Sign::Minus, HU::No), 0xc1700000},
      {"1.4013e-45"_su8, LI::Number(Sign::None, HU::No), 0x00000001},
      {"+1.4013e-45"_su8, LI::Number(Sign::Plus, HU::No), 0x00000001},
      {"-1.4013e-45"_su8, LI::Number(Sign::Minus, HU::No), 0x80000001},
      {"1.1754944e-38"_su8, LI::Number(Sign::None, HU::No), 0x00800000},
      {"+1.1754944e-38"_su8, LI::Number(Sign::Plus, HU::No), 0x00800000},
      {"-1.1754944e-38"_su8, LI::Number(Sign::Minus, HU::No), 0x80800000},
      {"1.1754942e-38"_su8, LI::Number(Sign::None, HU::No), 0x007fffff},
      {"+1.1754942e-38"_su8, LI::Number(Sign::Plus, HU::No), 0x007fffff},
      {"-1.1754942e-38"_su8, LI::Number(Sign::Minus, HU::No), 0x807fffff},
      {"3.4028234e+38"_su8, LI::Number(Sign::None, HU::No), 0x7f7fffff},
      {"+3.4028234e+38"_su8, LI::Number(Sign::Plus, HU::No), 0x7f7fffff},
      {"-3.4028234e+38"_su8, LI::Number(Sign::Minus, HU::No), 0xff7fffff},

      {"0x1.5"_su8, LI::HexNumber(Sign::None, HU::No), 0x3fa80000},
      {"+0x1.5"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x3fa80000},
      {"-0x1.5"_su8, LI::HexNumber(Sign::Minus, HU::No), 0xbfa80000},
      {"0x9.a5p+7"_su8, LI::HexNumber(Sign::None, HU::No), 0x449a5000},
      {"+0x9.a5p+7"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x449a5000},
      {"-0x9.a5p+7"_su8, LI::HexNumber(Sign::Minus, HU::No), 0xc49a5000},
      {"0x9.a5P7"_su8, LI::HexNumber(Sign::None, HU::No), 0x449a5000},
      {"+0x9.a5P+7"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x449a5000},
      {"-0x9.a5P+7"_su8, LI::HexNumber(Sign::Minus, HU::No), 0xc49a5000},
      {"0x1p-149"_su8, LI::HexNumber(Sign::None, HU::No), 0x00000001},
      {"+0x1p-149"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x00000001},
      {"-0x1p-149"_su8, LI::HexNumber(Sign::Minus, HU::No), 0x80000001},
      {"0x1p-126"_su8, LI::HexNumber(Sign::None, HU::No), 0x00800000},
      {"+0x1p-126"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x00800000},
      {"-0x1p-126"_su8, LI::HexNumber(Sign::Minus, HU::No), 0x80800000},
      {"0x1.fffffep+127"_su8, LI::HexNumber(Sign::None, HU::No), 0x7f7fffff},
      {"+0x1.fffffep+127"_su8, LI::HexNumber(Sign::Plus, HU::No), 0x7f7fffff},
      {"-0x1.fffffep+127"_su8, LI::HexNumber(Sign::Minus, HU::No), 0xff7fffff},

      {"0_0_0_0"_su8, LI::Number(Sign::None, HU::Yes), 0x00000000},
      {"00_0.0_00"_su8, LI::Number(Sign::None, HU::Yes), 0x00000000},
      {"0_0.0_0e0_0"_su8, LI::Number(Sign::None, HU::Yes), 0x00000000},
      {"0.00_0e00_00"_su8, LI::Number(Sign::None, HU::Yes), 0x00000000},
      {"0x0_0.0_0p0_0"_su8, LI::Number(Sign::None, HU::Yes), 0x00000000},

      {"inf"_su8, LI::Infinity(Sign::None), 0x7f800000},
      {"+inf"_su8, LI::Infinity(Sign::Plus), 0x7f800000},
      {"-inf"_su8, LI::Infinity(Sign::Minus), 0xff800000},

      {"nan"_su8, LI::Nan(Sign::None), 0x7fc00000},
      {"+nan"_su8, LI::Nan(Sign::Plus), 0x7fc00000},
      {"-nan"_su8, LI::Nan(Sign::Minus), 0xffc00000},

      {"nan:0x1"_su8, LI::NanPayload(Sign::None, HU::No), 0x7f800001},
      {"+nan:0x1"_su8, LI::NanPayload(Sign::Plus, HU::No), 0x7f800001},
      {"-nan:0x1"_su8, LI::NanPayload(Sign::Minus, HU::No), 0xff800001},

      {"nan:0x123456"_su8, LI::NanPayload(Sign::None, HU::No), 0x7f923456},
      {"+nan:0x123456"_su8, LI::NanPayload(Sign::Plus, HU::No), 0x7f923456},
      {"-nan:0x123456"_su8, LI::NanPayload(Sign::Minus, HU::No), 0xff923456},
  };
  for (auto test : tests) {
    ExpectFloat<f32, u32>(test.span, test.info, test.value_bits);
  }
}

TEST(NumericTest, StrToFloat_f64) {
  const auto none = LI::Number(Sign::None, HU::No);
  const auto plus = LI::Number(Sign::Plus, HU::No);
  const auto minus = LI::Number(Sign::Minus, HU::No);
  const auto hex_none = LI::HexNumber(Sign::None, HU::No);
  const auto hex_plus = LI::HexNumber(Sign::Plus, HU::No);
  const auto hex_minus = LI::HexNumber(Sign::Minus, HU::No);
  const auto none_hu = LI::Number(Sign::None, HU::Yes);

  struct {
    SpanU8 span;
    LiteralInfo info;
    u64 value_bits;
  } tests[] = {
      {"0"_su8, none, 0x00000000'00000000ull},
      {"+0"_su8, plus, 0x00000000'00000000ull},
      {"-0"_su8, minus, 0x80000000'00000000ull},
      {"0.0"_su8, none, 0x00000000'00000000ull},
      {"+0.0"_su8, plus, 0x00000000'00000000ull},
      {"-0.0"_su8, minus, 0x80000000'00000000ull},
      {"0.0e0"_su8, none, 0x00000000'00000000ull},
      {"+0.0e0"_su8, plus, 0x00000000'00000000ull},
      {"-0.0e0"_su8, minus, 0x80000000'00000000ull},
      {"0.0e+0"_su8, none, 0x00000000'00000000ull},
      {"+0.0e+0"_su8, plus, 0x00000000'00000000ull},
      {"-0.0e+0"_su8, minus, 0x80000000'00000000ull},
      {"0.0e-0"_su8, none, 0x00000000'00000000ull},
      {"+0.0e-0"_su8, plus, 0x00000000'00000000ull},
      {"-0.0e-0"_su8, minus, 0x80000000'00000000ull},
      {"0.0E0"_su8, none, 0x00000000'00000000ull},
      {"+0.0E+0"_su8, plus, 0x00000000'00000000ull},
      {"-0.0E-0"_su8, minus, 0x80000000'00000000ull},

      {"1234.5"_su8, none, 0x40934a00'00000000ull},
      {"+1234.5"_su8, plus, 0x40934a00'00000000ull},
      {"-1234.5"_su8, minus, 0xc0934a00'00000000ull},
      {"1.5e1"_su8, none, 0x402e0000'00000000ull},
      {"+1.5e1"_su8, plus, 0x402e0000'00000000ull},
      {"-1.5e1"_su8, minus, 0xc02e0000'00000000ull},
      {"4.94066e-324"_su8, none, 0x00000000'00000001ull},
      {"+4.94066e-324"_su8, plus, 0x00000000'00000001ull},
      {"-4.94066e-324"_su8, minus, 0x80000000'00000001ull},
      {"2.2250738585072012e-308"_su8, none, 0x00100000'00000000ull},
      {"+2.2250738585072012e-308"_su8, plus, 0x00100000'00000000ull},
      {"-2.2250738585072012e-308"_su8, minus, 0x80100000'00000000ull},
      {"2.2250738585072011e-308"_su8, none, 0x000fffff'ffffffffull},
      {"+2.2250738585072011e-308"_su8, plus, 0x000fffff'ffffffffull},
      {"-2.2250738585072011e-308"_su8, minus, 0x800fffff'ffffffffull},
      {"1.7976931348623157e+308"_su8, none, 0x7fefffff'ffffffffull},
      {"+1.7976931348623157e+308"_su8, plus, 0x7fefffff'ffffffffull},
      {"-1.7976931348623157e+308"_su8, minus, 0xffefffff'ffffffffull},

      {"0x1.5"_su8, hex_none, 0x3ff50000'00000000ull},
      {"+0x1.5"_su8, hex_plus, 0x3ff50000'00000000ull},
      {"-0x1.5"_su8, hex_minus, 0xbff50000'00000000ull},
      {"0x9.a5p+7"_su8, hex_none, 0x40934a00'00000000ull},
      {"+0x9.a5p+7"_su8, hex_plus, 0x40934a00'00000000ull},
      {"-0x9.a5p+7"_su8, hex_minus, 0xc0934a00'00000000ull},
      {"0x9.a5P7"_su8, hex_none, 0x40934a00'00000000ull},
      {"+0x9.a5P+7"_su8, hex_plus, 0x40934a00'00000000ull},
      {"-0x9.a5P+7"_su8, hex_minus, 0xc0934a00'00000000ull},
      {"0x0.0000000000001p-1022"_su8, hex_none, 0x00000000'00000001ull},
      {"+0x0.0000000000001p-1022"_su8, hex_plus, 0x00000000'00000001ull},
      {"-0x0.0000000000001p-1022"_su8, hex_minus, 0x80000000'00000001ull},
      {"0x1p-1022"_su8, hex_none, 0x00100000'00000000ull},
      {"+0x1p-1022"_su8, hex_plus, 0x00100000'00000000ull},
      {"-0x1p-1022"_su8, hex_minus, 0x80100000'00000000ull},
      {"0x0.fffffffffffffp-1022"_su8, hex_none, 0x000fffff'ffffffffull},
      {"+0x0.fffffffffffffp-1022"_su8, hex_plus, 0x000fffff'ffffffffull},
      {"-0x0.fffffffffffffp-1022"_su8, hex_minus, 0x800fffff'ffffffffull},
      {"0x1.fffffffffffffp+1023"_su8, hex_none, 0x7fefffff'ffffffffull},
      {"+0x1.fffffffffffffp+1023"_su8, hex_plus, 0x7fefffff'ffffffffull},
      {"-0x1.fffffffffffffp+1023"_su8, hex_minus, 0xffefffff'ffffffffull},

      {"0_0_0_0"_su8, none_hu, 0x00000000'00000000ull},
      {"00_0.0_00"_su8, none_hu, 0x00000000'00000000ull},
      {"0_0.0_0e0_0"_su8, none_hu, 0x00000000'00000000ull},
      {"0.00_0e00_00"_su8, none_hu, 0x00000000'00000000ull},
      {"0x0_0.0_0p0_0"_su8, none_hu, 0x00000000'00000000ull},

      {"inf"_su8, LI::Infinity(Sign::None), 0x7ff00000'00000000ull},
      {"+inf"_su8, LI::Infinity(Sign::Plus), 0x7ff00000'00000000ull},
      {"-inf"_su8, LI::Infinity(Sign::Minus), 0xfff00000'00000000ull},

      {"nan"_su8, LI::Nan(Sign::None), 0x7ff80000'00000000ull},
      {"+nan"_su8, LI::Nan(Sign::Plus), 0x7ff80000'00000000ull},
      {"-nan"_su8, LI::Nan(Sign::Minus), 0xfff80000'00000000ull},

      {"nan:0x1"_su8, LI::NanPayload(Sign::None, HU::No), 0x7ff00000'00000001ull},
      {"+nan:0x1"_su8, LI::NanPayload(Sign::Plus, HU::No), 0x7ff00000'00000001ull},
      {"-nan:0x1"_su8, LI::NanPayload(Sign::Minus, HU::No), 0xfff00000'00000001ull},

      {"nan:0x123456789abcd"_su8, LI::NanPayload(Sign::None, HU::No), 0x7ff12345'6789abcdull},
      {"+nan:0x123456789abcd"_su8, LI::NanPayload(Sign::Plus, HU::No), 0x7ff12345'6789abcdull},
      {"-nan:0x123456789abcd"_su8, LI::NanPayload(Sign::Minus, HU::No), 0xfff12345'6789abcdull},
  };
  for (auto test : tests) {
    ExpectFloat<f64, u64>(test.span, test.info, test.value_bits);
  }
}

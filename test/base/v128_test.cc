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

#include "wasp/base/v128.h"

#include "gtest/gtest.h"
#include "test/test_utils.h"

using namespace ::wasp;

TEST(V128Test, s64x2) {
  EXPECT_EQ(s64x2({{1, 2}}), v128(s64{1}, s64{2}).as<s64x2>());
}

TEST(V128Test, u64x2) {
  EXPECT_EQ(u64x2({{1, 2}}), v128(u64{1}, u64{2}).as<u64x2>());
}

TEST(V128Test, f64x2) {
  EXPECT_EQ(f64x2({{1, 2}}), v128(f64{1}, f64{2}).as<f64x2>());
}

TEST(V128Test, s32x4) {
  EXPECT_EQ(s32x4({{1, 2, 3, 4}}),
            v128(s32{1}, s32{2}, s32{3}, s32{4}).as<s32x4>());
}

TEST(V128Test, u32x4) {
  EXPECT_EQ(u32x4({{1, 2, 3, 4}}),
            v128(u32{1}, u32{2}, u32{3}, u32{4}).as<u32x4>());
}

TEST(V128Test, f32x4) {
  EXPECT_EQ(f32x4({{1, 2, 3, 4}}),
            v128(f32{1}, f32{2}, f32{3}, f32{4}).as<f32x4>());
}

TEST(V128Test, s16x8) {
  EXPECT_EQ(s16x8({{1, 2, 3, 4, 5, 6, 7, 8}}),
            v128(s16{1}, s16{2}, s16{3}, s16{4}, s16{5}, s16{6}, s16{7}, s16{8})
                .as<s16x8>());
}

TEST(V128Test, u16x8) {
  EXPECT_EQ(u16x8({{1, 2, 3, 4, 5, 6, 7, 8}}),
            v128(u16{1}, u16{2}, u16{3}, u16{4}, u16{5}, u16{6}, u16{7}, u16{8})
                .as<u16x8>());
}

TEST(V128Test, s8x16) {
  EXPECT_EQ(s8x16({{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}}),
            v128(s8{1}, s8{2}, s8{3}, s8{4}, s8{5}, s8{6}, s8{7}, s8{8}, s8{9},
                 s8{10}, s8{11}, s8{12}, s8{13}, s8{14}, s8{15}, s8{16})
                .as<s8x16>());
}

TEST(V128Test, u8x16) {
  EXPECT_EQ(u8x16({{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}}),
            v128(u8{1}, u8{2}, u8{3}, u8{4}, u8{5}, u8{6}, u8{7}, u8{8}, u8{9},
                 u8{10}, u8{11}, u8{12}, u8{13}, u8{14}, u8{15}, u8{16})
                .as<u8x16>());
}

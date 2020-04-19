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

#include <cstring>

#include "wasp/base/hash.h"
#include "wasp/base/operator_eq_ne_macros.h"

namespace wasp {

v128::v128() : v128{u64x2{{0, 0}}} {}

v128::v128(s64 x0, s64 x1) : v128{s64x2{{x0, x1}}} {}

v128::v128(u64 x0, u64 x1) : v128{u64x2{{x0, x1}}} {}

v128::v128(f64 x0, f64 x1) : v128{f64x2{{x0, x1}}} {}

v128::v128(s32 x0, s32 x1, s32 x2, s32 x3) : v128{s32x4{{x0, x1, x2, x3}}} {}

v128::v128(u32 x0, u32 x1, u32 x2, u32 x3) : v128{u32x4{{x0, x1, x2, x3}}} {}

v128::v128(f32 x0, f32 x1, f32 x2, f32 x3) : v128{f32x4{{x0, x1, x2, x3}}} {}

v128::v128(s16 x0, s16 x1, s16 x2, s16 x3, s16 x4, s16 x5, s16 x6, s16 x7)
    : v128{s16x8{{x0, x1, x2, x3, x4, x5, x6, x7}}} {}

v128::v128(u16 x0, u16 x1, u16 x2, u16 x3, u16 x4, u16 x5, u16 x6, u16 x7)
    : v128{u16x8{{x0, x1, x2, x3, x4, x5, x6, x7}}} {}

v128::v128(s8 x0,
           s8 x1,
           s8 x2,
           s8 x3,
           s8 x4,
           s8 x5,
           s8 x6,
           s8 x7,
           s8 x8,
           s8 x9,
           s8 x10,
           s8 x11,
           s8 x12,
           s8 x13,
           s8 x14,
           s8 x15)
    : v128{s8x16{{x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13,
                  x14, x15}}} {}

v128::v128(u8 x0,
           u8 x1,
           u8 x2,
           u8 x3,
           u8 x4,
           u8 x5,
           u8 x6,
           u8 x7,
           u8 x8,
           u8 x9,
           u8 x10,
           u8 x11,
           u8 x12,
           u8 x13,
           u8 x14,
           u8 x15)
    : v128{u8x16{{x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13,
                  x14, x15}}} {}

v128::v128(s64x2 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(u64x2 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(f64x2 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(s32x4 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(u32x4 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(f32x4 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(s16x8 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(u16x8 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(s8x16 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

v128::v128(u8x16 other) {
  static_assert(sizeof(data_) == sizeof(other), "v128 size mismatch");
  memcpy(&data_, &other, sizeof(data_));
}

WASP_OPERATOR_EQ_NE_1(v128, data_)

}  // namespace wasp

namespace std {

size_t hash<::wasp::v128>::operator()(const ::wasp::v128& v) const {
  auto u = v.as<::wasp::u64x2>();
  return ::wasp::HashState::combine(0, u[0], u[1]);
}

}  // namespace std

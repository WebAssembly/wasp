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

#ifndef WASP_BASE_V128_H_
#define WASP_BASE_V128_H_

#include <array>
#include <functional>
#include <type_traits>

#include "wasp/base/types.h"

namespace wasp {

using s64x2 = std::array<s64, 2>;
using u64x2 = std::array<u64, 2>;
using f64x2 = std::array<f64, 2>;
using s32x4 = std::array<s32, 4>;
using u32x4 = std::array<u32, 4>;
using f32x4 = std::array<f32, 4>;
using s16x8 = std::array<s16, 8>;
using u16x8 = std::array<u16, 8>;
using s8x16 = std::array<s8, 16>;
using u8x16 = std::array<u8, 16>;

template <typename T>
struct IsV128Type : std::disjunction<std::is_same<T, s64x2>,
                                     std::is_same<T, u64x2>,
                                     std::is_same<T, f64x2>,
                                     std::is_same<T, s32x4>,
                                     std::is_same<T, u32x4>,
                                     std::is_same<T, f32x4>,
                                     std::is_same<T, s16x8>,
                                     std::is_same<T, u16x8>,
                                     std::is_same<T, s8x16>,
                                     std::is_same<T, u8x16>> {};

template <typename T>
using GetV128Type = std::enable_if_t<IsV128Type<T>::value, T>;

class v128 {
 public:
  explicit v128();
  explicit v128(s64, s64);
  explicit v128(u64, u64);
  explicit v128(f64, f64);
  explicit v128(s32, s32, s32, s32);
  explicit v128(u32, u32, u32, u32);
  explicit v128(f32, f32, f32, f32);
  explicit v128(s16, s16, s16, s16, s16, s16, s16, s16);
  explicit v128(u16, u16, u16, u16, u16, u16, u16, u16);
  explicit v128(s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8, s8);
  explicit v128(u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8);
  explicit v128(s64x2);
  explicit v128(u64x2);
  explicit v128(f64x2);
  explicit v128(s32x4);
  explicit v128(u32x4);
  explicit v128(f32x4);
  explicit v128(s16x8);
  explicit v128(u16x8);
  explicit v128(s8x16);
  explicit v128(u8x16);

  template <typename T>
  GetV128Type<T> as() const;

  friend bool operator==(const v128&, const v128&);
  friend bool operator!=(const v128&, const v128&);

 private:
  u8x16 data_;
};

}  // namespace wasp

namespace std {

template <>
struct hash<::wasp::v128> {
  size_t operator()(const ::wasp::v128&) const;
};

}  // namespace std

#include "wasp/base/v128-inl.h"

#endif  // WASP_BASE_V128_H_

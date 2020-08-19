//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BASE_FORMATTERS_H_
#define WASP_BASE_FORMATTERS_H_

#include <array>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/features.h"
#include "wasp/base/formatter_macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"

namespace wasp {

// We want to be able to overload `operator<<` for types that are not in this
// namespace. We can do that, but the correct `operator<<` function won't be
// found by argument-dependent lookup. To print these values, we can wrap them
// in FormatWrapper, which will forward to `wasp::operator<<`, e.g.:
//
//   my_namespace::MyObject my_value;
//   os << FormatWrapper{my_value};
template <typename T>
struct FormatWrapper {
  T&& contents;
};

template <typename T>
FormatWrapper(T&& contents) -> FormatWrapper<T>;

// At<T>
template <typename T>
std::ostream& operator<<(std::ostream&, const ::wasp::At<T>&);

// Wrapper<T>
template <typename T>
std::ostream& operator<<(std::ostream&, const ::wasp::FormatWrapper<T>&);

// span<const T>
template <typename T>
std::ostream& operator<<(std::ostream&, ::wasp::span<const T>);
template <>
std::ostream& operator<<(std::ostream&, ::wasp::SpanU8);

// std::array<T, N>
template <typename T, size_t N>
std::ostream& operator<<(std::ostream&, const ::std::array<T, N>&);

// std::vector<T>
template <typename T>
std::ostream& operator<<(std::ostream&, const ::std::vector<T>&);

// variant<Ts...>
template <typename... Ts>
std::ostream& operator<<(std::ostream&, const ::wasp::variant<Ts...>&);

// optional<T>
template <typename T>
std::ostream& operator<<(std::ostream&, const ::wasp::optional<T>&);

// nullopt_t
std::ostream& operator<<(std::ostream&, const ::std::nullopt_t&);

WASP_BASE_WASM_ENUMS(WASP_DECLARE_FORMATTER)
WASP_BASE_WASM_STRUCTS(WASP_DECLARE_FORMATTER)

WASP_DECLARE_FORMATTER(v128);
WASP_DECLARE_FORMATTER(Features);
WASP_DECLARE_FORMATTER(monostate);
WASP_DECLARE_FORMATTER(ShuffleImmediate);

}  // namespace wasp

namespace wasp {

template <typename T>
struct VariantName { const char* GetName() const; };

// At<T>
template <typename T>
struct VariantName<At<T>> {
  const char* GetName() const { return VariantName<T>().GetName(); };
};

#define WASP_DEFINE_VARIANT_NAME(Type, Name)     \
  template <>                                    \
  struct VariantName<Type> {                     \
    const char* GetName() const { return Name; } \
  };

WASP_DEFINE_VARIANT_NAME(u8, "u8")
WASP_DEFINE_VARIANT_NAME(u16, "u16")
WASP_DEFINE_VARIANT_NAME(u32, "u32")
WASP_DEFINE_VARIANT_NAME(u64, "u64")
WASP_DEFINE_VARIANT_NAME(s8, "s8")
WASP_DEFINE_VARIANT_NAME(s16, "s16")
WASP_DEFINE_VARIANT_NAME(s32, "s32")
WASP_DEFINE_VARIANT_NAME(s64, "s64")
WASP_DEFINE_VARIANT_NAME(f32, "f32")
WASP_DEFINE_VARIANT_NAME(f64, "f64")

WASP_DEFINE_VARIANT_NAME(v128, "v128")

WASP_DEFINE_VARIANT_NAME(string_view, "string_view")
WASP_DEFINE_VARIANT_NAME(ShuffleImmediate, "shuffle")

// TODO: More base types.

}  // namespace wasp

#include "wasp/base/formatters-inl.h"

#endif // WASP_BASE_FORMATTERS_H_

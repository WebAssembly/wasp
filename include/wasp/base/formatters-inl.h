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

#include "wasp/base/macros.h"

#include <iostream>
#include <type_traits>

namespace wasp {

WASP_DEFINE_VARIANT_NAME(wasp::NumericType, "numeric_type")
WASP_DEFINE_VARIANT_NAME(wasp::ReferenceKind, "reference_kind")
WASP_DEFINE_VARIANT_NAME(wasp::HeapKind, "heap_kind")

template <typename T>
std::ostream& operator<<(std::ostream& os, const ::wasp::At<T>& self) {
  return os << *self;
}

template <typename T>
std::ostream& operator<<(std::ostream& os,
                         const ::wasp::FormatWrapper<T>& self) {
  // Unwrap the value and make sure to use the wasp namespace to look up the
  // output function. This way we can print values in other namespaces, e.g.
  // std.
  return wasp::operator<<(os, self.contents);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, ::wasp::span<const T> self) {
  string_view space = "";
  os << "[";
  for (const auto& x : self) {
    os << space << x;
    space = " ";
  }
  return os << "]";
}

template <typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const ::std::array<T, N>& self) {
  string_view space = "";
  os << "[";
  for (const auto& x : self) {
    // TODO: Never want to print `x` as a char; this breaks ShuffleImmediate
    os << space << x;
    space = " ";
  }
  return os << "]";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const ::std::vector<T>& self) {
  return os << ::wasp::span{self};
}

template <typename... Ts>
std::ostream& operator<<(std::ostream& os, const ::wasp::variant<Ts...>& self) {
  std::visit(
      [&](auto&& arg) {
        using Type = std::remove_cv_t<std::remove_reference_t<decltype(arg)> >;
        if constexpr (std::is_same_v<Type, ::wasp::monostate>) {
          os << "empty";
        } else {
          os << ::wasp::VariantName<Type>().GetName() << " " << arg;
        }
      },
      self);
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const ::wasp::optional<T>& self) {
  if (self.has_value()) {
    os << self.value();
  } else {
    os << "none";
  }
  return os;
}

}  // namespace wasp

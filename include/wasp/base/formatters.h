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

#include <vector>

#include "wasp/base/format.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/v128.h"

namespace fmt {

// Convert from a fmt::basic_memory_buffer to a fmt::string_view. Not sure why
// this conversion was omitted from the fmt library.
template <typename T, std::size_t SIZE, typename Allocator>
string_view to_string_view(const basic_memory_buffer<T, SIZE, Allocator>&);

template <>
struct formatter<::wasp::SpanU8> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::SpanU8&, Ctx&);
};

template <typename T>
struct formatter<::wasp::span<const T>> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::span<const T>, Ctx&);
};

template <typename T>
struct formatter<std::vector<T>> : formatter<::wasp::span<const T>> {
  template <typename Ctx>
  typename Ctx::iterator format(const std::vector<T>&, Ctx&);
};

template <>
struct formatter<::wasp::v128> : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator format(::wasp::v128, Ctx&);
};

}  // namespace fmt

#include "wasp/base/formatters-inl.h"

#endif // WASP_BASE_FORMATTERS_H_

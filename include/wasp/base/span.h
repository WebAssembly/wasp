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

#ifndef WASP_BASE_SPAN_H_
#define WASP_BASE_SPAN_H_

#include <functional>

#include "nonstd/span.hpp"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp {

using nonstd::span;

using nonstd::operator==;
using nonstd::operator!=;
using nonstd::operator<;
using nonstd::operator<=;
using nonstd::operator>;
using nonstd::operator>=;

using span_index_t = nonstd::span_lite::index_t;

constexpr span_index_t dynamic_extent = -1;

template <class T, span_index_t Extent>
void remove_prefix(span<T, Extent>* s, span_index_t offset) {
  *s = s->subspan(offset);
}

using SpanU8 = span<const u8>;
using Location = SpanU8;

// Make SpanU8 from literal string.
inline SpanU8 operator"" _su8(const char* str, size_t N) {
  return SpanU8{reinterpret_cast<const u8*>(str),
                static_cast<SpanU8::index_type>(N)};
}

inline string_view ToStringView(SpanU8 span) {
  return string_view{reinterpret_cast<const char*>(span.data()),
                     static_cast<size_t>(span.size())};
}

}  // namespace wasp

namespace std {

template <>
struct hash<::wasp::SpanU8> {
  size_t operator()(::wasp::SpanU8) const;
};

}

#endif  // WASP_BASE_SPAN_H_

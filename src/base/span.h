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

#include "nonstd/span.hpp"

#include "src/base/types.h"

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
span<T, dynamic_extent> remove_prefix(span<T, Extent> s, span_index_t offset) {
  return s.subspan(offset);
}

using SpanU8 = span<const u8>;

}  // namespace wasp

#endif  // WASP_BASE_SPAN_H_

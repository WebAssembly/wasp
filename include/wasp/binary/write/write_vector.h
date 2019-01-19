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

#ifndef WASP_BINARY_WRITE_WRITE_VECTOR_H_
#define WASP_BINARY_WRITE_WRITE_VECTOR_H_

#include <iterator>

#include "wasp/binary/write/write_u32.h"

namespace wasp {
namespace binary {

template <typename InputIterator, typename OutputIterator>
OutputIterator WriteVector(InputIterator in_begin,
                           InputIterator in_end,
                           OutputIterator out) {
  size_t count = std::distance(in_begin, in_end);
  assert(count < std::numeric_limits<u32>::max());
  out = Write(static_cast<u32>(count), out);
  for (auto it = in_begin; it != in_end; ++it) {
    out = Write(*it, out);
  }
  return out;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_VECTOR_H_

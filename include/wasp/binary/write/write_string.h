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

#ifndef WASP_BINARY_WRITE_WRITE_STRING_H_
#define WASP_BINARY_WRITE_WRITE_STRING_H_

#include <limits>

#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/write/write_u32.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(string_view value, Iterator out, const Features& features) {
  assert(value.size() < std::numeric_limits<u32>::max());
  u32 value_size = value.size();
  out = Write(value_size, out, features);
  return WriteBytes(
      SpanU8{reinterpret_cast<const u8*>(value.data()), value_size}, out,
      features);
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_STRING_H_

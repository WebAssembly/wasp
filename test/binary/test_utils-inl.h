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

#include "wasp/binary/constant_expression.h"
#include "wasp/binary/expression.h"

namespace wasp {
namespace binary {
namespace test {

inline SpanU8 operator"" _su8(const char* str, size_t N) {
  return SpanU8{reinterpret_cast<const u8*>(str),
                static_cast<SpanU8::index_type>(N)};
}

inline Expression operator"" _expr(const char* str, size_t N) {
  return Expression{operator"" _su8(str, N)};
}

}  // namespace test
}  // namespace binary
}  // namespace wasp

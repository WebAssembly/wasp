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

#ifndef WASP_BASE_TO_STRING_H_
#define WASP_BASE_TO_STRING_H_

#include <string>
#include <vector>

#include "src/base/format.h"
#include "src/base/types.h"

namespace wasp {

inline std::string ToString(u32 x) {
  return format("{}", x);
}

inline std::string ToString(const SpanU8& self) {
  std::string result = "\"";
  for (auto x : self) {
    result += format("\\{:02x}", x);
  }
  result += '\"';
  return result;
}

template <typename T>
std::string ToString(const std::vector<T>& self) {
  bool first = true;
  std::string result = "[";
  for (const auto& x : self) {
    if (!first) {
      result += ' ';
    }
    result += ToString(x);
    first = false;
  }
  result += ']';
  return result;
}

}  // namespace wasp

#endif // WASP_BASE_TO_STRING_H_

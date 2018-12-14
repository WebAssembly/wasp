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

#ifndef WASP_BINARY_ERRORS_VECTOR_H_
#define WASP_BINARY_ERRORS_VECTOR_H_

#include <cstdio>

#include "src/base/types.h"

namespace wasp {
namespace binary {

/// ---
class ErrorsVector {
 public:
  // TODO
  void PushContext(SpanU8 pos, string_view desc) {}
  void PopContext() {}
  void OnError(SpanU8 pos, string_view message) {
    print("error: {}\n", message);
  }
};

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_ERRORS_VECTOR_H_

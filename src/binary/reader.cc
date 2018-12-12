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

#include "src/binary/reader.h"

#include <cassert>
#include <type_traits>

#include "absl/strings/str_format.h"

#include "src/binary/encoding.h"
#include "src/binary/to_string.h"

namespace wasp {
namespace binary {

void ErrorsVector::PushContext(SpanU8 pos, string_view desc) {
}

void ErrorsVector::PopContext() {
}

void ErrorsVector::OnError(SpanU8 pos, string_view message) {
  absl::PrintF("error: %s\n", message);
}

}  // namespace binary
}  // namespace wasp

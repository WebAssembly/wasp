//
// Copyright 2020 WebAssembly Community Group participants
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

#include "wasp/binary/lazy_sequence.h"

#include "wasp/base/errors.h"
#include "wasp/base/format.h"

namespace wasp {
namespace binary {

// static
void LazySequenceBase::OnCountError(Errors& errors,
                                    SpanU8 data,
                                    string_view name,
                                    Index expected,
                                    Index actual) {
  errors.OnError(data, format("Expected {} to have count {}, got {}", name,
                              expected, actual));
}

}  // namespace binary
}  // namespace wasp

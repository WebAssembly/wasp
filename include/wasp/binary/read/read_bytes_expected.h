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

#ifndef WASP_BINARY_READ_READ_BYTES_EXPECTED_H_
#define WASP_BINARY_READ_READ_BYTES_EXPECTED_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/read_bytes.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<SpanU8> ReadBytesExpected(SpanU8* data,
                                   SpanU8 expected,
                                   const Features& features,
                                   Errors& errors,
                                   string_view desc) {
  ErrorsContextGuard<Errors> guard{errors, *data, desc};

  auto actual = ReadBytes(data, expected.size(), features, errors);
  if (actual && actual != expected) {
    errors.OnError(*data,
                   format("Mismatch: expected {}, got {}", expected, *actual));
  }
  return actual;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_BYTES_EXPECTED_H_
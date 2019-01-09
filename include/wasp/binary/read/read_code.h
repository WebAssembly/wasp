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

#ifndef WASP_BINARY_READ_READ_CODE_H_
#define WASP_BINARY_READ_READ_CODE_H_

#include "wasp/base/features.h"
#include "wasp/binary/code.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_length.h"
#include "wasp/binary/read/read_locals.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Code> Read(SpanU8* data,
                    const Features& features,
                    Errors& errors,
                    Tag<Code>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "code"};
  WASP_TRY_READ(body_size, ReadLength(data, features, errors));
  WASP_TRY_READ(body, ReadBytes(data, body_size, features, errors));
  WASP_TRY_READ(locals,
                ReadVector<Locals>(&body, features, errors, "locals vector"));
  return Code{std::move(locals), Expression{std::move(body)}};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_CODE_H_

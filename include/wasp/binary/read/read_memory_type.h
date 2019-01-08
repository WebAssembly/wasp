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

#ifndef WASP_BINARY_READ_READ_MEMORY_TYPE_H_
#define WASP_BINARY_READ_READ_MEMORY_TYPE_H_

#include "wasp/binary/memory_type.h"

#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_limits.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<MemoryType> Read(SpanU8* data, Errors& errors, Tag<MemoryType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, errors));
  return MemoryType{limits};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_MEMORY_TYPE_H_

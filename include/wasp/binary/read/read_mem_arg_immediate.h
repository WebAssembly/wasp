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

#ifndef WASP_BINARY_READ_READ_MEM_ARG_IMMEDIATE_H_
#define WASP_BINARY_READ_READ_MEM_ARG_IMMEDIATE_H_

#include "wasp/binary/instruction.h"  // XXX

#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_u32.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<MemArgImmediate> Read(SpanU8* data,
                               Errors& errors,
                               Tag<MemArgImmediate>) {
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, errors), "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, errors), "offset");
  return MemArgImmediate{align_log2, offset};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_MEM_ARG_IMMEDIATE_H_

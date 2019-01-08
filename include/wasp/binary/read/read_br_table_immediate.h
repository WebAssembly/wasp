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

#ifndef WASP_BINARY_READ_READ_BR_TABLE_IMMEDIATE_H_
#define WASP_BINARY_READ_READ_BR_TABLE_IMMEDIATE_H_

#include "wasp/binary/instruction.h"  // XXX

#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<BrTableImmediate> Read(SpanU8* data,
                                Errors& errors,
                                Tag<BrTableImmediate>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "br_table"};
  WASP_TRY_READ(targets, ReadVector<Index>(data, errors, "targets"));
  WASP_TRY_READ(default_target, ReadIndex(data, errors, "default target"));
  return BrTableImmediate{std::move(targets), default_target};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_BR_TABLE_IMMEDIATE_H_

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

#ifndef WASP_BINARY_READ_READ_DATA_SEGMENT_H_
#define WASP_BINARY_READ_READ_DATA_SEGMENT_H_

#include "wasp/base/features.h"
#include "wasp/binary/data_segment.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_constant_expression.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_length.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<DataSegment> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<DataSegment>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "data segment"};
  WASP_TRY_READ(memory_index,
                ReadIndex(data, features, errors, "memory index"));
  WASP_TRY_READ_CONTEXT(
      offset, Read<ConstantExpression>(data, features, errors), "offset");
  WASP_TRY_READ(len, ReadLength(data, features, errors));
  WASP_TRY_READ(init, ReadBytes(data, len, features, errors));
  return DataSegment{memory_index, std::move(offset), init};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_DATA_SEGMENT_H_

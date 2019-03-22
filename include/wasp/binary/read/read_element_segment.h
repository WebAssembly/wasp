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

#ifndef WASP_BINARY_READ_READ_ELEMENT_SEGMENT_H_
#define WASP_BINARY_READ_READ_ELEMENT_SEGMENT_H_

#include "wasp/base/features.h"
#include "wasp/binary/element_segment.h"
#include "wasp/binary/encoding/segment_flags_encoding.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_constant_expression.h"
#include "wasp/binary/read/read_element_expression.h"
#include "wasp/binary/read/read_element_type.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

inline optional<ElementSegment> Read(SpanU8* data,
                                     const Features& features,
                                     Errors& errors,
                                     Tag<ElementSegment>) {
  ErrorsContextGuard guard{errors, *data, "element segment"};
  auto decoded = encoding::DecodedSegmentFlags::MVP();
  if (features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, features, errors, "flags"));
    WASP_TRY_DECODE(decoded_opt, flags, SegmentFlags, "flags");
    decoded = *decoded_opt;
  }

  Index table_index = 0;
  if (decoded.has_index == encoding::HasIndex::Yes) {
    WASP_TRY_READ(table_index_,
                  ReadIndex(data, features, errors, "table index"));
    table_index = table_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(
        offset, Read<ConstantExpression>(data, features, errors), "offset");
    WASP_TRY_READ(init,
                  ReadVector<Index>(data, features, errors, "initializers"));
    return ElementSegment{table_index, offset, init};
  } else {
    WASP_TRY_READ(element_type, Read<ElementType>(data, features, errors));
    WASP_TRY_READ(init, ReadVector<ElementExpression>(data, features, errors,
                                                      "initializers"));
    return ElementSegment{element_type, init};
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_ELEMENT_SEGMENT_H_

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

#ifndef WASP_BINARY_WRITE_WRITE_ELEMENT_SEGMENT_H_
#define WASP_BINARY_WRITE_WRITE_ELEMENT_SEGMENT_H_

#include "wasp/binary/element_segment.h"
#include "wasp/binary/encoding/segment_flags_encoding.h"
#include "wasp/binary/write/write_constant_expression.h"
#include "wasp/binary/write/write_element_expression.h"
#include "wasp/binary/write/write_element_type.h"
#include "wasp/binary/write/write_index.h"
#include "wasp/binary/write/write_u32.h"
#include "wasp/binary/write/write_u8.h"
#include "wasp/binary/write/write_vector.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(const ElementSegment& value, Iterator out) {
  encoding::DecodedSegmentFlags flags = {value.segment_type(),
                                         encoding::HasIndex::No};
  if (flags.segment_type == SegmentType::Active) {
    const auto& active = value.active();
    flags.has_index = active.table_index != 0 ? encoding::HasIndex::Yes
                                              : encoding::HasIndex::No;
    out = Write(encoding::SegmentFlags::Encode(flags), out);
    if (flags.has_index == encoding::HasIndex::Yes) {
      out = Write(active.table_index, out);
    }
    out = Write(active.offset, out);
    out = WriteVector(active.init.begin(), active.init.end(), out);
  } else {
    const auto& passive = value.passive();
    out = Write(encoding::SegmentFlags::Encode(flags), out);
    out = Write(passive.element_type, out);
    out = WriteVector(passive.init.begin(), passive.init.end(), out);
  }
  return out;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_ELEMENT_SEGMENT_H_

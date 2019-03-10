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

#ifndef WASP_BINARY_WRITE_WRITE_DATA_SEGMENT_H_
#define WASP_BINARY_WRITE_WRITE_DATA_SEGMENT_H_

#include <limits>

#include "wasp/binary/data_segment.h"
#include "wasp/binary/encoding/segment_flags_encoding.h"
#include "wasp/binary/write/write_bytes.h"
#include "wasp/binary/write/write_constant_expression.h"
#include "wasp/binary/write/write_index.h"
#include "wasp/binary/write/write_u32.h"
#include "wasp/binary/write/write_u8.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(const DataSegment& value, Iterator out) {
  encoding::DecodedSegmentFlags flags = {value.segment_type(),
                                         encoding::HasIndex::No};
  if (flags.segment_type == SegmentType::Active) {
    const auto& active = value.active();
    flags.has_index = active.memory_index != 0 ? encoding::HasIndex::Yes
                                               : encoding::HasIndex::No;
    out = Write(encoding::SegmentFlags::Encode(flags), out);
    if (flags.has_index == encoding::HasIndex::Yes) {
      out = Write(active.memory_index, out);
    }
    out = Write(active.offset, out);
  } else {
    out = Write(encoding::SegmentFlags::Encode(flags), out);
  }
  assert(value.init.size() < std::numeric_limits<u32>::max());
  out = Write(static_cast<u32>(value.init.size()), out);
  out = WriteBytes(value.init, out);
  return out;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_DATA_SEGMENT_H_

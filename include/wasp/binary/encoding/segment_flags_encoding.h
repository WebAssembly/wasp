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

#ifndef WASP_BINARY_SEGMENT_FLAGS_ENCODING_H
#define WASP_BINARY_SEGMENT_FLAGS_ENCODING_H

#include "wasp/base/types.h"
#include "wasp/base/optional.h"
#include "wasp/binary/segment_type.h"

namespace wasp {
namespace binary {
namespace encoding {

enum class HasIndex { No, Yes };

struct DecodedSegmentFlags {
  static DecodedSegmentFlags MVP();

  SegmentType segment_type;
  HasIndex has_index;
};

// static
inline DecodedSegmentFlags DecodedSegmentFlags::MVP() {
  return DecodedSegmentFlags{SegmentType::Active, HasIndex::Yes};
}

struct SegmentFlags {
  static constexpr u8 ActiveIndex0 = 0;
  static constexpr u8 Passive = 1;
  static constexpr u8 ActiveWithIndex = 2;

  static u8 Encode(DecodedSegmentFlags);
  static optional<DecodedSegmentFlags> Decode(Index);
};

// static
inline u8 SegmentFlags::Encode(DecodedSegmentFlags flags) {
  if (flags.segment_type == SegmentType::Active) {
    return flags.has_index == HasIndex::Yes ? ActiveWithIndex : ActiveIndex0;
  }
  return Passive;
}

// static
inline optional<DecodedSegmentFlags> SegmentFlags::Decode(Index flags) {
  switch (flags) {
    case ActiveIndex0:
      return {{SegmentType::Active, HasIndex::No}};

    case Passive:
      return {{SegmentType::Passive, HasIndex::No}};

    case ActiveWithIndex:
      return {{SegmentType::Active, HasIndex::Yes}};

    default:
      return nullopt;
  }
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SEGMENT_TYPE_ENCODING_H

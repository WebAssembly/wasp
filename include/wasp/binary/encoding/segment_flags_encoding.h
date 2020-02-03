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

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/types_linking.h"

namespace wasp {
namespace binary {
namespace encoding {

enum class HasNonZeroIndex { No, Yes };
enum class HasExpressions { No, Yes };

struct DecodedDataSegmentFlags {
  static DecodedDataSegmentFlags MVP();

  SegmentType segment_type;
  HasNonZeroIndex has_non_zero_index;
};

struct DecodedElemSegmentFlags {
  static DecodedElemSegmentFlags MVP();

  bool is_legacy_active() const;

  SegmentType segment_type;
  HasNonZeroIndex has_non_zero_index;
  HasExpressions has_expressions;
};

// static
inline DecodedDataSegmentFlags DecodedDataSegmentFlags::MVP() {
  return DecodedDataSegmentFlags{SegmentType::Active, HasNonZeroIndex::No};
}

// static
inline DecodedElemSegmentFlags DecodedElemSegmentFlags::MVP() {
  return DecodedElemSegmentFlags{SegmentType::Active, HasNonZeroIndex::No,
                                 HasExpressions::No};
}

inline bool DecodedElemSegmentFlags::is_legacy_active() const {
  return segment_type == SegmentType::Active &&
         has_non_zero_index == HasNonZeroIndex::No;
}

struct DataSegmentFlags {
  static constexpr u8 ActiveIndex0 = 0;
  static constexpr u8 Passive = 1;
  static constexpr u8 ActiveWithIndex = 2;

  static u8 Encode(DecodedDataSegmentFlags);
  static optional<DecodedDataSegmentFlags> Decode(Index);
};

struct ElemSegmentFlags {
  static constexpr u8 Active = 0;
  static constexpr u8 Passive = 1;
  static constexpr u8 HasNonZeroIndex = 2;
  static constexpr u8 Declared = 3;
  static constexpr u8 HasExpressions = 4;

  static u8 Encode(DecodedElemSegmentFlags);
  static optional<DecodedElemSegmentFlags> Decode(Index, const Features&);
};

// static
inline u8 DataSegmentFlags::Encode(DecodedDataSegmentFlags flags) {
  if (flags.segment_type == SegmentType::Active) {
    return flags.has_non_zero_index == HasNonZeroIndex::Yes ? ActiveWithIndex
                                                            : ActiveIndex0;
  }
  return Passive;
}

// static
inline optional<DecodedDataSegmentFlags> DataSegmentFlags::Decode(Index flags) {
  switch (flags) {
    case ActiveIndex0:
      return {{SegmentType::Active, HasNonZeroIndex::No}};

    case Passive:
      return {{SegmentType::Passive, HasNonZeroIndex::No}};

    case ActiveWithIndex:
      return {{SegmentType::Active, HasNonZeroIndex::Yes}};

    default:
      return nullopt;
  }
}

// static
inline u8 ElemSegmentFlags::Encode(DecodedElemSegmentFlags flags) {
  u8 result = 0;
  if (flags.segment_type == SegmentType::Passive) {
    result |= Passive;
  } else if (flags.segment_type == SegmentType::Declared) {
    assert(flags.has_non_zero_index == HasNonZeroIndex::No);
    result |= Declared;
  }
  if (flags.has_non_zero_index == HasNonZeroIndex::Yes) {
    result |= HasNonZeroIndex;
  }
  if (flags.has_expressions == HasExpressions::Yes) {
    result |= HasExpressions;
  }
  return result;
}

// static
inline optional<DecodedElemSegmentFlags> ElemSegmentFlags::Decode(
    Index flags,
    const Features& features) {
  switch (flags) {
    case Active:
      return {{SegmentType::Active, HasNonZeroIndex::No, HasExpressions::No}};

    case Passive:
      return {{SegmentType::Passive, HasNonZeroIndex::No, HasExpressions::No}};

    case Active | HasNonZeroIndex:
      return {{SegmentType::Active, HasNonZeroIndex::Yes, HasExpressions::No}};

    case Declared:
      if (features.reference_types_enabled()) {
        return {
            {SegmentType::Declared, HasNonZeroIndex::No, HasExpressions::No}};
      }
      break;

    case Active | HasExpressions:
      return {{SegmentType::Active, HasNonZeroIndex::No, HasExpressions::Yes}};

    case Passive | HasExpressions:
      return {{SegmentType::Passive, HasNonZeroIndex::No, HasExpressions::Yes}};

    case Active | HasNonZeroIndex | HasExpressions:
      return {{SegmentType::Active, HasNonZeroIndex::Yes, HasExpressions::Yes}};

    case Declared | HasExpressions:
      if (features.reference_types_enabled()) {
        return {
            {SegmentType::Declared, HasNonZeroIndex::No, HasExpressions::Yes}};
      }
      break;
  }
  return nullopt;
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SEGMENT_TYPE_ENCODING_H

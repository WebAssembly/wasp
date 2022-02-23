//
// Copyright 2020 WebAssembly Community Group participants
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

#include "wasp/binary/encoding.h"

#include <cassert>

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/types.h"

namespace wasp::binary::encoding {

// BlockType values are 0x40, and 0x7c through 0x7f in the MVP. In the
// multi-value proposal, a block type is extended to a s32 value, where
// negative values represent the standard value types, and non-negative values
// are indexes into the type section.
//
// The values 0x40, 0x7c..0x7f are all representations of small negative
// numbers encoded as signed LEB128. For example, 0x40 is the encoding for -64.
// Signed LEB128 values have their sign bit as the 6th bit (instead of the 7th
// bit), so to convert them to a s32 value, we must shift by 25.
constexpr s32 EncodeU8AsSLEB128(u8 value) {
  return (value << 25) >> 25;
}

// static
bool BlockType::IsBare(u8 val) {
  return val == Void;
}

// static
bool BlockType::IsS32(u8 val) {
  return val < 0x40 || val >= 0x80;
}

// static
optional<::wasp::binary::BlockType> BlockType::Decode(
    At<u8> val,
    const Features& features) {
  if (val == Void) {
    return binary::BlockType{At{val.loc(), VoidType{}}};
  }
  return nullopt;
}

// static
optional<::wasp::binary::BlockType> BlockType::Decode(
    At<s32> val,
    const Features& features) {
  if (val >= 0 && features.multi_value_enabled()) {
    return binary::BlockType{At{val.loc(), Index(val)}};
  }
  return nullopt;
}

// static
u8 TagAttribute::Encode(::wasp::TagAttribute decoded) {
  return u8(decoded);
}

// static
optional<::wasp::TagAttribute> TagAttribute::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::TagAttribute::Name;
#include "wasp/base/inc/tag_attribute.inc"
#undef WASP_V
    default:
      return nullopt;
  }
}

// static
u8 ExternalKind::Encode(::wasp::ExternalKind decoded) {
  return u8(decoded);
}

// static
optional<::wasp::ExternalKind> ExternalKind::Decode(u8 val,
                                                    const Features& features) {
  switch (val) {
#define WASP_V(val, Name, str, ...) \
  case val:                         \
    return ::wasp::ExternalKind::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::ExternalKind::Name;        \
    }                                           \
    break;
#include "wasp/base/inc/external_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
bool HeapKind::Is(u8 byte) {
  return false
#define WASP_V(val, Name, str) || byte == u8(::wasp::HeapKind::Name)
#define WASP_FEATURE_V(val, Name, str, feature) \
  || byte == u8(::wasp::HeapKind::Name)
#include "wasp/base/inc/heap_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
      ;
}

// static
u8 HeapKind::Encode(const ::wasp::HeapKind& decoded) {
  return u8(decoded);
}

// static
optional<::wasp::HeapKind> HeapKind::Decode(u8 val, const Features& features) {
  switch (val) {
#define WASP_V(val, Name, str, ...) \
  case val:                         \
    return ::wasp::HeapKind::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::HeapKind::Name;            \
    }                                           \
    break;
#include "wasp/base/inc/heap_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
u8 LimitsFlags::Encode(const DecodedLimitsFlags& decoded) {
  if (decoded.shared == Shared::No) {
    if (decoded.has_max == HasMax::No && decoded.index_type == IndexType::I32) {
      return NoMax | IndexType32;
    } else if (decoded.has_max == HasMax::Yes &&
               decoded.index_type == IndexType::I32) {
      return HasMax | IndexType32;
    } else if (decoded.has_max == HasMax::No &&
               decoded.index_type == IndexType::I64) {
      return NoMax | IndexType64;
    } else {
      assert(decoded.has_max == HasMax::Yes);
      assert(decoded.index_type == IndexType::I64);
      return HasMax | IndexType64;
    }
  } else {
    assert(decoded.has_max == HasMax::Yes);
    assert(decoded.index_type == IndexType::I32);
    return HasMaxAndShared;
  }
}

// static
u8 LimitsFlags::Encode(const Limits& limits) {
  return Encode(DecodedLimitsFlags{limits.max ? HasMax::Yes : HasMax::No,
                                   limits.shared, limits.index_type});
}

// static
optional<DecodedLimitsFlags> LimitsFlags::Decode(u8 flags,
                                                 const Features& features) {
  switch (flags) {
    case LimitsFlags::NoMax | LimitsFlags::IndexType32:
      return {{HasMax::No, Shared::No, IndexType::I32}};

    case LimitsFlags::HasMax | LimitsFlags::IndexType32:
      return {{HasMax::Yes, Shared::No, IndexType::I32}};

    case LimitsFlags::NoMax | LimitsFlags::IndexType64:
      if (features.memory64_enabled()) {
        return {{HasMax::No, Shared::No, IndexType::I64}};
      } else {
        return nullopt;
      }

    case LimitsFlags::HasMax | LimitsFlags::IndexType64:
      if (features.memory64_enabled()) {
        return {{HasMax::Yes, Shared::No, IndexType::I64}};
      } else {
        return nullopt;
      }

    case LimitsFlags::HasMaxAndShared:
      if (features.threads_enabled()) {
        return {{HasMax::Yes, Shared::Yes, IndexType::I32}};
      } else {
        return nullopt;
      }

    default:
      return nullopt;
  }
}

// static
u8 Mutability::Encode(::wasp::Mutability decoded) {
  return u8(decoded);
}

// static
optional<::wasp::Mutability> Mutability::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::Mutability::Name;
#include "wasp/base/inc/mutability.inc"
#undef WASP_V
    default:
      return nullopt;
  }
}

// static
u8 Null::Encode(::wasp::Null decoded) {
  return u8(decoded);
}

// static
optional<::wasp::Null> Null::Decode(u8 val) {
  switch (val) {
    case u8(::wasp::Null::No):
      return ::wasp::Null::No;

    case u8(::wasp::Null::Yes):
      return ::wasp::Null::Yes;

    default:
      return nullopt;
  }
}

// static
bool Opcode::IsPrefixByte(u8 code, const Features& features) {
  switch (code) {
    case GcPrefix:
      return features.gc_enabled();

    case MiscPrefix:
      return features.saturating_float_to_int_enabled() ||
             features.bulk_memory_enabled() ||
             features.reference_types_enabled();

    case SimdPrefix:
      return features.simd_enabled();

    case ThreadsPrefix:
      return features.threads_enabled();

    default:
      return false;
  }
}

// static
EncodedOpcode Opcode::Encode(::wasp::Opcode decoded) {
  switch (decoded) {
#define WASP_V(prefix, code, Name, str) \
  case ::wasp::Opcode::Name:            \
    return {code, {}};
#define WASP_FEATURE_V(prefix, code, Name, str, feature) \
  WASP_V(prefix, code, Name, str)
#define WASP_PREFIX_V(prefix, code, Name, str, feature) \
  case ::wasp::Opcode::Name:                            \
    return {prefix, code};
#include "wasp/base/inc/opcode.inc"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      WASP_UNREACHABLE();
  }
}

// static
optional<::wasp::Opcode> Opcode::Decode(u8 code, const Features& features) {
  switch (code) {
#define WASP_V(prefix, code, Name, str) \
  case code:                            \
    return ::wasp::Opcode::Name;
#define WASP_FEATURE_V(prefix, code, Name, str, feature) \
  case code:                                             \
    if (features.feature##_enabled()) {                  \
      return ::wasp::Opcode::Name;                       \
    }                                                    \
    break;
#define WASP_PREFIX_V(...) /* Invalid. */
#include "wasp/base/inc/opcode.inc"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      break;
  }
  return nullopt;
}

constexpr u64 MakePrefixCode(u8 prefix, u32 code) {
  return (u64{prefix} << 32) | code;
}

// static
optional<::wasp::Opcode> Opcode::Decode(u8 prefix,
                                        u32 code,
                                        const Features& features) {
  switch (MakePrefixCode(prefix, code)) {
#define WASP_V(...) /* Invalid. */
#define WASP_FEATURE_V(...) /* Invalid. */
#define WASP_PREFIX_V(prefix, code, Name, str, feature) \
  case MakePrefixCode(prefix, code):                    \
    if (features.feature##_enabled()) {                 \
      return ::wasp::Opcode::Name;                      \
    }                                                   \
    break;
#include "wasp/base/inc/opcode.inc"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      break;
  }
  return nullopt;
}

// static
bool RefType::Is(u8 val) {
  return val == Ref || val == RefNull;
}

// static
u8 RefType::Encode(::wasp::Null null) {
  return null == ::wasp::Null::No ? Ref : RefNull;
}

// static
optional<::wasp::Null> RefType::Decode(u8 code, const Features& features) {
  switch (code) {
    case RefNull:
      return ::wasp::Null::Yes;

    case Ref:
      return ::wasp::Null::No;

    default:
      return nullopt;
  }
}

// static
u8 ReferenceKind::Encode(::wasp::ReferenceKind decoded) {
  return u8(decoded);
}

// static
optional<::wasp::ReferenceKind> ReferenceKind::Decode(
    u8 val,
    const Features& features) {
  switch (val) {
#define WASP_V(val, Name, str, ...) \
  case val:                         \
    return ::wasp::ReferenceKind::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::ReferenceKind::Name;       \
    }                                           \
    break;
#include "wasp/base/inc/reference_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
bool Rtt::Is(u8 val) {
  return val == RttPrefix;
}

// static
u32 SectionId::Encode(::wasp::binary::SectionId decoded) {
  switch (decoded) {
#define WASP_V(val, Name, str, ...)     \
  case ::wasp::binary::SectionId::Name: \
    return val;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/inc/section_id.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
}

// static
optional<::wasp::binary::SectionId> SectionId::Decode(
    u32 val,
    const Features& features) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::SectionId::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::binary::SectionId::Name;   \
    }                                           \
    break;
#include "wasp/binary/inc/section_id.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
DecodedDataSegmentFlags DecodedDataSegmentFlags::MVP() {
  return DecodedDataSegmentFlags{SegmentType::Active, HasNonZeroIndex::No};
}

// static
DecodedElemSegmentFlags DecodedElemSegmentFlags::MVP() {
  return DecodedElemSegmentFlags{SegmentType::Active, HasNonZeroIndex::No,
                                 HasExpressions::No};
}

bool DecodedElemSegmentFlags::is_legacy_active() const {
  return segment_type == SegmentType::Active &&
         has_non_zero_index == HasNonZeroIndex::No;
}

// static
u8 DataSegmentFlags::Encode(DecodedDataSegmentFlags flags) {
  if (flags.segment_type == SegmentType::Active) {
    return flags.has_non_zero_index == HasNonZeroIndex::Yes ? ActiveWithIndex
                                                            : ActiveIndex0;
  }
  return Passive;
}

// static
optional<DecodedDataSegmentFlags> DataSegmentFlags::Decode(Index flags) {
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
u8 ElemSegmentFlags::Encode(DecodedElemSegmentFlags flags) {
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
optional<DecodedElemSegmentFlags> ElemSegmentFlags::Decode(
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

// static
bool NumericType::Is(u8 byte) {
  return false
#define WASP_V(val, Name, str) || byte == u8(::wasp::NumericType::Name)
#define WASP_FEATURE_V(val, Name, str, feature) \
  || byte == u8(::wasp::NumericType::Name)
#include "wasp/base/inc/numeric_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
      ;
}

// static
u8 NumericType::Encode(::wasp::NumericType decoded) {
  return u8(decoded);
}

// static
optional<::wasp::NumericType> NumericType::Decode(u8 val,
                                                  const Features& features) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::NumericType::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::NumericType::Name;         \
    }                                           \
    break;
#include "wasp/base/inc/numeric_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
bool PackedType::Is(u8 byte) {
  return false
#define WASP_V(val, Name, str) || byte == u8(::wasp::PackedType::Name)
#define WASP_FEATURE_V(val, Name, str, feature) \
  || byte == u8(::wasp::PackedType::Name)
#include "wasp/base/inc/packed_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
      ;
}

// static
u8 PackedType::Encode(::wasp::PackedType decoded) {
  return u8(decoded);
}

// static
optional<::wasp::PackedType> PackedType::Decode(u8 byte,
                                                const Features& features) {
  switch (byte) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::PackedType::Name;
#define WASP_FEATURE_V(val, Name, str, feature) \
  case val:                                     \
    if (features.feature##_enabled()) {         \
      return ::wasp::PackedType::Name;          \
    }                                           \
    break;
#include "wasp/base/inc/packed_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

}  // namespace wasp::binary::encoding

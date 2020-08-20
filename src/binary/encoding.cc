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
constexpr u8 EncodeSLEB128AsU8(s32 value) {
  return u32(value) & 0x7f;
}

constexpr s32 EncodeU8AsSLEB128(u8 value) {
  return (value << 25) >> 25;
}

// static
EncodedValueType BlockType::Encode(::wasp::binary::BlockType decoded) {
  if (decoded.is_value_type()) {
    return ValueType::Encode(decoded.value_type());
  } else if (decoded.is_void()) {
    return {EncodeU8AsSLEB128(Void), nullopt};
  } else {
    return {s32(decoded.index()), nullopt};
  }
}

// static
optional<::wasp::binary::BlockType> BlockType::Decode(
    At<u8> val,
    const Features& features) {
  if (val == Void) {
    return binary::BlockType{At{val.loc(), VoidType{}}};
  } else {
    auto opt_valtype = ValueType::Decode(val, features);
    if (opt_valtype) {
      return binary::BlockType{At{val.loc(), *opt_valtype}};
    }
  }
  return nullopt;
}

// static
optional<::wasp::binary::BlockType> BlockType::Decode(
    At<s32> val,
    const Features& features) {
  if (val >= 0) {
    if (features.multi_value_enabled()) {
      return binary::BlockType{At{val.loc(), Index(val)}};
    }
  } else if (EncodeSLEB128AsU8(val) == Void) {
    return binary::BlockType{At{val.loc(), VoidType{}}};
  } else if (val >= -0xff) {
    auto opt_valtype =
        ValueType::Decode(At{val.loc(), EncodeSLEB128AsU8(val)}, features);
    if (opt_valtype) {
      return binary::BlockType{At{val.loc(), *opt_valtype}};
    }
  }
  return nullopt;
}

// static
u8 EventAttribute::Encode(::wasp::EventAttribute decoded) {
  return u8(decoded);
}

// static
optional<::wasp::EventAttribute> EventAttribute::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::EventAttribute::Name;
#include "wasp/base/def/event_attribute.def"
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
#include "wasp/base/def/external_kind.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
s32 HeapType::Encode(const ::wasp::binary::HeapType& decoded) {
  if (decoded.is_heap_kind()) {
    return EncodeU8AsSLEB128(u8(*decoded.heap_kind()));
  } else {
    assert(decoded.is_index());
    return s32(decoded.index());
  }
}

// static
optional<::wasp::binary::HeapType> HeapType::Decode(At<s32> val,
                                                    const Features& features) {
  if (val >= 0) {
    return ::wasp::binary::HeapType{At{val.loc(), Index(val)}};
  } else {
    optional<HeapKind> kind_opt;
    switch (val) {
      case Func:
        kind_opt = HeapKind::Func;
        break;

      case Extern:
        if (features.reference_types_enabled()) {
          kind_opt = HeapKind::Extern;
        }
        break;

      case Exn:
        if (features.exceptions_enabled()) {
          kind_opt = HeapKind::Exn;
        }
        break;

      default:
        break;
    }

    if (kind_opt) {
      return ::wasp::binary::HeapType{At{val.loc(), *kind_opt}};
    }
  }
  return nullopt;
}

// static
u8 LimitsFlags::Encode(const DecodedLimitsFlags& decoded) {
  if (decoded.shared == Shared::No) {
    if (decoded.has_max == HasMax::No) {
      return NoMax;
    } else {
      return HasMax;
    }
  } else {
    assert(decoded.has_max == HasMax::Yes);
    return HasMaxAndShared;
  }
}

// static
u8 LimitsFlags::Encode(const Limits& limits) {
  return Encode(
      DecodedLimitsFlags{limits.max ? HasMax::Yes : HasMax::No, limits.shared});
}

// static
optional<DecodedLimitsFlags> LimitsFlags::Decode(u8 flags,
                                                 const Features& features) {
  switch (flags) {
    case LimitsFlags::NoMax:
      return {{HasMax::No, Shared::No}};

    case LimitsFlags::HasMax:
      return {{HasMax::Yes, Shared::No}};

    case LimitsFlags::HasMaxAndShared:
      if (features.threads_enabled()) {
        return {{HasMax::Yes, Shared::Yes}};
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
#include "wasp/base/def/mutability.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

// static
bool Opcode::IsPrefixByte(u8 code, const Features& features) {
  switch (code) {
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
#include "wasp/base/def/opcode.def"
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
#include "wasp/base/def/opcode.def"
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
#include "wasp/base/def/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default:
      break;
  }
  return nullopt;
}

// static
bool ReferenceType::IsPrefixByte(u8 val, const Features& features) {
  return features.function_references_enabled() &&
         (val == Ref || val == RefNull);
}

// static
EncodedValueType ReferenceType::Encode(::wasp::binary::ReferenceType decoded) {
  if (decoded.is_reference_kind()) {
    return {EncodeU8AsSLEB128(u8(*decoded.reference_kind())), nullopt};
  } else {
    assert(decoded.is_ref());
    return {EncodeU8AsSLEB128(decoded.ref()->null == Null::Yes ? RefNull : Ref),
            HeapType::Encode(decoded.ref()->heap_type)};
  }
}

// static
optional<::wasp::binary::ReferenceType> ReferenceType::Decode(
    At<u8> val,
    const Features& features,
    AllowFuncref allow_funcref) {
  optional<ReferenceKind> kind_opt;
  switch (val) {
    case Funcref:
      if (allow_funcref == AllowFuncref::Yes ||
          features.reference_types_enabled()) {
        kind_opt = ReferenceKind::Funcref;
      }
      break;

    case Externref:
      if (features.reference_types_enabled()) {
        kind_opt = ReferenceKind::Externref;
      }
      break;

    case RefNull:
    case Ref:
      assert(false);  // Use the other Decode function.
      break;

    case Exnref:
      if (features.exceptions_enabled()) {
        kind_opt = ReferenceKind::Exnref;
      }
      break;

    default:
      break;
  }

  if (kind_opt) {
    return ::wasp::binary::ReferenceType{At{val.loc(), *kind_opt}};
  }
  return nullopt;
}

// static
optional<::wasp::binary::ReferenceType>
ReferenceType::Decode(At<u8> prefix, At<s32> code, const Features& features) {
  assert(features.function_references_enabled());
  assert(prefix == RefNull || prefix == Ref);
  auto heap_type = HeapType::Decode(code, features);
  if (heap_type) {
    Null null = prefix == RefNull ? Null::Yes : Null::No;
    return ::wasp::binary::ReferenceType{
        ::wasp::binary::RefType{At{code.loc(), *heap_type}, null}};
  }
  return nullopt;
}

// static
u32 SectionId::Encode(::wasp::binary::SectionId decoded) {
  switch (decoded) {
#define WASP_V(val, Name, str, ...)     \
  case ::wasp::binary::SectionId::Name: \
    return val;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/def/section_id.def"
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
#include "wasp/binary/def/section_id.def"
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
#include "wasp/base/def/numeric_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      break;
  }
  return nullopt;
}

// static
bool ValueType::IsPrefixByte(u8 val, const Features& features) {
  return ReferenceType::IsPrefixByte(val, features);
}

// static
EncodedValueType ValueType::Encode(::wasp::binary::ValueType decoded) {
  if (decoded.is_numeric_type()) {
    return {EncodeU8AsSLEB128(NumericType::Encode(decoded.numeric_type())),
            nullopt};
  } else {
    assert(decoded.is_reference_type());
    return ReferenceType::Encode(decoded.reference_type());
  }
}

// static
optional<::wasp::binary::ValueType> ValueType::Decode(
    At<u8> val,
    const Features& features) {
  auto opt_numeric = NumericType::Decode(val, features);
  if (opt_numeric) {
    return ::wasp::binary::ValueType{At{val.loc(), *opt_numeric}};
  } else {
    auto opt_ref = ReferenceType::Decode(val, features, AllowFuncref::No);
    if (opt_ref) {
      return ::wasp::binary::ValueType{At{val.loc(), *opt_ref}};
    }
    return nullopt;
  }
}

// static
optional<::wasp::binary::ValueType>
ValueType::Decode(At<u8> prefix, At<s32> code, const Features& features) {
  auto opt_ref = ReferenceType::Decode(prefix, code, features);
  if (opt_ref) {
    return ::wasp::binary::ValueType{At{code.loc(), *opt_ref}};
  }
  return nullopt;
}

}  // namespace wasp::binary::encoding

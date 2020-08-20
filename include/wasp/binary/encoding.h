//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_ENCODING_H
#define WASP_BINARY_ENCODING_H

#include "wasp/base/types.h"
#include "wasp/base/wasm_types.h"
#include "wasp/binary/types.h"

namespace wasp {

class Features;

namespace binary::encoding {

constexpr u8 Magic[] = {0, 'a', 's', 'm'};
constexpr u8 Version[] = {1, 0, 0, 0};

struct Type {
  static constexpr u8 Function = 0x60;
};

struct EncodedValueType {
  s32 code;
  optional<s32> immediate;
};

struct BlockType {
  static constexpr s32 Void = 0x40;

  static EncodedValueType Encode(::wasp::binary::BlockType);
  static optional<::wasp::binary::BlockType> Decode(At<u8>, const Features&);
  static optional<::wasp::binary::BlockType> Decode(At<s32>, const Features&);
};

struct EventAttribute {
  static u8 Encode(::wasp::EventAttribute);
  static optional<::wasp::EventAttribute> Decode(u8);
};

struct ExternalKind {
  static u8 Encode(::wasp::ExternalKind);
  static optional<::wasp::ExternalKind> Decode(u8, const Features& feature);
};

enum class HasMax { No, Yes };

struct DecodedLimitsFlags {
  HasMax has_max;
  Shared shared;
};

struct HeapType {
  static constexpr s32 Func = -0x10;
  static constexpr s32 Extern = -0x11;
  static constexpr s32 Exn = -0x18;

  static s32 Encode(const ::wasp::binary::HeapType&);
  static optional<::wasp::binary::HeapType> Decode(At<s32>, const Features&);
};

struct LimitsFlags {
  static constexpr u8 NoMax = 0;
  static constexpr u8 HasMax = 1;
  static constexpr u8 HasMaxAndShared = 3;

  static u8 Encode(const DecodedLimitsFlags&);
  static u8 Encode(const Limits&);
  static optional<DecodedLimitsFlags> Decode(u8, const Features&);
};

struct Mutability {
  static u8 Encode(::wasp::Mutability);
  static optional<::wasp::Mutability> Decode(u8);
};

struct EncodedOpcode {
  u8 u8_code;
  optional<u32> u32_code;
};

struct Opcode {
  static constexpr u8 MiscPrefix = 0xfc;
  static constexpr u8 SimdPrefix = 0xfd;
  static constexpr u8 ThreadsPrefix = 0xfe;

  static bool IsPrefixByte(u8, const Features&);
  static EncodedOpcode Encode(::wasp::Opcode);
  static optional<::wasp::Opcode> Decode(u8 code, const Features&);
  static optional<::wasp::Opcode> Decode(u8 prefix, u32 code, const Features&);
};

enum class AllowFuncref { No, Yes };

struct ReferenceType {
  static constexpr u8 Funcref = 0x70;
  static constexpr u8 Externref = 0x6f;
  static constexpr u8 RefNull = 0x6c;  // Prefix.
  static constexpr u8 Ref = 0x6b;      // Prefix.
  static constexpr u8 Exnref = 0x68;

  static bool IsPrefixByte(u8, const Features&);
  static EncodedValueType Encode(::wasp::binary::ReferenceType);
  static optional<::wasp::binary::ReferenceType>
  Decode(At<u8>, const Features&, AllowFuncref = AllowFuncref::Yes);
  static optional<::wasp::binary::ReferenceType> Decode(At<u8> prefix,
                                                        At<s32> code,
                                                        const Features&);
};

struct SectionId {
  static u32 Encode(::wasp::binary::SectionId);
  static optional<::wasp::binary::SectionId> Decode(u32,
                                                    const Features& features);
};

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

struct NumericType {
  static u8 Encode(::wasp::NumericType);
  static optional<::wasp::NumericType> Decode(u8, const Features&);
};

struct ValueType {
  static bool IsPrefixByte(u8, const Features&);
  static EncodedValueType Encode(::wasp::binary::ValueType);
  static optional<::wasp::binary::ValueType> Decode(At<u8>, const Features&);
  static optional<::wasp::binary::ValueType> Decode(At<u8> prefix,
                                                    At<s32> code,
                                                    const Features&);
};

}  // namespace binary::encoding
}  // namespace wasp

#endif // WASP_BINARY_ENCODING_H

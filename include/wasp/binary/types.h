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

#ifndef WASP_BINARY_TYPES_H_
#define WASP_BINARY_TYPES_H_

#include <array>
#include <functional>
#include <vector>

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"

namespace wasp {
namespace binary {

// BlockType values are 0x40, and 0x7c through 0x7f in the MVP. In the
// multi-value proposal, a block type is extended to a s32 value, where
// negative values represent the standard value types, and non-negative values
// are indexes into the type section.
//
// The values 0x40, 0x7c..0x7f are all representations of small negative
// numbers encoded as signed LEB128. For example, 0x40 is the encoding for -64.
// Signed LEB128 values have their sign bit as the 6th bit (instead of the 7th
// bit), so to convert them to a s32 value, we must shift by 25.
constexpr s32 ConvertValueTypeToBlockType(u8 value) {
  return (value << 25) >> 25;
}

enum class BlockType : s32 {
#define WASP_V(val, Name, str) Name = ConvertValueTypeToBlockType(val),
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/block_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

static_assert(s32(BlockType::I32) == -1, "Invalid value for BlockType::I32");
static_assert(s32(BlockType::I64) == -2, "Invalid value for BlockType::I64");
static_assert(s32(BlockType::F32) == -3, "Invalid value for BlockType::F32");
static_assert(s32(BlockType::F64) == -4, "Invalid value for BlockType::F64");
static_assert(s32(BlockType::Void) == -64, "Invalid value for BlockType::Void");

enum class ElementType : s32 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/element_type.def"
#undef WASP_V
};

enum class ExternalKind : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/external_kind.def"
#undef WASP_V
};

enum class Mutability : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/mutability.def"
#undef WASP_V
};

enum class Opcode : u32 {
#define WASP_V(prefix, val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
};

// The section ids are ordered by their expected order in the binary format.
enum class SectionId : u32 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/section_id.def"
#undef WASP_V
};

enum class SegmentType {
  Active,
  Passive,
};

enum class Shared {
  No,
  Yes,
};

enum class ValueType : s32 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};


using ValueTypes = std::vector<ValueType>;
using ShuffleImmediate = std::array<u8, 16>;

// Section

struct KnownSection {
  SectionId id;
  SpanU8 data;
};

struct CustomSection {
  string_view name;
  SpanU8 data;
};

struct Section {
  bool is_known() const;
  bool is_custom() const;

  KnownSection& known();
  const KnownSection& known() const;
  CustomSection& custom();
  const CustomSection& custom() const;

  SectionId id() const;
  SpanU8 data() const;

  variant<KnownSection, CustomSection> contents;
};


// Instruction

struct EmptyImmediate {};

struct CallIndirectImmediate {
  Index index;
  Index table_index;
};

struct BrOnExnImmediate {
  Index target;
  Index exception_index;
};

struct BrTableImmediate {
  std::vector<Index> targets;
  Index default_target;
};

struct MemArgImmediate {
  u32 align_log2;
  u32 offset;
};

struct InitImmediate {
  Index segment_index;
  Index dst_index;
};

struct CopyImmediate {
  Index dst_index;
  Index src_index;
};

struct Instruction {
  explicit Instruction(Opcode opcode);
  explicit Instruction(Opcode opcode, EmptyImmediate);
  explicit Instruction(Opcode opcode, BlockType);
  explicit Instruction(Opcode opcode, Index);
  explicit Instruction(Opcode opcode, CallIndirectImmediate);
  explicit Instruction(Opcode opcode, BrOnExnImmediate);
  explicit Instruction(Opcode opcode, BrTableImmediate);
  explicit Instruction(Opcode opcode, u8);
  explicit Instruction(Opcode opcode, MemArgImmediate);
  explicit Instruction(Opcode opcode, s32);
  explicit Instruction(Opcode opcode, s64);
  explicit Instruction(Opcode opcode, f32);
  explicit Instruction(Opcode opcode, f64);
  explicit Instruction(Opcode opcode, v128);
  explicit Instruction(Opcode opcode, InitImmediate);
  explicit Instruction(Opcode opcode, CopyImmediate);
  explicit Instruction(Opcode opcode, ShuffleImmediate);
  explicit Instruction(Opcode opcode, const ValueTypes&);

  bool has_empty_immediate() const;
  bool has_block_type_immediate() const;
  bool has_index_immediate() const;
  bool has_call_indirect_immediate() const;
  bool has_br_table_immediate() const;
  bool has_br_on_exn_immediate() const;
  bool has_u8_immediate() const;
  bool has_mem_arg_immediate() const;
  bool has_s32_immediate() const;
  bool has_s64_immediate() const;
  bool has_f32_immediate() const;
  bool has_f64_immediate() const;
  bool has_v128_immediate() const;
  bool has_init_immediate() const;
  bool has_copy_immediate() const;
  bool has_shuffle_immediate() const;
  bool has_value_types_immediate() const;

  EmptyImmediate& empty_immediate();
  const EmptyImmediate& empty_immediate() const;
  BlockType& block_type_immediate();
  const BlockType& block_type_immediate() const;
  Index& index_immediate();
  const Index& index_immediate() const;
  CallIndirectImmediate& call_indirect_immediate();
  const CallIndirectImmediate& call_indirect_immediate() const;
  BrTableImmediate& br_table_immediate();
  const BrTableImmediate& br_table_immediate() const;
  BrOnExnImmediate& br_on_exn_immediate();
  const BrOnExnImmediate& br_on_exn_immediate() const;
  u8& u8_immediate();
  const u8& u8_immediate() const;
  MemArgImmediate& mem_arg_immediate();
  const MemArgImmediate& mem_arg_immediate() const;
  s32& s32_immediate();
  const s32& s32_immediate() const;
  s64& s64_immediate();
  const s64& s64_immediate() const;
  f32& f32_immediate();
  const f32& f32_immediate() const;
  f64& f64_immediate();
  const f64& f64_immediate() const;
  v128& v128_immediate();
  const v128& v128_immediate() const;
  InitImmediate& init_immediate();
  const InitImmediate& init_immediate() const;
  CopyImmediate& copy_immediate();
  const CopyImmediate& copy_immediate() const;
  ShuffleImmediate& shuffle_immediate();
  const ShuffleImmediate& shuffle_immediate() const;
  ValueTypes& value_types_immediate();
  const ValueTypes& value_types_immediate() const;

  Opcode opcode;
  variant<EmptyImmediate,
          BlockType,
          Index,
          CallIndirectImmediate,
          BrTableImmediate,
          BrOnExnImmediate,
          u8,
          MemArgImmediate,
          s32,
          s64,
          f32,
          f64,
          v128,
          InitImmediate,
          CopyImmediate,
          ShuffleImmediate,
          ValueTypes>
      immediate;
};


// Section 1: Type

struct FunctionType {
  ValueTypes param_types;
  ValueTypes result_types;
};

struct TypeEntry {
  FunctionType type;
};

// Section 2: Import

struct Limits {
  explicit Limits(u32 min);
  explicit Limits(u32 min, u32 max);
  explicit Limits(u32 min, u32 max, Shared);

  u32 min;
  optional<u32> max;
  Shared shared;
};

struct TableType {
  Limits limits;
  ElementType elemtype;
};

struct MemoryType {
  Limits limits;
};

struct GlobalType {
  ValueType valtype;
  Mutability mut;
};

struct Import {
  ExternalKind kind() const;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;

  Index& index();
  const Index& index() const;
  TableType& table_type();
  const TableType& table_type() const;
  MemoryType& memory_type();
  const MemoryType& memory_type() const;
  GlobalType& global_type();
  const GlobalType& global_type() const;

  string_view module;
  string_view name;
  variant<Index, TableType, MemoryType, GlobalType> desc;
};

// Section 3: Function

struct Function {
  Index type_index;
};

// Section 4: Table

struct Table {
  TableType table_type;
};

// Section 5: Memory

struct Memory {
  MemoryType memory_type;
};

// Section 6: Global

struct ConstantExpression {
  Instruction instruction;
};

struct Global {
  GlobalType global_type;
  ConstantExpression init;
};

// Section 7: Export

struct Export {
  ExternalKind kind;
  string_view name;
  Index index;
};

// Section 8: Start

struct Start {
  Index func_index;
};

// Section 9: Elem

struct ElementExpression {
  Instruction instruction;
};

struct ElementSegment {
  struct Active {
    Index table_index;
    ConstantExpression offset;
    std::vector<Index> init;
  };

  struct Passive {
    ElementType element_type;
    std::vector<ElementExpression> init;
  };

  // Active.
  explicit ElementSegment(Index table_index,
                          ConstantExpression offset,
                          const std::vector<Index>& init);

  // Passive.
  explicit ElementSegment(ElementType element_type,
                          const std::vector<ElementExpression>& init);

  SegmentType segment_type() const;
  bool is_active() const;
  bool is_passive() const;

  Active& active();
  const Active& active() const;
  Passive& passive();
  const Passive& passive() const;

  variant<Active, Passive> desc;
};

// Section 10: Code

struct Locals {
  Index count;
  ValueType type;
};

struct Expression {
  SpanU8 data;
};

struct Code {
  std::vector<Locals> locals;
  Expression body;
};

// Section 11: Data

struct DataSegment {
  struct Active {
    Index memory_index;
    ConstantExpression offset;
  };

  struct Passive {};

  // Active.
  explicit DataSegment(Index memory_index,
                       ConstantExpression offset,
                       SpanU8 init);

  // Passive.
  explicit DataSegment(SpanU8 init);

  SegmentType segment_type() const;
  bool is_active() const;
  bool is_passive() const;

  Active& active();
  const Active& active() const;
  Passive& passive();
  const Passive& passive() const;

  SpanU8 init;
  variant<Active, Passive> desc;
};

// Section 12: DataCount

struct DataCount {
  Index count;
};


#define WASP_TYPES(WASP_V)        \
  WASP_V(BrOnExnImmediate)        \
  WASP_V(BrTableImmediate)        \
  WASP_V(CallIndirectImmediate)   \
  WASP_V(Code)                    \
  WASP_V(ConstantExpression)      \
  WASP_V(CopyImmediate)           \
  WASP_V(CustomSection)           \
  WASP_V(DataCount)               \
  WASP_V(DataSegment)             \
  WASP_V(DataSegment::Active)     \
  WASP_V(DataSegment::Passive)    \
  WASP_V(ElementExpression)       \
  WASP_V(ElementSegment)          \
  WASP_V(ElementSegment::Active)  \
  WASP_V(ElementSegment::Passive) \
  WASP_V(EmptyImmediate)          \
  WASP_V(Export)                  \
  WASP_V(Expression)              \
  WASP_V(Function)                \
  WASP_V(FunctionType)            \
  WASP_V(Global)                  \
  WASP_V(GlobalType)              \
  WASP_V(Import)                  \
  WASP_V(InitImmediate)           \
  WASP_V(Instruction)             \
  WASP_V(KnownSection)            \
  WASP_V(Limits)                  \
  WASP_V(Locals) \
  WASP_V(MemArgImmediate) \
  WASP_V(Memory) \
  WASP_V(MemoryType) \
  WASP_V(Section) \
  WASP_V(Start) \
  WASP_V(Table) \
  WASP_V(TableType) \
  WASP_V(TypeEntry) \

#define WASP_DECLARE_OPERATOR_EQ_NE(Type)    \
  bool operator==(const Type&, const Type&); \
  bool operator!=(const Type&, const Type&);

WASP_TYPES(WASP_DECLARE_OPERATOR_EQ_NE)

#undef WASP_DECLARE_OPERATOR_EQ_NE

}  // namespace binary
}  // namespace wasp

namespace std {

#define WASP_DECLARE_STD_HASH(Type)                       \
  template <>                                             \
  struct hash<::wasp::binary::Type> {                     \
    size_t operator()(const ::wasp::binary::Type&) const; \
  };

WASP_TYPES(WASP_DECLARE_STD_HASH)

#undef WASP_DECLARE_STD_HASH
#undef WASP_TYPES

template <>
struct hash<::wasp::binary::ShuffleImmediate> {
  size_t operator()(const ::wasp::binary::ShuffleImmediate&) const;
};

template <>
struct hash<::wasp::binary::ValueTypes> {
  size_t operator()(const ::wasp::binary::ValueTypes&) const;
};

}  // namespace std

#include "wasp/binary/data_segment-inl.h"
#include "wasp/binary/element_segment-inl.h"
#include "wasp/binary/import-inl.h"
#include "wasp/binary/instruction-inl.h"
#include "wasp/binary/section-inl.h"

#endif // WASP_BINARY_TYPES_H_

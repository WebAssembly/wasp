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

#include "wasp/base/at.h"
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
#include "wasp/binary/def/block_type.def"
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
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/def/element_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class ExternalKind : u8 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/def/external_kind.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class EventAttribute : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/def/event_attribute.def"
#undef WASP_V
};

enum class Mutability : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/def/mutability.def"
#undef WASP_V
};

// The section ids are ordered by their expected order in the binary format.
enum class SectionId : u32 {
#define WASP_V(val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/def/section_id.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class SegmentType {
  Active,
  Passive,
  Declared,
};

enum class Shared {
  No,
  Yes,
};

using ValueTypes = std::vector<At<ValueType>>;
using ShuffleImmediate = std::array<u8, 16>;

// Section

struct KnownSection {
  At<SectionId> id;
  SpanU8 data;
};

struct CustomSection {
  At<string_view> name;
  SpanU8 data;
};

struct Section {
  Section(At<KnownSection>);
  Section(At<CustomSection>);

  // Convenience constructors w/ no Location.
  Section(KnownSection);
  Section(CustomSection);

  bool is_known() const;
  bool is_custom() const;

  auto known() -> At<KnownSection>&;
  auto known() const -> const At<KnownSection>&;
  auto custom() -> At<CustomSection>&;
  auto custom() const -> const At<CustomSection>&;

  At<SectionId> id() const;
  SpanU8 data() const;

  variant<At<KnownSection>, At<CustomSection>> contents;
};


// Instruction

struct EmptyImmediate {};

struct CallIndirectImmediate {
  At<Index> index;
  At<Index> table_index;
};

struct BrOnExnImmediate {
  At<Index> target;
  At<Index> event_index;
};

struct BrTableImmediate {
  std::vector<At<Index>> targets;
  At<Index> default_target;
};

struct MemArgImmediate {
  At<u32> align_log2;
  At<u32> offset;
};

struct InitImmediate {
  At<Index> segment_index;
  At<Index> dst_index;
};

struct CopyImmediate {
  At<Index> dst_index;
  At<Index> src_index;
};

struct Instruction {
  explicit Instruction(At<Opcode>);
  explicit Instruction(At<Opcode>, EmptyImmediate);
  explicit Instruction(At<Opcode>, At<BlockType>);
  explicit Instruction(At<Opcode>, At<Index>);
  explicit Instruction(At<Opcode>, At<CallIndirectImmediate>);
  explicit Instruction(At<Opcode>, At<BrOnExnImmediate>);
  explicit Instruction(At<Opcode>, At<BrTableImmediate>);
  explicit Instruction(At<Opcode>, At<u8>);
  explicit Instruction(At<Opcode>, At<MemArgImmediate>);
  explicit Instruction(At<Opcode>, At<s32>);
  explicit Instruction(At<Opcode>, At<s64>);
  explicit Instruction(At<Opcode>, At<f32>);
  explicit Instruction(At<Opcode>, At<f64>);
  explicit Instruction(At<Opcode>, At<v128>);
  explicit Instruction(At<Opcode>, At<InitImmediate>);
  explicit Instruction(At<Opcode>, At<CopyImmediate>);
  explicit Instruction(At<Opcode>, At<ShuffleImmediate>);
  explicit Instruction(At<Opcode>, const ValueTypes&);

  // Convenience constructors w/ no Location.
  explicit Instruction(Opcode);
  explicit Instruction(Opcode, EmptyImmediate);
  explicit Instruction(Opcode, BlockType);
  explicit Instruction(Opcode, Index);
  explicit Instruction(Opcode, CallIndirectImmediate);
  explicit Instruction(Opcode, BrOnExnImmediate);
  explicit Instruction(Opcode, BrTableImmediate);
  explicit Instruction(Opcode, u8);
  explicit Instruction(Opcode, MemArgImmediate);
  explicit Instruction(Opcode, s32);
  explicit Instruction(Opcode, s64);
  explicit Instruction(Opcode, f32);
  explicit Instruction(Opcode, f64);
  explicit Instruction(Opcode, v128);
  explicit Instruction(Opcode, InitImmediate);
  explicit Instruction(Opcode, CopyImmediate);
  explicit Instruction(Opcode, ShuffleImmediate);
  explicit Instruction(Opcode, const ValueTypes&);

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

  auto empty_immediate() -> EmptyImmediate&;
  auto empty_immediate() const -> const EmptyImmediate&;
  auto block_type_immediate() -> At<BlockType>&;
  auto block_type_immediate() const -> const At<BlockType>&;
  auto index_immediate() -> At<Index>&;
  auto index_immediate() const -> const At<Index>&;
  auto call_indirect_immediate() -> At<CallIndirectImmediate>&;
  auto call_indirect_immediate() const -> const At<CallIndirectImmediate>&;
  auto br_table_immediate() -> At<BrTableImmediate>&;
  auto br_table_immediate() const -> const At<BrTableImmediate>&;
  auto br_on_exn_immediate() -> At<BrOnExnImmediate>&;
  auto br_on_exn_immediate() const -> const At<BrOnExnImmediate>&;
  auto u8_immediate() -> At<u8>&;
  auto u8_immediate() const -> const At<u8>&;
  auto mem_arg_immediate() -> At<MemArgImmediate>&;
  auto mem_arg_immediate() const -> const At<MemArgImmediate>&;
  auto s32_immediate() -> At<s32>&;
  auto s32_immediate() const -> const At<s32>&;
  auto s64_immediate() -> At<s64>&;
  auto s64_immediate() const -> const At<s64>&;
  auto f32_immediate() -> At<f32>&;
  auto f32_immediate() const -> const At<f32>&;
  auto f64_immediate() -> At<f64>&;
  auto f64_immediate() const -> const At<f64>&;
  auto v128_immediate() -> At<v128>&;
  auto v128_immediate() const -> const At<v128>&;
  auto init_immediate() -> At<InitImmediate>&;
  auto init_immediate() const -> const At<InitImmediate>&;
  auto copy_immediate() -> At<CopyImmediate>&;
  auto copy_immediate() const -> const At<CopyImmediate>&;
  auto shuffle_immediate() -> At<ShuffleImmediate>&;
  auto shuffle_immediate() const -> const At<ShuffleImmediate>&;
  auto value_types_immediate() -> ValueTypes&;
  auto value_types_immediate() const -> const ValueTypes&;

  At<Opcode> opcode;
  variant<EmptyImmediate,
          At<BlockType>,
          At<Index>,
          At<CallIndirectImmediate>,
          At<BrTableImmediate>,
          At<BrOnExnImmediate>,
          At<u8>,
          At<MemArgImmediate>,
          At<s32>,
          At<s64>,
          At<f32>,
          At<f64>,
          At<v128>,
          At<InitImmediate>,
          At<CopyImmediate>,
          At<ShuffleImmediate>,
          ValueTypes>
      immediate;
};


// Section 1: Type

struct FunctionType {
  ValueTypes param_types;
  ValueTypes result_types;
};

struct TypeEntry {
  At<FunctionType> type;
};

// Section 2: Import

struct Limits {
  explicit Limits(At<u32> min);
  explicit Limits(At<u32> min, OptAt<u32> max);
  explicit Limits(At<u32> min, OptAt<u32> max, At<Shared>);

  At<u32> min;
  OptAt<u32> max;
  At<Shared> shared;
};

struct TableType {
  At<Limits> limits;
  At<ElementType> elemtype;
};

struct MemoryType {
  At<Limits> limits;
};

struct GlobalType {
  At<ValueType> valtype;
  At<Mutability> mut;
};

struct EventType {
  At<EventAttribute> attribute;
  At<Index> type_index;
};

struct Import {
  explicit Import(At<string_view> module, At<string_view> name, At<Index>);
  explicit Import(At<string_view> module, At<string_view> name, At<TableType>);
  explicit Import(At<string_view> module, At<string_view> name, At<MemoryType>);
  explicit Import(At<string_view> module, At<string_view> name, At<GlobalType>);
  explicit Import(At<string_view> module, At<string_view> name, At<EventType>);

  // Convenience constructors w/ no Location.
  explicit Import(string_view module, string_view name, Index);
  explicit Import(string_view module, string_view name, TableType);
  explicit Import(string_view module, string_view name, MemoryType);
  explicit Import(string_view module, string_view name, GlobalType);
  explicit Import(string_view module, string_view name, EventType);

  ExternalKind kind() const;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;
  bool is_event() const;

  auto index() -> At<Index>&;
  auto index() const -> const At<Index>&;
  auto table_type() -> At<TableType>&;
  auto table_type() const -> const At<TableType>&;
  auto memory_type() -> At<MemoryType>&;
  auto memory_type() const -> const At<MemoryType>&;
  auto global_type() -> At<GlobalType>&;
  auto global_type() const -> const At<GlobalType>&;
  auto event_type() -> At<EventType>&;
  auto event_type() const -> const At<EventType>&;

  At<string_view> module;
  At<string_view> name;
  variant<At<Index>,
          At<TableType>,
          At<MemoryType>,
          At<GlobalType>,
          At<EventType>>
      desc;
};

// Section 3: Function

struct Function {
  At<Index> type_index;
};

// Section 4: Table

struct Table {
  At<TableType> table_type;
};

// Section 5: Memory

struct Memory {
  At<MemoryType> memory_type;
};

// Section 6: Global

struct ConstantExpression {
  At<Instruction> instruction;
};

struct Global {
  At<GlobalType> global_type;
  At<ConstantExpression> init;
};

// Section 7: Export

struct Export {
  Export(At<ExternalKind>, At<string_view>, At<Index>);

  // Convenience constructor w/ no Location.
  Export(ExternalKind, string_view, Index);

  At<ExternalKind> kind;
  At<string_view> name;
  At<Index> index;
};

// Section 8: Start

struct Start {
  At<Index> func_index;
};

// Section 9: Elem

struct ElementExpression {
  At<Instruction> instruction;
};

struct ElementSegment {
  using Indexes = std::vector<At<Index>>;
  using ElementExpressions = std::vector<At<ElementExpression>>;

  struct IndexesInit {
    At<ExternalKind> kind;
    Indexes init;
  };

  struct ExpressionsInit {
    At<ElementType> element_type;
    ElementExpressions init;
  };

  // Active.
  explicit ElementSegment(At<Index> table_index,
                          At<ConstantExpression> offset,
                          At<ExternalKind>,
                          const Indexes& init);
  explicit ElementSegment(At<Index> table_index,
                          At<ConstantExpression> offset,
                          At<ElementType>,
                          const ElementExpressions& init);

  // Passive or declared.
  explicit ElementSegment(SegmentType, At<ExternalKind>, const Indexes& init);
  explicit ElementSegment(SegmentType,
                          At<ElementType>,
                          const ElementExpressions& init);

  bool has_indexes() const;
  bool has_expressions() const;

  auto indexes() -> IndexesInit&;
  auto indexes() const -> const IndexesInit&;
  auto expressions() -> ExpressionsInit&;
  auto expressions() const -> const ExpressionsInit&;

  auto elemtype() const -> At<ElementType>;

  SegmentType type;
  OptAt<Index> table_index;
  OptAt<ConstantExpression> offset;
  variant<IndexesInit, ExpressionsInit> desc;
};

// Section 10: Code

struct Locals {
  At<Index> count;
  At<ValueType> type;
};

struct Expression {
  SpanU8 data;
};

struct Code {
  std::vector<At<Locals>> locals;
  At<Expression> body;
};

// Section 11: Data

struct DataSegment {
  // Active.
  explicit DataSegment(OptAt<Index> memory_index,
                       OptAt<ConstantExpression> offset,
                       SpanU8 init);

  // Passive.
  explicit DataSegment(SpanU8 init);

  SegmentType type;
  OptAt<Index> memory_index;
  OptAt<ConstantExpression> offset;
  SpanU8 init;
};

// Section 12: DataCount

struct DataCount {
  At<Index> count;
};

// Section 13: Event

struct Event {
  At<EventType> event_type;
};

#define WASP_TYPES(WASP_V)                \
  WASP_V(BrOnExnImmediate)                \
  WASP_V(BrTableImmediate)                \
  WASP_V(CallIndirectImmediate)           \
  WASP_V(Code)                            \
  WASP_V(ConstantExpression)              \
  WASP_V(CopyImmediate)                   \
  WASP_V(CustomSection)                   \
  WASP_V(DataCount)                       \
  WASP_V(DataSegment)                     \
  WASP_V(ElementExpression)               \
  WASP_V(ElementSegment)                  \
  WASP_V(ElementSegment::IndexesInit)     \
  WASP_V(ElementSegment::ExpressionsInit) \
  WASP_V(EmptyImmediate)                  \
  WASP_V(Event)                           \
  WASP_V(EventType)                       \
  WASP_V(Export)                          \
  WASP_V(Expression)                      \
  WASP_V(Function)                        \
  WASP_V(FunctionType)                    \
  WASP_V(Global)                          \
  WASP_V(GlobalType)                      \
  WASP_V(Import)                          \
  WASP_V(InitImmediate)                   \
  WASP_V(Instruction)                     \
  WASP_V(KnownSection)                    \
  WASP_V(Limits)                          \
  WASP_V(Locals)                          \
  WASP_V(MemArgImmediate)                 \
  WASP_V(Memory)                          \
  WASP_V(MemoryType)                      \
  WASP_V(Section)                         \
  WASP_V(Start)                           \
  WASP_V(Table)                           \
  WASP_V(TableType)                       \
  WASP_V(TypeEntry)

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

#include "wasp/binary/types-inl.h"

#endif // WASP_BINARY_TYPES_H_

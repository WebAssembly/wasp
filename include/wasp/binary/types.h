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
#include <iosfwd>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/print_to_macros.h"
#include "wasp/base/span.h"
#include "wasp/base/std_hash_macros.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"

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

// The section ids are ordered by their expected order in the binary format.
enum class SectionId : u32 {
#define WASP_V(val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/def/section_id.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

using IndexList = std::vector<At<Index>>;

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

struct BrOnExnImmediate {
  At<Index> target;
  At<Index> event_index;
};

struct BrTableImmediate {
  IndexList targets;
  At<Index> default_target;
};

struct CallIndirectImmediate {
  At<Index> index;
  At<Index> table_index;
};

struct CopyImmediate {
  At<Index> dst_index;
  At<Index> src_index;
};

struct InitImmediate {
  At<Index> segment_index;
  At<Index> dst_index;
};

struct MemArgImmediate {
  At<u32> align_log2;
  At<u32> offset;
};

using SelectImmediate = ValueTypeList;
using SimdLaneImmediate = u8;

struct Instruction {
  explicit Instruction(At<Opcode>);
  explicit Instruction(At<Opcode>, At<s32>);
  explicit Instruction(At<Opcode>, At<s64>);
  explicit Instruction(At<Opcode>, At<f32>);
  explicit Instruction(At<Opcode>, At<f64>);
  explicit Instruction(At<Opcode>, At<v128>);
  explicit Instruction(At<Opcode>, At<Index>);
  explicit Instruction(At<Opcode>, At<BlockType>);
  explicit Instruction(At<Opcode>, At<BrOnExnImmediate>);
  explicit Instruction(At<Opcode>, At<BrTableImmediate>);
  explicit Instruction(At<Opcode>, At<CallIndirectImmediate>);
  explicit Instruction(At<Opcode>, At<CopyImmediate>);
  explicit Instruction(At<Opcode>, At<InitImmediate>);
  explicit Instruction(At<Opcode>, At<MemArgImmediate>);
  explicit Instruction(At<Opcode>, At<ReferenceType>);
  explicit Instruction(At<Opcode>, At<SelectImmediate>);
  explicit Instruction(At<Opcode>, At<ShuffleImmediate>);
  explicit Instruction(At<Opcode>, At<SimdLaneImmediate>);

  // Convenience constructors w/ no Location for numeric types (since the
  // implicit conversions to At<T> doesn't work properly for these types).
  // These are primarily used for tests.
  explicit Instruction(Opcode, s32);
  explicit Instruction(Opcode, s64);
  explicit Instruction(Opcode, f32);
  explicit Instruction(Opcode, f64);
  explicit Instruction(Opcode, Index);
  explicit Instruction(Opcode, SimdLaneImmediate);

  bool has_no_immediate() const;
  bool has_s32_immediate() const;
  bool has_s64_immediate() const;
  bool has_f32_immediate() const;
  bool has_f64_immediate() const;
  bool has_v128_immediate() const;
  bool has_index_immediate() const;
  bool has_block_type_immediate() const;
  bool has_br_on_exn_immediate() const;
  bool has_br_table_immediate() const;
  bool has_call_indirect_immediate() const;
  bool has_copy_immediate() const;
  bool has_init_immediate() const;
  bool has_mem_arg_immediate() const;
  bool has_reference_type_immediate() const;
  bool has_select_immediate() const;
  bool has_shuffle_immediate() const;
  bool has_simd_lane_immediate() const;

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
  auto index_immediate() -> At<Index>&;
  auto index_immediate() const -> const At<Index>&;
  auto block_type_immediate() -> At<BlockType>&;
  auto block_type_immediate() const -> const At<BlockType>&;
  auto br_on_exn_immediate() -> At<BrOnExnImmediate>&;
  auto br_on_exn_immediate() const -> const At<BrOnExnImmediate>&;
  auto br_table_immediate() -> At<BrTableImmediate>&;
  auto br_table_immediate() const -> const At<BrTableImmediate>&;
  auto call_indirect_immediate() -> At<CallIndirectImmediate>&;
  auto call_indirect_immediate() const -> const At<CallIndirectImmediate>&;
  auto copy_immediate() -> At<CopyImmediate>&;
  auto copy_immediate() const -> const At<CopyImmediate>&;
  auto init_immediate() -> At<InitImmediate>&;
  auto init_immediate() const -> const At<InitImmediate>&;
  auto mem_arg_immediate() -> At<MemArgImmediate>&;
  auto mem_arg_immediate() const -> const At<MemArgImmediate>&;
  auto reference_type_immediate() -> At<ReferenceType>&;
  auto reference_type_immediate() const -> const At<ReferenceType>&;
  auto select_immediate() -> At<SelectImmediate>&;
  auto select_immediate() const -> const At<SelectImmediate>&;
  auto shuffle_immediate() -> At<ShuffleImmediate>&;
  auto shuffle_immediate() const -> const At<ShuffleImmediate>&;
  auto simd_lane_immediate() -> At<SimdLaneImmediate>&;
  auto simd_lane_immediate() const -> const At<SimdLaneImmediate>&;

  At<Opcode> opcode;
  variant<monostate,
          At<s32>,
          At<s64>,
          At<f32>,
          At<f64>,
          At<v128>,
          At<Index>,
          At<BlockType>,
          At<BrOnExnImmediate>,
          At<BrTableImmediate>,
          At<CallIndirectImmediate>,
          At<CopyImmediate>,
          At<InitImmediate>,
          At<MemArgImmediate>,
          At<ReferenceType>,
          At<SelectImmediate>,
          At<ShuffleImmediate>,
          At<SimdLaneImmediate>>
      immediate;
};


// Section 1: Type

struct FunctionType {
  ValueTypeList param_types;
  ValueTypeList result_types;
};

struct TypeEntry {
  At<FunctionType> type;
};

// Section 2: Import

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

using ElementExpressionList = std::vector<At<ElementExpression>>;

struct ElementListWithExpressions {
  At<ReferenceType> elemtype;
  ElementExpressionList list;
};

struct ElementListWithIndexes {
  At<ExternalKind> kind;
  IndexList list;
};

using ElementList = variant<ElementListWithIndexes, ElementListWithExpressions>;

struct ElementSegment {
  // Active.
  explicit ElementSegment(At<Index> table_index,
                          At<ConstantExpression> offset,
                          const ElementList&);

  // Passive or declared.
  explicit ElementSegment(SegmentType, const ElementList&);

  bool has_indexes() const;
  bool has_expressions() const;

  auto indexes() -> ElementListWithIndexes&;
  auto indexes() const -> const ElementListWithIndexes&;
  auto expressions() -> ElementListWithExpressions&;
  auto expressions() const -> const ElementListWithExpressions&;

  auto elemtype() const -> At<ReferenceType>;

  SegmentType type;
  OptAt<Index> table_index;
  OptAt<ConstantExpression> offset;
  ElementList elements;
};

// Section 10: Code

struct Locals {
  At<Index> count;
  At<ValueType> type;
};

using LocalsList = std::vector<At<Locals>>;

struct Expression {
  SpanU8 data;
};

struct Code {
  LocalsList locals;
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

#define WASP_BINARY_ENUMS(WASP_V) \
  WASP_V(binary::BlockType)       \
  WASP_V(binary::SectionId)

#define WASP_BINARY_STRUCTS(WASP_V)                                      \
  WASP_V(binary::BrOnExnImmediate, 2, target, event_index)               \
  WASP_V(binary::BrTableImmediate, 2, targets, default_target)           \
  WASP_V(binary::CallIndirectImmediate, 2, index, table_index)           \
  WASP_V(binary::Code, 2, locals, body)                                  \
  WASP_V(binary::ConstantExpression, 1, instruction)                     \
  WASP_V(binary::CopyImmediate, 2, src_index, dst_index)                 \
  WASP_V(binary::CustomSection, 2, name, data)                           \
  WASP_V(binary::DataCount, 1, count)                                    \
  WASP_V(binary::DataSegment, 4, type, memory_index, offset, init)       \
  WASP_V(binary::ElementExpression, 1, instruction)                      \
  WASP_V(binary::ElementSegment, 4, type, table_index, offset, elements) \
  WASP_V(binary::ElementListWithIndexes, 2, kind, list)                  \
  WASP_V(binary::ElementListWithExpressions, 2, elemtype, list)          \
  WASP_V(binary::Event, 1, event_type)                                   \
  WASP_V(binary::EventType, 2, attribute, type_index)                    \
  WASP_V(binary::Export, 3, kind, name, index)                           \
  WASP_V(binary::Expression, 1, data)                                    \
  WASP_V(binary::Function, 1, type_index)                                \
  WASP_V(binary::FunctionType, 2, param_types, result_types)             \
  WASP_V(binary::Global, 2, global_type, init)                           \
  WASP_V(binary::Import, 3, module, name, desc)                          \
  WASP_V(binary::InitImmediate, 2, segment_index, dst_index)             \
  WASP_V(binary::Instruction, 2, opcode, immediate)                      \
  WASP_V(binary::KnownSection, 2, id, data)                              \
  WASP_V(binary::Locals, 2, count, type)                                 \
  WASP_V(binary::MemArgImmediate, 2, align_log2, offset)                 \
  WASP_V(binary::Memory, 1, memory_type)                                 \
  WASP_V(binary::Section, 1, contents)                                   \
  WASP_V(binary::Start, 1, func_index)                                   \
  WASP_V(binary::Table, 1, table_type)                                   \
  WASP_V(binary::TypeEntry, 1, type)

#define WASP_BINARY_CONTAINERS(WASP_V) \
  WASP_V(binary::IndexList)            \
  WASP_V(binary::LocalsList)           \
  WASP_V(binary::ElementExpressionList)

WASP_BINARY_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_BINARY_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

// Used for gtest.

WASP_BINARY_ENUMS(WASP_DECLARE_PRINT_TO)
WASP_BINARY_STRUCTS(WASP_DECLARE_PRINT_TO)

}  // namespace binary
}  // namespace wasp

WASP_BINARY_STRUCTS(WASP_DECLARE_STD_HASH)
WASP_BINARY_CONTAINERS(WASP_DECLARE_STD_HASH)

#include "wasp/binary/types-inl.h"

#endif // WASP_BINARY_TYPES_H_

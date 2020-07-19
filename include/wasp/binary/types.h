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

struct HeapType {
  explicit HeapType(HeapKind);
  explicit HeapType(Index);
  static HeapType Func();
  static HeapType Extern();
  static HeapType Exn();

  bool is_heap_kind() const;
  bool is_func() const;
  bool is_extern() const;
  bool is_exn() const;
  bool is_index() const;

  auto heap_kind() -> HeapKind&;
  auto heap_kind() const -> const HeapKind&;
  auto index() -> Index&;
  auto index() const -> const Index&;

  variant<HeapKind, Index> type;
};

struct RefType {
  HeapType heap_type;
  Null null;
};

struct ReferenceType {
  explicit ReferenceType(ReferenceKind);
  explicit ReferenceType(RefType);
  static ReferenceType Funcref();
  static ReferenceType Externref();
  static ReferenceType Exnref();

  bool is_reference_kind() const;
  bool is_funcref() const;
  bool is_externref() const;
  bool is_exnref() const;
  bool is_ref() const;

  auto reference_kind() -> ReferenceKind&;
  auto reference_kind() const -> const ReferenceKind&;
  auto ref() -> RefType&;
  auto ref() const -> const RefType&;

  variant<ReferenceKind, RefType> type;
};

struct ValueType {
  explicit ValueType(NumericType);
  explicit ValueType(ReferenceType);
  static ValueType I32();
  static ValueType I64();
  static ValueType F32();
  static ValueType F64();
  static ValueType V128();
  static ValueType Funcref();
  static ValueType Externref();
  static ValueType Exnref();

  bool is_numeric_type() const;
  bool is_reference_type() const;

  auto numeric_type() -> NumericType&;
  auto numeric_type() const -> const NumericType&;
  auto reference_type() -> ReferenceType&;
  auto reference_type() const -> const ReferenceType&;

  variant<NumericType, ReferenceType> type;
};

using ValueTypeList = std::vector<At<ValueType>>;

struct VoidType {};
struct BlockType {
  explicit BlockType(ValueType);
  explicit BlockType(VoidType);
  explicit BlockType(Index);
  static BlockType I32();
  static BlockType I64();
  static BlockType F32();
  static BlockType F64();
  static BlockType V128();
  static BlockType Void();
  static BlockType Funcref();
  static BlockType Externref();
  static BlockType Exnref();

  bool is_value_type() const;
  bool is_void() const;
  bool is_index() const;

  auto value_type() -> ValueType&;
  auto value_type() const -> const ValueType&;
  auto index() -> Index&;
  auto index() const -> const Index&;

  variant<ValueType, VoidType, Index> type;
};

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

struct Locals {
  At<Index> count;
  At<ValueType> type;
};

using LocalsList = std::vector<At<Locals>>;

struct LetImmediate {
  At<BlockType> block_type;
  LocalsList locals;
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
  explicit Instruction(At<Opcode>, At<LetImmediate>);
  explicit Instruction(At<Opcode>, At<MemArgImmediate>);
  explicit Instruction(At<Opcode>, At<HeapType>);
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
  bool has_let_immediate() const;
  bool has_mem_arg_immediate() const;
  bool has_heap_type_immediate() const;
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
  auto let_immediate() -> At<LetImmediate>&;
  auto let_immediate() const -> const At<LetImmediate>&;
  auto mem_arg_immediate() -> At<MemArgImmediate>&;
  auto mem_arg_immediate() const -> const At<MemArgImmediate>&;
  auto heap_type_immediate() -> At<HeapType>&;
  auto heap_type_immediate() const -> const At<HeapType>&;
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
          At<LetImmediate>,
          At<MemArgImmediate>,
          At<HeapType>,
          At<SelectImmediate>,
          At<ShuffleImmediate>,
          At<SimdLaneImmediate>>
      immediate;
};

using InstructionList = std::vector<At<Instruction>>;

// Section 1: Type

struct FunctionType {
  ValueTypeList param_types;
  ValueTypeList result_types;
};

struct TypeEntry {
  At<FunctionType> type;
};

// Section 2: Import

struct TableType {
  At<Limits> limits;
  At<ReferenceType> elemtype;
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
  explicit ConstantExpression() = default;
  explicit ConstantExpression(const At<Instruction>&);
  explicit ConstantExpression(const InstructionList&);

  InstructionList instructions;
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
  explicit ElementExpression() = default;
  explicit ElementExpression(const At<Instruction>&);
  explicit ElementExpression(const InstructionList&);

  InstructionList instructions;
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

struct Expression {
  SpanU8 data;
};

struct Code {
  LocalsList locals;
  At<Expression> body;
};

struct UnpackedExpression {
  InstructionList instructions;
};

struct UnpackedCode {
  LocalsList locals;
  UnpackedExpression body;
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

// Module
// (primarily used as a container when converting from text. For binary
// decoding, it's more efficient to lazily decode sections.)

struct Module {
  std::vector<At<TypeEntry>> types;
  std::vector<At<Import>> imports;
  std::vector<At<Function>> functions;
  std::vector<At<Table>> tables;
  std::vector<At<Memory>> memories;
  std::vector<At<Global>> globals;
  std::vector<At<Event>> events;
  std::vector<At<Export>> exports;
  optional<At<Start>> start;
  std::vector<At<ElementSegment>> element_segments;
  optional<At<DataCount>> data_count;
  std::vector<At<UnpackedCode>> codes;
  std::vector<At<DataSegment>> data_segments;
};

#define WASP_BINARY_ENUMS(WASP_V) \
  WASP_V(binary::SectionId)

#define WASP_BINARY_STRUCTS_CUSTOM_FORMAT(WASP_V)                        \
  WASP_V(binary::BlockType, 1, type)                                     \
  WASP_V(binary::BrOnExnImmediate, 2, target, event_index)               \
  WASP_V(binary::BrTableImmediate, 2, targets, default_target)           \
  WASP_V(binary::CallIndirectImmediate, 2, index, table_index)           \
  WASP_V(binary::Code, 2, locals, body)                                  \
  WASP_V(binary::ConstantExpression, 1, instructions)                    \
  WASP_V(binary::CopyImmediate, 2, src_index, dst_index)                 \
  WASP_V(binary::CustomSection, 2, name, data)                           \
  WASP_V(binary::DataCount, 1, count)                                    \
  WASP_V(binary::DataSegment, 4, type, memory_index, offset, init)       \
  WASP_V(binary::ElementExpression, 1, instructions)                     \
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
  WASP_V(binary::GlobalType, 2, valtype, mut)                            \
  WASP_V(binary::HeapType, 1, type)                                      \
  WASP_V(binary::Import, 3, module, name, desc)                          \
  WASP_V(binary::InitImmediate, 2, segment_index, dst_index)             \
  WASP_V(binary::Instruction, 2, opcode, immediate)                      \
  WASP_V(binary::KnownSection, 2, id, data)                              \
  WASP_V(binary::LetImmediate, 2, block_type, locals)                    \
  WASP_V(binary::Locals, 2, count, type)                                 \
  WASP_V(binary::MemArgImmediate, 2, align_log2, offset)                 \
  WASP_V(binary::Memory, 1, memory_type)                                 \
  WASP_V(binary::RefType, 2, heap_type, null)                            \
  WASP_V(binary::ReferenceType, 1, type)                                 \
  WASP_V(binary::Section, 1, contents)                                   \
  WASP_V(binary::Start, 1, func_index)                                   \
  WASP_V(binary::Table, 1, table_type)                                   \
  WASP_V(binary::TableType, 2, limits, elemtype)                         \
  WASP_V(binary::TypeEntry, 1, type)                                     \
  WASP_V(binary::UnpackedCode, 2, locals, body)                          \
  WASP_V(binary::UnpackedExpression, 1, instructions)                    \
  WASP_V(binary::ValueType, 1, type)                                     \
  WASP_V(binary::VoidType, 0)

#define WASP_BINARY_CONTAINERS(WASP_V)  \
  WASP_V(binary::IndexList)             \
  WASP_V(binary::InstructionList)       \
  WASP_V(binary::LocalsList)            \
  WASP_V(binary::ElementExpressionList) \
  WASP_V(binary::ValueTypeList)

WASP_BINARY_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_BINARY_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

WASP_DECLARE_OPERATOR_EQ_NE(binary::Module)

// Used for gtest.

WASP_BINARY_ENUMS(WASP_DECLARE_PRINT_TO)
WASP_BINARY_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_PRINT_TO)

}  // namespace binary
}  // namespace wasp

WASP_BINARY_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_STD_HASH)
WASP_BINARY_CONTAINERS(WASP_DECLARE_STD_HASH)

#endif // WASP_BINARY_TYPES_H_

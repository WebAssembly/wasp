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

#ifndef WASP_TEXT_TYPES_H_
#define WASP_TEXT_TYPES_H_

#include <iosfwd>
#include <utility>

#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/std_hash_macros.h"
#include "wasp/base/string_view.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"
#include "wasp/text/read/token.h"

namespace wasp::text {

struct Var {
  bool is_index() const;
  bool is_name() const;

  auto index() -> Index&;
  auto index() const -> const Index&;
  auto name() -> string_view&;
  auto name() const -> const string_view&;

  variant<Index, string_view> desc;
};

struct HeapType {
  explicit HeapType(At<HeapKind>);
  explicit HeapType(At<Var>);

  bool is_heap_kind() const;
  bool is_var() const;

  auto heap_kind() -> At<HeapKind>&;
  auto heap_kind() const -> const At<HeapKind>&;
  auto var() -> At<Var>&;
  auto var() const -> const At<Var>&;

  variant<At<HeapKind>, At<Var>> type;
};

struct RefType {
  At<HeapType> heap_type;
  Null null;
};

struct ReferenceType {
  explicit ReferenceType(At<ReferenceKind>);
  explicit ReferenceType(At<RefType>);
  static ReferenceType Funcref_NoLocation();
  static ReferenceType Externref_NoLocation();
  static ReferenceType Exnref_NoLocation();

  bool is_reference_kind() const;
  bool is_ref() const;

  auto reference_kind() -> At<ReferenceKind>&;
  auto reference_kind() const -> const At<ReferenceKind>&;
  auto ref() -> At<RefType>&;
  auto ref() const -> const At<RefType>&;

  variant<At<ReferenceKind>, At<RefType>> type;
};

struct Rtt {
  At<Index> depth;
  At<HeapType> type;
};

struct ValueType {
  explicit ValueType(At<NumericType>);
  explicit ValueType(At<ReferenceType>);
  explicit ValueType(At<Rtt>);
  static ValueType I32_NoLocation();
  static ValueType I64_NoLocation();
  static ValueType F32_NoLocation();
  static ValueType F64_NoLocation();
  static ValueType V128_NoLocation();
  static ValueType Funcref_NoLocation();
  static ValueType Externref_NoLocation();
  static ValueType Exnref_NoLocation();

  bool is_numeric_type() const;
  bool is_reference_type() const;
  bool is_rtt() const;

  auto numeric_type() -> At<NumericType>&;
  auto numeric_type() const -> const At<NumericType>&;
  auto reference_type() -> At<ReferenceType>&;
  auto reference_type() const -> const At<ReferenceType>&;
  auto rtt() -> At<Rtt>&;
  auto rtt() const -> const At<Rtt>&;

  variant<At<NumericType>, At<ReferenceType>, At<Rtt>> type;
};

using ValueTypeList = std::vector<At<ValueType>>;

struct StorageType {
  explicit StorageType(At<ValueType>);
  explicit StorageType(At<PackedType>);

  bool is_value_type() const;
  bool is_packed_type() const;

  auto value_type() -> At<ValueType>&;
  auto value_type() const -> const At<ValueType>&;
  auto packed_type() -> At<PackedType>&;
  auto packed_type() const -> const At<PackedType>&;

  variant<At<ValueType>, At<PackedType>> type;
};

using VarList = std::vector<At<Var>>;
using BindVar = string_view;

using TextList = std::vector<At<Text>>;

void ToBuffer(const TextList&, Buffer& buffer);

struct FunctionType {
  ValueTypeList params;
  ValueTypeList results;
};

struct FunctionTypeUse {
  bool IsInlineType() const;
  OptAt<ValueType> GetInlineType() const;

  OptAt<Var> type_use;
  At<FunctionType> type;
};

struct BlockImmediate {
  OptAt<BindVar> label;
  FunctionTypeUse type;
};

struct HeapType2Immediate {
  At<HeapType> parent;
  At<HeapType> child;
};

struct BrOnCastImmediate {
  At<Var> target;
  HeapType2Immediate types;
};

struct BrOnExnImmediate {
  At<Var> target;
  At<Var> event;
};

struct BrTableImmediate {
  VarList targets;
  At<Var> default_target;
};

struct CallIndirectImmediate {
  OptAt<Var> table;
  FunctionTypeUse type;
};

struct CopyImmediate {
  OptAt<Var> dst;
  OptAt<Var> src;
};

using FuncBindImmediate = FunctionTypeUse;

struct InitImmediate {
  At<Var> segment;
  OptAt<Var> dst;
};

struct BoundValueType {
  OptAt<BindVar> name;
  At<ValueType> type;
};

using BoundValueTypeList = std::vector<At<BoundValueType>>;

struct LetImmediate {
  BlockImmediate block;
  BoundValueTypeList locals;
};

struct MemArgImmediate {
  OptAt<u32> align;
  OptAt<u32> offset;
};

struct RttSubImmediate {
  At<Index> depth;
  HeapType2Immediate types;
};

using SelectImmediate = ValueTypeList;
using SimdLaneImmediate = u8;

struct StructFieldImmediate {
  At<Var> struct_;
  At<Var> field;
};

// NOTE this must be kept in sync with the Instruction variant below.
enum class InstructionKind {
  None,
  S32,
  S64,
  F32,
  F64,
  V128,
  Var,
  Block,
  BrOnExn,
  BrTable,
  CallIndirect,
  Copy,
  Init,
  Let,
  MemArg,
  HeapType,
  Select,
  Shuffle,
  SimdLane,
  FuncBind,
  BrOnCast,
  HeapType2,
  RttSub,
  StructField,
};

struct Instruction {
  explicit Instruction(At<Opcode>);
  explicit Instruction(At<Opcode>, At<s32>);
  explicit Instruction(At<Opcode>, At<s64>);
  explicit Instruction(At<Opcode>, At<f32>);
  explicit Instruction(At<Opcode>, At<f64>);
  explicit Instruction(At<Opcode>, At<v128>);
  explicit Instruction(At<Opcode>, At<Var>);
  explicit Instruction(At<Opcode>, At<BlockImmediate>);
  explicit Instruction(At<Opcode>, At<BrOnCastImmediate>);
  explicit Instruction(At<Opcode>, At<BrOnExnImmediate>);
  explicit Instruction(At<Opcode>, At<BrTableImmediate>);
  explicit Instruction(At<Opcode>, At<CallIndirectImmediate>);
  explicit Instruction(At<Opcode>, At<CopyImmediate>);
  explicit Instruction(At<Opcode>, At<FuncBindImmediate>);
  explicit Instruction(At<Opcode>, At<HeapType>);
  explicit Instruction(At<Opcode>, At<HeapType2Immediate>);
  explicit Instruction(At<Opcode>, At<InitImmediate>);
  explicit Instruction(At<Opcode>, At<LetImmediate>);
  explicit Instruction(At<Opcode>, At<MemArgImmediate>);
  explicit Instruction(At<Opcode>, At<RttSubImmediate>);
  explicit Instruction(At<Opcode>, At<SelectImmediate>);
  explicit Instruction(At<Opcode>, At<ShuffleImmediate>);
  explicit Instruction(At<Opcode>, At<SimdLaneImmediate>);
  explicit Instruction(At<Opcode>, At<StructFieldImmediate>);

  // Convenience constructors w/ no Location for numeric types (since the
  // implicit conversions to At<T> doesn't work properly for these types).
  // These are primarily used for tests.
  explicit Instruction(Opcode, s32);
  explicit Instruction(Opcode, s64);
  explicit Instruction(Opcode, f32);
  explicit Instruction(Opcode, f64);
  explicit Instruction(Opcode, SimdLaneImmediate);

  InstructionKind kind() const;
  bool has_no_immediate() const;
  bool has_s32_immediate() const;
  bool has_s64_immediate() const;
  bool has_f32_immediate() const;
  bool has_f64_immediate() const;
  bool has_v128_immediate() const;
  bool has_var_immediate() const;
  bool has_block_immediate() const;
  bool has_br_on_cast_immediate() const;
  bool has_br_on_exn_immediate() const;
  bool has_br_table_immediate() const;
  bool has_call_indirect_immediate() const;
  bool has_copy_immediate() const;
  bool has_func_bind_immediate() const;
  bool has_heap_type_immediate() const;
  bool has_heap_type_2_immediate() const;
  bool has_init_immediate() const;
  bool has_let_immediate() const;
  bool has_mem_arg_immediate() const;
  bool has_rtt_sub_immediate() const;
  bool has_select_immediate() const;
  bool has_shuffle_immediate() const;
  bool has_simd_lane_immediate() const;
  bool has_struct_field_immediate() const;

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
  auto var_immediate() -> At<Var>&;
  auto var_immediate() const -> const At<Var>&;
  auto block_immediate() -> At<BlockImmediate>&;
  auto block_immediate() const -> const At<BlockImmediate>&;
  auto br_on_cast_immediate() -> At<BrOnCastImmediate>&;
  auto br_on_cast_immediate() const -> const At<BrOnCastImmediate>&;
  auto br_on_exn_immediate() -> At<BrOnExnImmediate>&;
  auto br_on_exn_immediate() const -> const At<BrOnExnImmediate>&;
  auto br_table_immediate() -> At<BrTableImmediate>&;
  auto br_table_immediate() const -> const At<BrTableImmediate>&;
  auto call_indirect_immediate() -> At<CallIndirectImmediate>&;
  auto call_indirect_immediate() const -> const At<CallIndirectImmediate>&;
  auto copy_immediate() -> At<CopyImmediate>&;
  auto copy_immediate() const -> const At<CopyImmediate>&;
  auto func_bind_immediate() -> At<FuncBindImmediate>&;
  auto func_bind_immediate() const -> const At<FuncBindImmediate>&;
  auto heap_type_immediate() -> At<HeapType>&;
  auto heap_type_immediate() const -> const At<HeapType>&;
  auto heap_type_2_immediate() -> At<HeapType2Immediate>&;
  auto heap_type_2_immediate() const -> const At<HeapType2Immediate>&;
  auto init_immediate() -> At<InitImmediate>&;
  auto init_immediate() const -> const At<InitImmediate>&;
  auto let_immediate() -> At<LetImmediate>&;
  auto let_immediate() const -> const At<LetImmediate>&;
  auto mem_arg_immediate() -> At<MemArgImmediate>&;
  auto mem_arg_immediate() const -> const At<MemArgImmediate>&;
  auto rtt_sub_immediate() -> At<RttSubImmediate>&;
  auto rtt_sub_immediate() const -> const At<RttSubImmediate>&;
  auto select_immediate() -> At<SelectImmediate>&;
  auto select_immediate() const -> const At<SelectImmediate>&;
  auto shuffle_immediate() -> At<ShuffleImmediate>&;
  auto shuffle_immediate() const -> const At<ShuffleImmediate>&;
  auto simd_lane_immediate() -> At<SimdLaneImmediate>&;
  auto simd_lane_immediate() const -> const At<SimdLaneImmediate>&;
  auto struct_field_immediate() -> At<StructFieldImmediate>&;
  auto struct_field_immediate() const -> const At<StructFieldImmediate>&;

  At<Opcode> opcode;
  variant<monostate,
          At<s32>,
          At<s64>,
          At<f32>,
          At<f64>,
          At<v128>,
          At<Var>,
          At<BlockImmediate>,
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
          At<SimdLaneImmediate>,
          At<FuncBindImmediate>,
          At<BrOnCastImmediate>,
          At<HeapType2Immediate>,
          At<RttSubImmediate>,
          At<StructFieldImmediate>>
      immediate;
};

using InstructionList = std::vector<At<Instruction>>;

// Section 1: Type

struct BoundFunctionType {
  BoundValueTypeList params;
  ValueTypeList results;
};

FunctionType ToFunctionType(BoundFunctionType);
BoundFunctionType ToBoundFunctionType(FunctionType);

struct FieldType {
  OptAt<BindVar> name;
  At<StorageType> type;
  At<Mutability> mut;
};

using FieldTypeList = std::vector<At<FieldType>>;

struct StructType {
  FieldTypeList fields;
};

struct ArrayType {
  At<FieldType> field;
};

struct DefinedType {
  explicit DefinedType(OptAt<BindVar>, At<BoundFunctionType>);
  explicit DefinedType(OptAt<BindVar>, At<StructType>);
  explicit DefinedType(OptAt<BindVar>, At<ArrayType>);

  bool is_function_type() const;
  bool is_struct_type() const;
  bool is_array_type() const;

  auto function_type() -> At<BoundFunctionType>&;
  auto function_type() const -> const At<BoundFunctionType>&;
  auto struct_type() -> At<StructType>&;
  auto struct_type() const -> const At<StructType>&;
  auto array_type() -> At<ArrayType>&;
  auto array_type() const -> const At<ArrayType>&;

  OptAt<BindVar> name;
  variant<At<BoundFunctionType>, At<StructType>, At<ArrayType>> type;
};

// Section 2: Import

struct FunctionDesc {
  OptAt<BindVar> name;
  // Not using FunctionTypeUse, since that doesn't allow for bound params.
  OptAt<Var> type_use;
  At<BoundFunctionType> type;
};

struct TableType {
  At<Limits> limits;
  At<ReferenceType> elemtype;
};

struct TableDesc {
  OptAt<BindVar> name;
  At<TableType> type;
};

struct MemoryDesc {
  OptAt<BindVar> name;
  At<MemoryType> type;
};

struct GlobalType {
  At<ValueType> valtype;
  At<Mutability> mut;
};

struct GlobalDesc {
  OptAt<BindVar> name;
  At<GlobalType> type;
};

struct EventType {
  EventAttribute attribute;
  FunctionTypeUse type;
};

struct EventDesc {
  OptAt<BindVar> name;
  At<EventType> type;
};

struct Import {
  At<Text> module;
  At<Text> name;

  auto kind() const -> ExternalKind;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;
  bool is_event() const;

  auto function_desc() -> FunctionDesc&;
  auto function_desc() const -> const FunctionDesc&;
  auto table_desc() -> TableDesc&;
  auto table_desc() const -> const TableDesc&;
  auto memory_desc() -> MemoryDesc&;
  auto memory_desc() const -> const MemoryDesc&;
  auto global_desc() -> GlobalDesc&;
  auto global_desc() const -> const GlobalDesc&;
  auto event_desc() -> EventDesc&;
  auto event_desc() const -> const EventDesc&;

  // NOTE: variant order must be kept in sync with ExternalKind enum.
  variant<FunctionDesc, TableDesc, MemoryDesc, GlobalDesc, EventDesc> desc;
};

struct InlineImport {
  At<Text> module;
  At<Text> name;
};

// Section 3: Function

struct InlineExport {
  At<Text> name;
};

using InlineExportList = std::vector<At<InlineExport>>;

struct Export;

using ExportList = std::vector<At<Export>>;

struct Function {
  // Empty function.
  explicit Function() = default;

  // Defined function.
  explicit Function(const FunctionDesc&,
                    const BoundValueTypeList& locals,
                    const InstructionList&,
                    const InlineExportList&);

  // Imported function.
  explicit Function(const FunctionDesc&,
                    const At<InlineImport>&,
                    const InlineExportList&);

  // Imported or defined.
  explicit Function(const FunctionDesc&,
                    const BoundValueTypeList& locals,
                    const InstructionList&,
                    const OptAt<InlineImport>&,
                    const InlineExportList&);

  auto ToImport() const -> OptAt<Import>;
  auto ToExports(Index this_index) const -> ExportList;

  FunctionDesc desc;
  BoundValueTypeList locals;
  InstructionList instructions;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Section 4: Table

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

struct ElementListWithVars {
  At<ExternalKind> kind;
  VarList list;
};

using ElementList = variant<ElementListWithVars, ElementListWithExpressions>;

struct ElementSegment;

struct Table {
  // Defined table.
  explicit Table(const TableDesc&, const InlineExportList&);

  // Defined table with implicit element segment.
  explicit Table(const TableDesc&, const InlineExportList&, const ElementList&);

  // Imported table.
  explicit Table(const TableDesc&,
                 const At<InlineImport>&,
                 const InlineExportList&);

  auto ToImport() const -> OptAt<Import>;
  auto ToExports(Index this_index) const -> ExportList;
  auto ToElementSegment(Index this_index) const -> OptAt<ElementSegment>;

  TableDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<ElementList> elements;
};

// Section 5: Memory

struct DataSegment;

struct Memory {
  // Defined memory.
  explicit Memory(const MemoryDesc&, const InlineExportList&);

  // Defined memory with implicit data segment.
  explicit Memory(const MemoryDesc&, const InlineExportList&, const TextList&);

  // Imported memory.
  explicit Memory(const MemoryDesc&,
                  const At<InlineImport>&,
                  const InlineExportList&);

  auto ToImport() const -> OptAt<Import>;
  auto ToExports(Index this_index) const -> ExportList;
  auto ToDataSegment(Index this_index) const -> OptAt<DataSegment>;

  MemoryDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<TextList> data;
};

// Section 6: Global

struct ConstantExpression {
  explicit ConstantExpression() = default;
  explicit ConstantExpression(const At<Instruction>&);
  explicit ConstantExpression(const InstructionList&);

  InstructionList instructions;
};

struct Global {
  // Defined global.
  explicit Global(const GlobalDesc&,
                  const At<ConstantExpression>& init,
                  const InlineExportList&);

  // Imported global.
  explicit Global(const GlobalDesc&,
                  const At<InlineImport>&,
                  const InlineExportList&);

  auto ToImport() const -> OptAt<Import>;
  auto ToExports(Index this_index) const -> ExportList;

  GlobalDesc desc;
  OptAt<ConstantExpression> init;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Section 7: Export

struct Export {
  At<ExternalKind> kind;
  At<Text> name;
  At<Var> var;
};

// Section 8: Start

struct Start {
  At<Var> var;
};

// Section 9: Elem

struct ElementSegment {
  // Active.
  explicit ElementSegment(OptAt<BindVar> name,
                          OptAt<Var> table,
                          const At<ConstantExpression>& offset,
                          const ElementList&);

  // Passive or declared.
  explicit ElementSegment(OptAt<BindVar> name,
                          SegmentType,
                          const ElementList&);

  OptAt<BindVar> name;
  SegmentType type;
  OptAt<Var> table;
  OptAt<ConstantExpression> offset;
  ElementList elements;
};

// Section 10: Code (handled above in Func)

// Section 11: Data

struct DataSegment {
  // Active.
  explicit DataSegment(OptAt<BindVar> name,
                       OptAt<Var> memory,
                       const At<ConstantExpression>& offset,
                       const TextList&);

  // Passive.
  explicit DataSegment(OptAt<BindVar> name, const TextList&);

  OptAt<BindVar> name;
  SegmentType type;
  OptAt<Var> memory;
  OptAt<ConstantExpression> offset;
  TextList data;
};

// Section 12: DataCount

// Section 13: Event

struct Event {
  // Empty event.
  explicit Event() = default;

  // Defined event.
  explicit Event(const EventDesc&, const InlineExportList&);

  // Imported event.
  explicit Event(const EventDesc&,
                 const At<InlineImport>&,
                 const InlineExportList&);

  // Imported or defined.
  explicit Event(const EventDesc&,
                 const OptAt<InlineImport>&,
                 const InlineExportList&);

  auto ToImport() const -> OptAt<Import>;
  auto ToExports(Index this_index) const -> ExportList;

  EventDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Module

// NOTE this must be kept in sync with the ModuleItem variant below.
enum class ModuleItemKind {
  DefinedType,
  Import,
  Function,
  Table,
  Memory,
  Global,
  Export,
  Start,
  ElementSegment,
  DataSegment,
  Event
};

struct ModuleItem {
  auto kind() const -> ModuleItemKind;
  bool is_defined_type() const;
  bool is_import() const;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;
  bool is_export() const;
  bool is_start() const;
  bool is_element_segment() const;
  bool is_data_segment() const;
  bool is_event() const;

  auto defined_type() -> At<DefinedType>&;
  auto defined_type() const -> const At<DefinedType>&;
  auto import() -> At<Import>&;
  auto import() const -> const At<Import>&;
  auto function() -> At<Function>&;
  auto function() const -> const At<Function>&;
  auto table() -> At<Table>&;
  auto table() const -> const At<Table>&;
  auto memory() -> At<Memory>&;
  auto memory() const -> const At<Memory>&;
  auto global() -> At<Global>&;
  auto global() const -> const At<Global>&;
  auto export_() -> At<Export>&;
  auto export_() const -> const At<Export>&;
  auto start() -> At<Start>&;
  auto start() const -> const At<Start>&;
  auto element_segment() -> At<ElementSegment>&;
  auto element_segment() const -> const At<ElementSegment>&;
  auto data_segment() -> At<DataSegment>&;
  auto data_segment() const -> const At<DataSegment>&;
  auto event() -> At<Event>&;
  auto event() const -> const At<Event>&;

  variant<At<DefinedType>,
          At<Import>,
          At<Function>,
          At<Table>,
          At<Memory>,
          At<Global>,
          At<Export>,
          At<Start>,
          At<ElementSegment>,
          At<DataSegment>,
          At<Event>> desc;
};

using Module = std::vector<ModuleItem>;

// Script

using ModuleVar = string_view;

enum class ScriptModuleKind {
  Binary,  // (module bin "...")
  Text,    // (module ...)
  Quote    // (module quote "...")
};

struct ScriptModule {
  // For ScriptModuleKind::Text.
  bool has_module() const;
  auto module() -> Module&;
  auto module() const -> const Module&;

  // For ScriptModuleKind::Binary and ScriptModuleKind::Quote.
  bool has_text_list() const;
  auto text_list() -> TextList&;
  auto text_list() const -> const TextList&;

  OptAt<BindVar> name;
  ScriptModuleKind kind;
  variant<Module, TextList> contents;
};

struct RefNullConst {
  At<HeapType> type;
};

struct RefExternConst {
  At<u32> var;
};

// NOTE this must be kept in sync with the Const variant below.
enum class ConstKind {
  U32,
  U64,
  F32,
  F64,
  V128,
  RefNull,
  RefExtern,
};

struct Const {
  auto kind() const -> ConstKind;
  bool is_u32() const;
  bool is_u64() const;
  bool is_f32() const;
  bool is_f64() const;
  bool is_v128() const;
  bool is_ref_null() const;
  bool is_ref_extern() const;

  auto u32_() -> u32&;
  auto u32_() const -> const u32&;
  auto u64_() -> u64&;
  auto u64_() const -> const u64&;
  auto f32_() -> f32&;
  auto f32_() const -> const f32&;
  auto f64_() -> f64&;
  auto f64_() const -> const f64&;
  auto v128_() -> v128&;
  auto v128_() const -> const v128&;
  auto ref_null() -> RefNullConst&;
  auto ref_null() const -> const RefNullConst&;
  auto ref_extern() -> RefExternConst&;
  auto ref_extern() const -> const RefExternConst&;

  variant<u32, u64, f32, f64, v128, RefNullConst, RefExternConst> value;
};

using ConstList = std::vector<At<Const>>;

struct InvokeAction {
  OptAt<ModuleVar> module;
  At<Text> name;
  ConstList consts;
};

struct GetAction {
  OptAt<ModuleVar> module;
  At<Text> name;
};

using Action = variant<InvokeAction, GetAction>;

enum class AssertionKind {
  Malformed,
  Invalid,
  Unlinkable,
  ActionTrap,
  Return,
  ModuleTrap,
  Exhaustion,
};

struct ModuleAssertion {
  At<ScriptModule> module;
  At<Text> message;
};

struct ActionAssertion {
  At<Action> action;
  At<Text> message;
};

enum class NanKind { Canonical, Arithmetic };

template <typename T>
using FloatResult = variant<T, NanKind>;
using F32Result = FloatResult<f32>;
using F64Result = FloatResult<f64>;

using F32x4Result = std::array<F32Result, 4>;
using F64x2Result = std::array<F64Result, 2>;

struct RefExternResult {};
struct RefFuncResult {};

// TODO: u32 and u64 here seem to cause conversion warnings in win32
using ReturnResult = variant<u32,
                             u64,
                             v128,
                             F32Result,
                             F64Result,
                             F32x4Result,
                             F64x2Result,
                             RefNullConst,
                             RefExternConst,
                             RefExternResult,
                             RefFuncResult>;
using ReturnResultList = std::vector<At<ReturnResult>>;

struct ReturnAssertion {
  At<Action> action;
  ReturnResultList results;
};

struct Assertion {
  bool is_module_assertion() const;
  bool is_action_assertion() const;
  bool is_return_assertion() const;

  auto module_assertion() -> ModuleAssertion&;
  auto module_assertion() const -> const ModuleAssertion&;
  auto action_assertion() -> ActionAssertion&;
  auto action_assertion() const -> const ActionAssertion&;
  auto return_assertion() -> ReturnAssertion&;
  auto return_assertion() const -> const ReturnAssertion&;

  AssertionKind kind;
  variant<ModuleAssertion, ActionAssertion, ReturnAssertion> desc;
};

struct Register {
  At<Text> name;
  OptAt<ModuleVar> module;
};

// NOTE: variant order must be kept in sync with Command variant below.
enum class CommandKind {
  ScriptModule,
  Register,
  Action,
  Assertion,
};

struct Command {
  auto kind() const -> CommandKind;
  bool is_script_module() const;
  bool is_register() const;
  bool is_action() const;
  bool is_assertion() const;

  auto script_module() -> ScriptModule&;
  auto script_module() const -> const ScriptModule&;
  auto register_() -> Register&;
  auto register_() const -> const Register&;
  auto action() -> Action&;
  auto action() const -> const Action&;
  auto assertion() -> Assertion&;
  auto assertion() const -> const Assertion&;

  variant<ScriptModule, Register, Action, Assertion> contents;
};

using Script = std::vector<At<Command>>;

#define WASP_TEXT_ENUMS(WASP_V) \
  WASP_V(text::TokenType)             \
  WASP_V(text::Sign)                  \
  WASP_V(text::LiteralKind)           \
  WASP_V(text::Base)                  \
  WASP_V(text::HasUnderscores)        \
  WASP_V(text::ScriptModuleKind)      \
  WASP_V(text::AssertionKind)         \
  WASP_V(text::NanKind)

#define WASP_TEXT_STRUCTS(WASP_V)                                        \
  WASP_V(text::LiteralInfo, 4, sign, kind, base, has_underscores)        \
  WASP_V(text::OpcodeInfo, 2, opcode, features)                          \
  WASP_V(text::Text, 2, text, byte_size)                                 \
  WASP_V(text::Token, 3, loc, type, immediate)                           \
  WASP_V(text::BoundValueType, 2, name, type)                            \
  WASP_V(text::BoundFunctionType, 2, params, results)                    \
  WASP_V(text::FieldType, 3, name, type, mut)                            \
  WASP_V(text::StructType, 1, fields)                                    \
  WASP_V(text::ArrayType, 1, field)                                      \
  WASP_V(text::FunctionType, 2, params, results)                         \
  WASP_V(text::FunctionTypeUse, 2, type_use, type)                       \
  WASP_V(text::FunctionDesc, 3, name, type_use, type)                    \
  WASP_V(text::DefinedType, 2, name, type)                               \
  WASP_V(text::Instruction, 2, opcode, immediate)                        \
  WASP_V(text::BlockImmediate, 2, label, type)                           \
  WASP_V(text::HeapType2Immediate, 2, parent, child)                     \
  WASP_V(text::BrOnCastImmediate, 2, target, types)                      \
  WASP_V(text::BrOnExnImmediate, 2, target, event)                       \
  WASP_V(text::BrTableImmediate, 2, targets, default_target)             \
  WASP_V(text::CallIndirectImmediate, 2, table, type)                    \
  WASP_V(text::CopyImmediate, 2, dst, src)                               \
  WASP_V(text::InitImmediate, 2, segment, dst)                           \
  WASP_V(text::LetImmediate, 2, block, locals)                           \
  WASP_V(text::MemArgImmediate, 2, align, offset)                        \
  WASP_V(text::RttSubImmediate, 2, depth, types)                         \
  WASP_V(text::StructFieldImmediate, 2, struct_, field)                  \
  WASP_V(text::TableDesc, 2, name, type)                                 \
  WASP_V(text::MemoryDesc, 2, name, type)                                \
  WASP_V(text::GlobalDesc, 2, name, type)                                \
  WASP_V(text::EventType, 2, attribute, type)                            \
  WASP_V(text::EventDesc, 2, name, type)                                 \
  WASP_V(text::InlineImport, 2, module, name)                            \
  WASP_V(text::InlineExport, 1, name)                                    \
  WASP_V(text::Function, 5, desc, locals, instructions, import, exports) \
  WASP_V(text::Table, 4, desc, import, exports, elements)                \
  WASP_V(text::Memory, 4, desc, import, exports, data)                   \
  WASP_V(text::ConstantExpression, 1, instructions)                      \
  WASP_V(text::Global, 4, desc, init, import, exports)                   \
  WASP_V(text::Event, 3, desc, import, exports)                          \
  WASP_V(text::Import, 3, module, name, desc)                            \
  WASP_V(text::Export, 3, kind, name, var)                               \
  WASP_V(text::Start, 1, var)                                            \
  WASP_V(text::ElementExpression, 1, instructions)                       \
  WASP_V(text::ElementListWithExpressions, 2, elemtype, list)            \
  WASP_V(text::ElementListWithVars, 2, kind, list)                       \
  WASP_V(text::ElementSegment, 5, name, type, table, offset, elements)   \
  WASP_V(text::DataSegment, 5, name, type, memory, offset, data)         \
  WASP_V(text::ScriptModule, 3, name, kind, contents)                    \
  WASP_V(text::RefNullConst, 0)                                          \
  WASP_V(text::RefExternConst, 1, var)                                   \
  WASP_V(text::InvokeAction, 3, module, name, consts)                    \
  WASP_V(text::GetAction, 2, module, name)                               \
  WASP_V(text::ModuleAssertion, 2, module, message)                      \
  WASP_V(text::ActionAssertion, 2, action, message)                      \
  WASP_V(text::RefExternResult, 0)                                       \
  WASP_V(text::RefFuncResult, 0)                                         \
  WASP_V(text::ReturnAssertion, 2, action, results)                      \
  WASP_V(text::Assertion, 2, kind, desc)                                 \
  WASP_V(text::Register, 2, name, module)

#define WASP_TEXT_STRUCTS_CUSTOM_FORMAT(WASP_V) \
  WASP_V(text::Var, 1, desc)                    \
  WASP_V(text::ModuleItem, 1, desc)             \
  WASP_V(text::Const, 1, value)                 \
  WASP_V(text::Command, 1, contents)            \
  WASP_V(text::HeapType, 1, type)               \
  WASP_V(text::RefType, 2, heap_type, null)     \
  WASP_V(text::Rtt, 2, depth, type)             \
  WASP_V(text::ReferenceType, 1, type)          \
  WASP_V(text::ValueType, 1, type)              \
  WASP_V(text::StorageType, 1, type)            \
  WASP_V(text::TableType, 2, limits, elemtype)  \
  WASP_V(text::GlobalType, 2, valtype, mut)

#define WASP_TEXT_CONTAINERS(WASP_V)  \
  WASP_V(text::VarList)               \
  WASP_V(text::ValueTypeList)         \
  WASP_V(text::TextList)              \
  WASP_V(text::InstructionList)       \
  WASP_V(text::BoundValueTypeList)    \
  WASP_V(text::FieldTypeList)         \
  WASP_V(text::InlineExportList)      \
  WASP_V(text::ElementExpressionList) \
  WASP_V(text::Module)                \
  WASP_V(text::ConstList)             \
  WASP_V(text::F32x4Result)           \
  WASP_V(text::F64x2Result)           \
  WASP_V(text::ReturnResultList)      \
  WASP_V(text::Script)

WASP_TEXT_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_TEXT_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_TEXT_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

bool operator==(const BoundValueTypeList& lhs, const ValueTypeList& rhs);
bool operator==(const ValueTypeList& lhs, const BoundValueTypeList& rhs);
bool operator!=(const BoundValueTypeList& lhs, const ValueTypeList& rhs);
bool operator!=(const ValueTypeList& lhs, const BoundValueTypeList& rhs);

}  // namespace wasp::text

WASP_TEXT_STRUCTS(WASP_DECLARE_STD_HASH)
WASP_TEXT_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_STD_HASH)
WASP_TEXT_CONTAINERS(WASP_DECLARE_STD_HASH)

#endif  // WASP_TEXT_TYPES_H_

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

#include "wasp/text/types.h"

#include <algorithm>
#include <cassert>

#include "wasp/base/hash.h"
#include "wasp/base/macros.h"
#include "wasp/base/operator_eq_ne_macros.h"

namespace wasp::text {

void AppendToBuffer(const TextList& text_list, Buffer& buffer) {
  for (auto&& text : text_list) {
    text->AppendToBuffer(buffer);
  }
}

bool Var::is_index() const {
  return holds_alternative<Index>(desc);
}

bool Var::is_name() const {
  return holds_alternative<string_view>(desc);
}

auto Var::index() -> Index& {
  return get<Index>(desc);
}

auto Var::index() const -> const Index& {
  return get<Index>(desc);
}

auto Var::name() -> string_view& {
  return get<string_view>(desc);
}

auto Var::name() const -> const string_view& {
  return get<string_view>(desc);
}

HeapType::HeapType(At<HeapKind> type) : type{type} {}

HeapType::HeapType(At<Var> type) : type{type} {}

bool HeapType::is_heap_kind() const {
  return holds_alternative<At<HeapKind>>(type);
}

bool HeapType::is_var() const {
  return holds_alternative<At<Var>>(type);
}

auto HeapType::heap_kind() -> At<HeapKind>& {
  return get<At<HeapKind>>(type);
}

auto HeapType::heap_kind() const -> const At<HeapKind>& {
  return get<At<HeapKind>>(type);
}

auto HeapType::var() -> At<Var>& {
  return get<At<Var>>(type);
}

auto HeapType::var() const -> const At<Var>& {
  return get<At<Var>>(type);
}

ReferenceType::ReferenceType(At<ReferenceKind> type) : type{type} {}

ReferenceType::ReferenceType(At<RefType> type) : type{type} {}

// static
ReferenceType ReferenceType::Funcref_NoLocation() {
  return ReferenceType{ReferenceKind::Funcref};
}

// static
ReferenceType ReferenceType::Externref_NoLocation() {
  return ReferenceType{ReferenceKind::Externref};
}

bool ReferenceType::is_reference_kind() const {
  return holds_alternative<At<ReferenceKind>>(type);
}

bool ReferenceType::is_ref() const {
  return holds_alternative<At<RefType>>(type);
}

auto ReferenceType::reference_kind() -> At<ReferenceKind>& {
  return get<At<ReferenceKind>>(type);
}

auto ReferenceType::reference_kind() const -> const At<ReferenceKind>& {
  return get<At<ReferenceKind>>(type);
}

auto ReferenceType::ref() -> At<RefType>& {
  return get<At<RefType>>(type);
}

auto ReferenceType::ref() const -> const At<RefType>& {
  return get<At<RefType>>(type);
}

ValueType::ValueType(At<NumericType> type) : type{type} {}

ValueType::ValueType(At<ReferenceType> type) : type{type} {}

ValueType::ValueType(At<Rtt> type) : type{type} {}

// static
ValueType ValueType::I32_NoLocation() {
  return ValueType{NumericType::I32};
}

// static
ValueType ValueType::I64_NoLocation() {
  return ValueType{NumericType::I64};
}

// static
ValueType ValueType::F32_NoLocation() {
  return ValueType{NumericType::F32};
}

// static
ValueType ValueType::F64_NoLocation() {
  return ValueType{NumericType::F64};
}

// static
ValueType ValueType::V128_NoLocation() {
  return ValueType{NumericType::V128};
}

// static
ValueType ValueType::Funcref_NoLocation() {
  return ValueType{ReferenceType::Funcref_NoLocation()};
}

// static
ValueType ValueType::Externref_NoLocation() {
  return ValueType{ReferenceType::Externref_NoLocation()};
}

bool ValueType::is_numeric_type() const {
  return holds_alternative<At<NumericType>>(type);
}

bool ValueType::is_reference_type() const {
  return holds_alternative<At<ReferenceType>>(type);
}

bool ValueType::is_rtt() const {
  return holds_alternative<At<Rtt>>(type);
}

auto ValueType::numeric_type() -> At<NumericType>& {
  return get<At<NumericType>>(type);
}

auto ValueType::numeric_type() const -> const At<NumericType>& {
  return get<At<NumericType>>(type);
}

auto ValueType::reference_type() -> At<ReferenceType>& {
  return get<At<ReferenceType>>(type);
}

auto ValueType::reference_type() const -> const At<ReferenceType>& {
  return get<At<ReferenceType>>(type);
}

auto ValueType::rtt() -> At<Rtt>& {
  return get<At<Rtt>>(type);
}

auto ValueType::rtt() const -> const At<Rtt>& {
  return get<At<Rtt>>(type);
}

StorageType::StorageType(At<ValueType> type) : type{type} {}

StorageType::StorageType(At<PackedType> type) : type{type} {}

bool StorageType::is_value_type() const {
  return holds_alternative<At<ValueType>>(type);
}

bool StorageType::is_packed_type() const {
  return holds_alternative<At<PackedType>>(type);
}

auto StorageType::value_type() -> At<ValueType>& {
  return get<At<ValueType>>(type);
}

auto StorageType::value_type() const -> const At<ValueType>& {
  return get<At<ValueType>>(type);
}

auto StorageType::packed_type() -> At<PackedType>& {
  return get<At<PackedType>>(type);
}

auto StorageType::packed_type() const -> const At<PackedType>& {
  return get<At<PackedType>>(type);
}

DefinedType::DefinedType(OptAt<BindVar> name, At<BoundFunctionType> type)
    : name{name}, type{type} {}

DefinedType::DefinedType(OptAt<BindVar> name, At<StructType> type)
    : name{name}, type{type} {}

DefinedType::DefinedType(OptAt<BindVar> name, At<ArrayType> type)
    : name{name}, type{type} {}

bool DefinedType::is_function_type() const {
  return holds_alternative<At<BoundFunctionType>>(type);
}

bool DefinedType::is_struct_type() const {
  return holds_alternative<At<StructType>>(type);
}

bool DefinedType::is_array_type() const {
  return holds_alternative<At<ArrayType>>(type);
}

auto DefinedType::function_type() -> At<BoundFunctionType>& {
  return get<At<BoundFunctionType>>(type);
}

auto DefinedType::function_type() const -> const At<BoundFunctionType>& {
  return get<At<BoundFunctionType>>(type);
}

auto DefinedType::struct_type() -> At<StructType>& {
  return get<At<StructType>>(type);
}

auto DefinedType::struct_type() const -> const At<StructType>& {
  return get<At<StructType>>(type);
}

auto DefinedType::array_type() -> At<ArrayType>& {
  return get<At<ArrayType>>(type);
}

auto DefinedType::array_type() const -> const At<ArrayType>& {
  return get<At<ArrayType>>(type);
}

bool FunctionTypeUse::IsInlineType() const {
  return !type_use && type->params.empty() && type->results.size() <= 1;
}

OptAt<ValueType> FunctionTypeUse::GetInlineType() const {
  assert(IsInlineType());
  if (type->results.empty()) {
    return nullopt;
  }
  return type->results[0];
}

MemArgImmediate::MemArgImmediate(OptAt<u32> align,
                                 OptAt<u32> offset,
                                 OptAt<Var> memory)
    : align{align}, offset{offset}, memory{memory} {}

FunctionType ToFunctionType(BoundFunctionType bound_type) {
  ValueTypeList unbound_params;
  for (auto param : bound_type.params) {
    unbound_params.push_back(param->type);
  }
  return FunctionType{unbound_params, bound_type.results};
}

BoundFunctionType ToBoundFunctionType(FunctionType unbound_type) {
  BoundValueTypeList bound_params;
  for (auto param : unbound_type.params) {
    bound_params.push_back(BoundValueType{nullopt, param});
  }
  return BoundFunctionType{bound_params, unbound_type.results};
}

Instruction::Instruction(At<Opcode> opcode) : opcode{opcode} {}

Instruction::Instruction(At<Opcode> opcode, At<s32> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<s64> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<f32> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<f64> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<v128> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<Var> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<BlockImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<BrOnCastImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<BrTableImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<CallIndirectImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<CopyImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<FuncBindImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<HeapType> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<HeapType2Immediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<InitImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<LetImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<MemArgImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<MemOptImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<RttSubImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<SelectImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<ShuffleImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<SimdLaneImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<SimdMemoryLaneImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<StructFieldImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(Opcode opcode, s32 immediate)
    : opcode{opcode}, immediate{At{immediate}} {}

Instruction::Instruction(Opcode opcode, s64 immediate)
    : opcode{opcode}, immediate{At{immediate}} {}

Instruction::Instruction(Opcode opcode, f32 immediate)
    : opcode{opcode}, immediate{At{immediate}} {}

Instruction::Instruction(Opcode opcode, f64 immediate)
    : opcode{opcode}, immediate{At{immediate}} {}

Instruction::Instruction(Opcode opcode, SimdLaneImmediate immediate)
    : opcode{opcode}, immediate{At{immediate}} {}

InstructionKind Instruction::kind() const {
  return static_cast<InstructionKind>(immediate.index());
}

bool Instruction::has_no_immediate() const {
  return holds_alternative<monostate>(immediate);
}

bool Instruction::has_s32_immediate() const {
  return holds_alternative<At<s32>>(immediate);
}

bool Instruction::has_s64_immediate() const {
  return holds_alternative<At<s64>>(immediate);
}

bool Instruction::has_f32_immediate() const {
  return holds_alternative<At<f32>>(immediate);
}

bool Instruction::has_f64_immediate() const {
  return holds_alternative<At<f64>>(immediate);
}

bool Instruction::has_v128_immediate() const {
  return holds_alternative<At<v128>>(immediate);
}

bool Instruction::has_var_immediate() const {
  return holds_alternative<At<Var>>(immediate);
}

bool Instruction::has_block_immediate() const {
  return holds_alternative<At<BlockImmediate>>(immediate);
}

bool Instruction::has_br_on_cast_immediate() const {
  return holds_alternative<At<BrOnCastImmediate>>(immediate);
}

bool Instruction::has_br_table_immediate() const {
  return holds_alternative<At<BrTableImmediate>>(immediate);
}

bool Instruction::has_call_indirect_immediate() const {
  return holds_alternative<At<CallIndirectImmediate>>(immediate);
}

bool Instruction::has_copy_immediate() const {
  return holds_alternative<At<CopyImmediate>>(immediate);
}

bool Instruction::has_func_bind_immediate() const {
  return holds_alternative<At<FuncBindImmediate>>(immediate);
}

bool Instruction::has_heap_type_immediate() const {
  return holds_alternative<At<HeapType>>(immediate);
}

bool Instruction::has_heap_type_2_immediate() const {
  return holds_alternative<At<HeapType2Immediate>>(immediate);
}

bool Instruction::has_init_immediate() const {
  return holds_alternative<At<InitImmediate>>(immediate);
}

bool Instruction::has_let_immediate() const {
  return holds_alternative<At<LetImmediate>>(immediate);
}

bool Instruction::has_mem_arg_immediate() const {
  return holds_alternative<At<MemArgImmediate>>(immediate);
}

bool Instruction::has_mem_opt_immediate() const {
  return holds_alternative<At<MemOptImmediate>>(immediate);
}

bool Instruction::has_rtt_sub_immediate() const {
  return holds_alternative<At<RttSubImmediate>>(immediate);
}

bool Instruction::has_select_immediate() const {
  return holds_alternative<At<SelectImmediate>>(immediate);
}

bool Instruction::has_shuffle_immediate() const {
  return holds_alternative<At<ShuffleImmediate>>(immediate);
}

bool Instruction::has_simd_lane_immediate() const {
  return holds_alternative<At<SimdLaneImmediate>>(immediate);
}

bool Instruction::has_simd_memory_lane_immediate() const {
  return holds_alternative<At<SimdMemoryLaneImmediate>>(immediate);
}

bool Instruction::has_struct_field_immediate() const {
  return holds_alternative<At<StructFieldImmediate>>(immediate);
}


At<s32>& Instruction::s32_immediate() {
  return get<At<s32>>(immediate);
}

const At<s32>& Instruction::s32_immediate() const {
  return get<At<s32>>(immediate);
}

At<s64>& Instruction::s64_immediate() {
  return get<At<s64>>(immediate);
}

const At<s64>& Instruction::s64_immediate() const {
  return get<At<s64>>(immediate);
}

At<f32>& Instruction::f32_immediate() {
  return get<At<f32>>(immediate);
}

const At<f32>& Instruction::f32_immediate() const {
  return get<At<f32>>(immediate);
}

At<f64>& Instruction::f64_immediate() {
  return get<At<f64>>(immediate);
}

const At<f64>& Instruction::f64_immediate() const {
  return get<At<f64>>(immediate);
}

At<v128>& Instruction::v128_immediate() {
  return get<At<v128>>(immediate);
}

const At<v128>& Instruction::v128_immediate() const {
  return get<At<v128>>(immediate);
}

At<Var>& Instruction::var_immediate() {
  return get<At<Var>>(immediate);
}

const At<Var>& Instruction::var_immediate() const {
  return get<At<Var>>(immediate);
}

At<BlockImmediate>& Instruction::block_immediate() {
  return get<At<BlockImmediate>>(immediate);
}

const At<BlockImmediate>& Instruction::block_immediate() const {
  return get<At<BlockImmediate>>(immediate);
}

At<BrOnCastImmediate>& Instruction::br_on_cast_immediate() {
  return get<At<BrOnCastImmediate>>(immediate);
}

const At<BrOnCastImmediate>& Instruction::br_on_cast_immediate() const {
  return get<At<BrOnCastImmediate>>(immediate);
}

At<BrTableImmediate>& Instruction::br_table_immediate() {
  return get<At<BrTableImmediate>>(immediate);
}

const At<BrTableImmediate>& Instruction::br_table_immediate() const {
  return get<At<BrTableImmediate>>(immediate);
}

At<CallIndirectImmediate>& Instruction::call_indirect_immediate() {
  return get<At<CallIndirectImmediate>>(immediate);
}

const At<CallIndirectImmediate>& Instruction::call_indirect_immediate() const {
  return get<At<CallIndirectImmediate>>(immediate);
}

At<CopyImmediate>& Instruction::copy_immediate() {
  return get<At<CopyImmediate>>(immediate);
}

const At<CopyImmediate>& Instruction::copy_immediate() const {
  return get<At<CopyImmediate>>(immediate);
}

At<FuncBindImmediate>& Instruction::func_bind_immediate() {
  return get<At<FuncBindImmediate>>(immediate);
}

const At<FuncBindImmediate>& Instruction::func_bind_immediate() const {
  return get<At<FuncBindImmediate>>(immediate);
}

At<HeapType>& Instruction::heap_type_immediate() {
  return get<At<HeapType>>(immediate);
}

const At<HeapType>& Instruction::heap_type_immediate() const {
  return get<At<HeapType>>(immediate);
}

At<HeapType2Immediate>& Instruction::heap_type_2_immediate() {
  return get<At<HeapType2Immediate>>(immediate);
}

const At<HeapType2Immediate>& Instruction::heap_type_2_immediate() const {
  return get<At<HeapType2Immediate>>(immediate);
}

At<InitImmediate>& Instruction::init_immediate() {
  return get<At<InitImmediate>>(immediate);
}

const At<InitImmediate>& Instruction::init_immediate() const {
  return get<At<InitImmediate>>(immediate);
}

At<LetImmediate>& Instruction::let_immediate() {
  return get<At<LetImmediate>>(immediate);
}

const At<LetImmediate>& Instruction::let_immediate() const {
  return get<At<LetImmediate>>(immediate);
}

At<MemArgImmediate>& Instruction::mem_arg_immediate() {
  return get<At<MemArgImmediate>>(immediate);
}

const At<MemArgImmediate>& Instruction::mem_arg_immediate() const {
  return get<At<MemArgImmediate>>(immediate);
}

At<MemOptImmediate>& Instruction::mem_opt_immediate() {
  return get<At<MemOptImmediate>>(immediate);
}

const At<MemOptImmediate>& Instruction::mem_opt_immediate() const {
  return get<At<MemOptImmediate>>(immediate);
}

At<RttSubImmediate>& Instruction::rtt_sub_immediate() {
  return get<At<RttSubImmediate>>(immediate);
}

const At<RttSubImmediate>& Instruction::rtt_sub_immediate() const {
  return get<At<RttSubImmediate>>(immediate);
}

At<SelectImmediate>& Instruction::select_immediate() {
  return get<At<SelectImmediate>>(immediate);
}

const At<SelectImmediate>& Instruction::select_immediate() const {
  return get<At<SelectImmediate>>(immediate);
}

At<ShuffleImmediate>& Instruction::shuffle_immediate() {
  return get<At<ShuffleImmediate>>(immediate);
}

const At<ShuffleImmediate>& Instruction::shuffle_immediate() const {
  return get<At<ShuffleImmediate>>(immediate);
}

At<SimdLaneImmediate>& Instruction::simd_lane_immediate() {
  return get<At<SimdLaneImmediate>>(immediate);
}

const At<SimdLaneImmediate>& Instruction::simd_lane_immediate() const {
  return get<At<SimdLaneImmediate>>(immediate);
}

At<SimdMemoryLaneImmediate>& Instruction::simd_memory_lane_immediate() {
  return get<At<SimdMemoryLaneImmediate>>(immediate);
}

const At<SimdMemoryLaneImmediate>& Instruction::simd_memory_lane_immediate()
    const {
  return get<At<SimdMemoryLaneImmediate>>(immediate);
}

At<StructFieldImmediate>& Instruction::struct_field_immediate() {
  return get<At<StructFieldImmediate>>(immediate);
}

const At<StructFieldImmediate>& Instruction::struct_field_immediate() const {
  return get<At<StructFieldImmediate>>(immediate);
}

ExternalKind Import::kind() const {
  return static_cast<ExternalKind>(desc.index());
}

bool Import::is_function() const {
  return holds_alternative<FunctionDesc>(desc);
}

bool Import::is_table() const {
  return holds_alternative<TableDesc>(desc);
}

bool Import::is_memory() const {
  return holds_alternative<MemoryDesc>(desc);
}

bool Import::is_global() const {
  return holds_alternative<GlobalDesc>(desc);
}

bool Import::is_tag() const {
  return holds_alternative<TagDesc>(desc);
}


auto Import::function_desc() -> FunctionDesc& {
  return get<FunctionDesc>(desc);
}

auto Import::function_desc() const -> const FunctionDesc& {
  return get<FunctionDesc>(desc);
}

auto Import::table_desc() -> TableDesc& {
  return get<TableDesc>(desc);
}

auto Import::table_desc() const -> const TableDesc& {
  return get<TableDesc>(desc);
}

auto Import::memory_desc() -> MemoryDesc& {
  return get<MemoryDesc>(desc);
}

auto Import::memory_desc() const -> const MemoryDesc& {
  return get<MemoryDesc>(desc);
}

auto Import::global_desc() -> GlobalDesc& {
  return get<GlobalDesc>(desc);
}

auto Import::global_desc() const -> const GlobalDesc& {
  return get<GlobalDesc>(desc);
}

auto Import::tag_desc() -> TagDesc& {
  return get<TagDesc>(desc);
}

auto Import::tag_desc() const -> const TagDesc& {
  return get<TagDesc>(desc);
}


Function::Function(const FunctionDesc& desc,
                   const BoundValueTypeList& locals,
                   const InstructionList& instructions,
                   const InlineExportList& exports)
    : desc{desc},
      locals{locals},
      instructions{instructions},
      exports{exports} {}

Function::Function(const FunctionDesc& desc,
                   const At<InlineImport>& import,
                   const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

Function::Function(const FunctionDesc& desc,
                   const BoundValueTypeList& locals,
                   const InstructionList& instructions,
                   const OptAt<InlineImport>& import,
                   const InlineExportList& exports)
    : desc{desc},
      locals{locals},
      instructions{instructions},
      import{import},
      exports{exports} {}

auto Function::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return At{import->loc(),
            Import{import.value()->module, import.value()->name, desc}};
}

auto MakeExportList(ExternalKind kind,
                    Index this_index,
                    const InlineExportList& inline_exports) -> ExportList {
  ExportList result;
  for (auto& inline_export : inline_exports) {
    result.push_back(At{inline_export.loc(),
                        Export{kind, inline_export->name, Var{this_index}}});
  }
  return result;
}

auto Function::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Function, this_index, exports);
}

Table::Table(const TableDesc& desc, const InlineExportList& exports)
    : desc{desc}, exports{exports} {}

Table::Table(const TableDesc& desc,
             const InlineExportList& exports,
             const ElementList& elements)
    : desc{desc}, exports{exports}, elements{elements} {}

Table::Table(const TableDesc& desc,
             const At<InlineImport>& import,
             const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Table::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return At{import->loc(),
            Import{import->value().module, import->value().name, desc}};
}

auto Table::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Table, this_index, exports);
}

auto Table::ToElementSegment(Index this_index) const -> OptAt<ElementSegment> {
  if (!elements) {
    return nullopt;
  }
  return ElementSegment{
      nullopt, Var{this_index},
      At{ConstantExpression{Instruction{At{Opcode::I32Const}, At{s32{0}}}}},
      *elements};
}

auto NumericData::data_type_size() const -> u32 {
  switch (type) {
    case NumericDataType::I8:   return 1;
    case NumericDataType::I16:  return 2;
    case NumericDataType::I32:
    case NumericDataType::F32:  return 4;
    case NumericDataType::I64:
    case NumericDataType::F64:  return 8;
    case NumericDataType::V128: return 16;
    default:
      WASP_UNREACHABLE();
  }
}

auto NumericData::byte_size() const -> u32 {
  return static_cast<u32>(data.size());
}

auto NumericData::count() const -> Index {
  return data.size() / data_type_size();
}

void NumericData::AppendToBuffer(Buffer& buffer) const {
  auto old_size = buffer.size();
  buffer.resize(old_size + data.size());
  std::copy(data.begin(), data.end(), buffer.begin() + old_size);
}

bool DataItem::is_text() const {
  return holds_alternative<Text>(value);
}

bool DataItem::is_numeric_data() const {
  return holds_alternative<NumericData>(value);
}

auto DataItem::text() -> Text& {
  return get<Text>(value);
}

auto DataItem::text() const -> const Text& {
  return get<Text>(value);
}

auto DataItem::numeric_data() -> NumericData& {
  return get<NumericData>(value);
}

auto DataItem::numeric_data() const -> const NumericData& {
  return get<NumericData>(value);
}

auto DataItem::byte_size() const -> u32 {
  return is_text() ? text().byte_size : numeric_data().byte_size();
}

void DataItem::AppendToBuffer(Buffer& buffer) const {
  if (is_text()) {
    text().AppendToBuffer(buffer);
  } else {
    assert(is_numeric_data());
    numeric_data().AppendToBuffer(buffer);
  }
}

Memory::Memory(const MemoryDesc& desc, const InlineExportList& exports)
    : desc{desc}, exports{exports} {}

Memory::Memory(const MemoryDesc& desc,
               const InlineExportList& exports,
               const DataItemList& data)
    : desc{desc}, exports{exports}, data{data} {}

Memory::Memory(const MemoryDesc& desc,
               const At<InlineImport>& import,
               const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Memory::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return At{import->loc(),
            Import{import->value().module, import->value().name, desc}};
}

auto Memory::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Memory, this_index, exports);
}

auto Memory::ToDataSegment(Index this_index) const -> OptAt<DataSegment> {
  if (!data) {
    return nullopt;
  }
  if (desc.type->limits->index_type == IndexType::I64) {
    return DataSegment{
        nullopt, Var{this_index},
        At{ConstantExpression{Instruction{At{Opcode::I64Const}, At{s64{0}}}}},
        *data};
  }
  return DataSegment{
      nullopt, Var{this_index},
      At{ConstantExpression{Instruction{At{Opcode::I32Const}, At{s32{0}}}}},
      *data};
}

ConstantExpression::ConstantExpression(const At<Instruction>& instruction)
    : instructions{{instruction}} {}

ConstantExpression::ConstantExpression(const InstructionList& instructions)
    : instructions{instructions} {}

Global::Global(const GlobalDesc& desc,
               const At<ConstantExpression>& init,
               const InlineExportList& exports)
    : desc{desc}, init{init}, exports{exports} {}

Global::Global(const GlobalDesc& desc,
               const At<InlineImport>& import,
               const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Global::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return At{import->loc(),
            Import{import->value().module, import->value().name, desc}};
}

auto Global::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Global, this_index, exports);
}

Tag::Tag(const TagDesc& desc, const InlineExportList& exports)
    : desc{desc}, exports{exports} {}

Tag::Tag(const TagDesc& desc,
         const At<InlineImport>& import,
         const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

Tag::Tag(const TagDesc& desc,
         const OptAt<InlineImport>& import,
         const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Tag::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return At{import->loc(),
            Import{import->value().module, import->value().name, desc}};
}

auto Tag::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Tag, this_index, exports);
}

ElementExpression::ElementExpression(const At<Instruction>& instruction)
    : instructions{{instruction}} {}

ElementExpression::ElementExpression(const InstructionList& instructions)
    : instructions{instructions} {}

ElementSegment::ElementSegment(OptAt<BindVar> name,
                               OptAt<Var> table,
                               const At<ConstantExpression>& offset,
                               const ElementList& elements)
    : name{name},
      type{SegmentType::Active},
      table{table},
      offset{offset},
      elements{elements} {}

ElementSegment::ElementSegment(OptAt<BindVar> name,
                               SegmentType type,
                               const ElementList& elements)
    : name{name}, type{type}, elements{elements} {}

DataSegment::DataSegment(OptAt<BindVar> name,
                         OptAt<Var> memory,
                         const At<ConstantExpression>& offset,
                         const DataItemList& data)
    : name{name},
      type{SegmentType::Active},
      memory{memory},
      offset{offset},
      data{data} {}

DataSegment::DataSegment(OptAt<BindVar> name, const DataItemList& data)
    : name{name}, type{SegmentType::Passive}, data{data} {}

auto ModuleItem::kind() const -> ModuleItemKind {
  return static_cast<ModuleItemKind>(desc.index());
}

bool ModuleItem::is_defined_type() const {
  return holds_alternative<At<DefinedType>>(desc);
}

bool ModuleItem::is_import() const {
  return holds_alternative<At<Import>>(desc);
}

bool ModuleItem::is_function() const {
  return holds_alternative<At<Function>>(desc);
}

bool ModuleItem::is_table() const {
  return holds_alternative<At<Table>>(desc);
}

bool ModuleItem::is_memory() const {
  return holds_alternative<At<Memory>>(desc);
}

bool ModuleItem::is_global() const {
  return holds_alternative<At<Global>>(desc);
}

bool ModuleItem::is_export() const {
  return holds_alternative<At<Export>>(desc);
}

bool ModuleItem::is_start() const {
  return holds_alternative<At<Start>>(desc);
}

bool ModuleItem::is_element_segment() const {
  return holds_alternative<At<ElementSegment>>(desc);
}

bool ModuleItem::is_data_segment() const {
  return holds_alternative<At<DataSegment>>(desc);
}

bool ModuleItem::is_tag() const {
  return holds_alternative<At<Tag>>(desc);
}

auto ModuleItem::defined_type() -> At<DefinedType>& {
  return get<At<DefinedType>>(desc);
}

auto ModuleItem::defined_type() const -> const At<DefinedType>& {
  return get<At<DefinedType>>(desc);
}

auto ModuleItem::import() -> At<Import>& {
  return get<At<Import>>(desc);
}

auto ModuleItem::import() const -> const At<Import>& {
  return get<At<Import>>(desc);
}

auto ModuleItem::function() -> At<Function>& {
  return get<At<Function>>(desc);
}

auto ModuleItem::function() const -> const At<Function>& {
  return get<At<Function>>(desc);
}

auto ModuleItem::table() -> At<Table>& {
  return get<At<Table>>(desc);
}

auto ModuleItem::table() const -> const At<Table>& {
  return get<At<Table>>(desc);
}

auto ModuleItem::memory() -> At<Memory>& {
  return get<At<Memory>>(desc);
}

auto ModuleItem::memory() const -> const At<Memory>& {
  return get<At<Memory>>(desc);
}

auto ModuleItem::global() -> At<Global>& {
  return get<At<Global>>(desc);
}

auto ModuleItem::global() const -> const At<Global>& {
  return get<At<Global>>(desc);
}

auto ModuleItem::export_() -> At<Export>& {
  return get<At<Export>>(desc);
}

auto ModuleItem::export_() const -> const At<Export>& {
  return get<At<Export>>(desc);
}

auto ModuleItem::start() -> At<Start>& {
  return get<At<Start>>(desc);
}

auto ModuleItem::start() const -> const At<Start>& {
  return get<At<Start>>(desc);
}

auto ModuleItem::element_segment() -> At<ElementSegment>& {
  return get<At<ElementSegment>>(desc);
}

auto ModuleItem::element_segment() const -> const At<ElementSegment>& {
  return get<At<ElementSegment>>(desc);
}

auto ModuleItem::data_segment() -> At<DataSegment>& {
  return get<At<DataSegment>>(desc);
}

auto ModuleItem::data_segment() const -> const At<DataSegment>& {
  return get<At<DataSegment>>(desc);
}

auto ModuleItem::tag() -> At<Tag>& {
  return get<At<Tag>>(desc);
}

auto ModuleItem::tag() const -> const At<Tag>& {
  return get<At<Tag>>(desc);
}

bool ScriptModule::has_module() const {
  return holds_alternative<Module>(contents);
}

auto ScriptModule::module() -> Module& {
  return get<Module>(contents);
}

auto ScriptModule::module() const -> const Module& {
  return get<Module>(contents);
}


bool ScriptModule::has_text_list() const {
  return holds_alternative<TextList>(contents);
}

auto ScriptModule::text_list() -> TextList& {
  return get<TextList>(contents);
}

auto ScriptModule::text_list() const -> const TextList& {
  return get<TextList>(contents);
}


auto Const::kind() const -> ConstKind {
  return static_cast<ConstKind>(value.index());
}

bool Const::is_u32() const {
  return holds_alternative<u32>(value);
}

bool Const::is_u64() const {
  return holds_alternative<u64>(value);
}

bool Const::is_f32() const {
  return holds_alternative<f32>(value);
}

bool Const::is_f64() const {
  return holds_alternative<f64>(value);
}

bool Const::is_v128() const {
  return holds_alternative<v128>(value);
}

bool Const::is_ref_null() const {
  return holds_alternative<RefNullConst>(value);
}

bool Const::is_ref_extern() const {
  return holds_alternative<RefExternConst>(value);
}


auto Const::u32_() -> u32& {
  return get<u32>(value);
}

auto Const::u32_() const -> const u32& {
  return get<u32>(value);
}

auto Const::u64_() -> u64& {
  return get<u64>(value);
}

auto Const::u64_() const -> const u64& {
  return get<u64>(value);
}

auto Const::f32_() -> f32& {
  return get<f32>(value);
}

auto Const::f32_() const -> const f32& {
  return get<f32>(value);
}

auto Const::f64_() -> f64& {
  return get<f64>(value);
}

auto Const::f64_() const -> const f64& {
  return get<f64>(value);
}

auto Const::v128_() -> v128& {
  return get<v128>(value);
}

auto Const::v128_() const -> const v128& {
  return get<v128>(value);
}

auto Const::ref_null() -> RefNullConst& {
  return get<RefNullConst>(value);
}

auto Const::ref_null() const -> const RefNullConst& {
  return get<RefNullConst>(value);
}

auto Const::ref_extern() -> RefExternConst& {
  return get<RefExternConst>(value);
}

auto Const::ref_extern() const -> const RefExternConst& {
  return get<RefExternConst>(value);
}

bool Assertion::is_module_assertion() const {
  return holds_alternative<ModuleAssertion>(desc);
}

bool Assertion::is_action_assertion() const {
  return holds_alternative<ActionAssertion>(desc);
}

bool Assertion::is_return_assertion() const {
  return holds_alternative<ReturnAssertion>(desc);
}


auto Assertion::module_assertion() -> ModuleAssertion& {
  return get<ModuleAssertion>(desc);
}

auto Assertion::module_assertion() const -> const ModuleAssertion& {
  return get<ModuleAssertion>(desc);
}

auto Assertion::action_assertion() -> ActionAssertion& {
  return get<ActionAssertion>(desc);
}

auto Assertion::action_assertion() const -> const ActionAssertion& {
  return get<ActionAssertion>(desc);
}

auto Assertion::return_assertion() -> ReturnAssertion& {
  return get<ReturnAssertion>(desc);
}

auto Assertion::return_assertion() const -> const ReturnAssertion& {
  return get<ReturnAssertion>(desc);
}


auto Command::kind() const -> CommandKind {
  return static_cast<CommandKind>(contents.index());
}

bool Command::is_script_module() const {
  return holds_alternative<ScriptModule>(contents);
}

bool Command::is_register() const {
  return holds_alternative<Register>(contents);
}

bool Command::is_action() const {
  return holds_alternative<Action>(contents);
}

bool Command::is_assertion() const {
  return holds_alternative<Assertion>(contents);
}

auto Command::script_module() -> ScriptModule& {
  return get<ScriptModule>(contents);
}

auto Command::script_module() const -> const ScriptModule& {
  return get<ScriptModule>(contents);
}

auto Command::register_() -> Register& {
  return get<Register>(contents);
}

auto Command::register_() const -> const Register& {
  return get<Register>(contents);
}

auto Command::action() -> Action& {
  return get<Action>(contents);
}

auto Command::action() const -> const Action& {
  return get<Action>(contents);
}

auto Command::assertion() -> Assertion& {
  return get<Assertion>(contents);
}

auto Command::assertion() const -> const Assertion& {
  return get<Assertion>(contents);
}


WASP_TEXT_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_TEXT_STRUCTS_CUSTOM_FORMAT(WASP_OPERATOR_EQ_NE_VARGS)
WASP_TEXT_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

bool operator==(const BoundValueTypeList& lhs, const ValueTypeList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                    [](auto&& lhs, auto&& rhs) { return lhs->type == rhs; });
}

bool operator!=(const BoundValueTypeList& lhs, const ValueTypeList& rhs) {
  return !(lhs == rhs);
}

}  // namespace wasp::text

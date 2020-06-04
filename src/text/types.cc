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
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

namespace wasp {
namespace text {

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

Instruction::Instruction(At<Opcode> opcode, At<BrOnExnImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<BrTableImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<CallIndirectImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<CopyImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<InitImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<MemArgImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<ReferenceType> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<SelectImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<ShuffleImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<SimdLaneImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(Opcode opcode, s32 immediate)
    : opcode{opcode}, immediate{MakeAt(immediate)} {}

Instruction::Instruction(Opcode opcode, s64 immediate)
    : opcode{opcode}, immediate{MakeAt(immediate)} {}

Instruction::Instruction(Opcode opcode, f32 immediate)
    : opcode{opcode}, immediate{MakeAt(immediate)} {}

Instruction::Instruction(Opcode opcode, f64 immediate)
    : opcode{opcode}, immediate{MakeAt(immediate)} {}

Instruction::Instruction(Opcode opcode, SimdLaneImmediate immediate)
    : opcode{opcode}, immediate{MakeAt(immediate)} {}

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

bool Instruction::has_br_on_exn_immediate() const {
  return holds_alternative<At<BrOnExnImmediate>>(immediate);
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

bool Instruction::has_init_immediate() const {
  return holds_alternative<At<InitImmediate>>(immediate);
}

bool Instruction::has_mem_arg_immediate() const {
  return holds_alternative<At<MemArgImmediate>>(immediate);
}

bool Instruction::has_reference_type_immediate() const {
  return holds_alternative<At<ReferenceType>>(immediate);
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

At<BrOnExnImmediate>& Instruction::br_on_exn_immediate() {
  return get<At<BrOnExnImmediate>>(immediate);
}

const At<BrOnExnImmediate>& Instruction::br_on_exn_immediate() const {
  return get<At<BrOnExnImmediate>>(immediate);
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

At<InitImmediate>& Instruction::init_immediate() {
  return get<At<InitImmediate>>(immediate);
}

const At<InitImmediate>& Instruction::init_immediate() const {
  return get<At<InitImmediate>>(immediate);
}

At<MemArgImmediate>& Instruction::mem_arg_immediate() {
  return get<At<MemArgImmediate>>(immediate);
}

const At<MemArgImmediate>& Instruction::mem_arg_immediate() const {
  return get<At<MemArgImmediate>>(immediate);
}

At<ReferenceType>& Instruction::reference_type_immediate() {
  return get<At<ReferenceType>>(immediate);
}

const At<ReferenceType>& Instruction::reference_type_immediate() const {
  return get<At<ReferenceType>>(immediate);
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

bool Import::is_event() const {
  return holds_alternative<EventDesc>(desc);
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

auto Import::event_desc() -> EventDesc& {
  return get<EventDesc>(desc);
}

auto Import::event_desc() const -> const EventDesc& {
  return get<EventDesc>(desc);
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
  return MakeAt(import->loc(),
                Import{import.value()->module, import.value()->name, desc});
}

auto MakeExportList(ExternalKind kind,
                    Index this_index,
                    const InlineExportList& inline_exports) -> ExportList {
  ExportList result;
  for (auto& inline_export : inline_exports) {
    result.push_back(
        MakeAt(inline_export.loc(),
               Export{kind, inline_export->name, Var{this_index}}));
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
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Table::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Table, this_index, exports);
}

auto Table::ToElementSegment(Index this_index) const -> OptAt<ElementSegment> {
  if (!elements) {
    return nullopt;
  }
  return ElementSegment{nullopt, Var{this_index},
                        MakeAt(ConstantExpression{Instruction{
                            MakeAt(Opcode::I32Const), MakeAt(s32{0})}}),
                        *elements};
}

Memory::Memory(const MemoryDesc& desc, const InlineExportList& exports)
    : desc{desc}, exports{exports} {}

Memory::Memory(const MemoryDesc& desc,
               const InlineExportList& exports,
               const TextList& data)
    : desc{desc}, exports{exports}, data{data} {}

Memory::Memory(const MemoryDesc& desc,
               const At<InlineImport>& import,
               const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Memory::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Memory::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Memory, this_index, exports);
}

auto Memory::ToDataSegment(Index this_index) const -> OptAt<DataSegment> {
  if (!data) {
    return nullopt;
  }
  return DataSegment{nullopt, Var{this_index},
                     MakeAt(ConstantExpression{Instruction{
                         MakeAt(Opcode::I32Const), MakeAt(s32{0})}}),
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
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Global::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Global, this_index, exports);
}

Event::Event(const EventDesc& desc, const InlineExportList& exports)
    : desc{desc}, exports{exports} {}

Event::Event(const EventDesc& desc,
             const At<InlineImport>& import,
             const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

Event::Event(const EventDesc& desc,
             const OptAt<InlineImport>& import,
             const InlineExportList& exports)
    : desc{desc}, import{import}, exports{exports} {}

auto Event::ToImport() const -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Event::ToExports(Index this_index) const -> ExportList {
  return MakeExportList(ExternalKind::Event, this_index, exports);
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
                         const TextList& data)
    : name{name},
      type{SegmentType::Active},
      memory{memory},
      offset{offset},
      data{data} {}

DataSegment::DataSegment(OptAt<BindVar> name, const TextList& data)
    : name{name}, type{SegmentType::Passive}, data{data} {}

auto ModuleItem::kind() const -> ModuleItemKind {
  return static_cast<ModuleItemKind>(desc.index());
}

bool ModuleItem::is_type_entry() const {
  return holds_alternative<At<TypeEntry>>(desc);
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

bool ModuleItem::is_event() const {
  return holds_alternative<At<Event>>(desc);
}

auto ModuleItem::type_entry() -> At<TypeEntry>& {
  return get<At<TypeEntry>>(desc);
}

auto ModuleItem::type_entry() const -> const At<TypeEntry>& {
  return get<At<TypeEntry>>(desc);
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

auto ModuleItem::event() -> At<Event>& {
  return get<At<Event>>(desc);
}

auto ModuleItem::event() const -> const At<Event>& {
  return get<At<Event>>(desc);
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


WASP_TEXT_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_TEXT_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)
WASP_OPERATOR_EQ_NE_1(text::Var, desc)
WASP_OPERATOR_EQ_NE_1(text::ModuleItem, desc)
WASP_OPERATOR_EQ_NE_1(text::Const, value)

bool operator==(const BoundValueTypeList& lhs, const ValueTypeList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                    [](auto&& lhs, auto&& rhs) { return lhs->type == rhs; });
}

bool operator!=(const BoundValueTypeList& lhs, const ValueTypeList& rhs) {
  return !(lhs == rhs);
}

}  // namespace text
}  // namespace wasp

WASP_TEXT_STRUCTS(WASP_STD_HASH_VARGS)
WASP_TEXT_CONTAINERS(WASP_STD_HASH_CONTAINER)
WASP_STD_HASH_1(text::Var, desc)
WASP_STD_HASH_1(text::ModuleItem, desc)
WASP_STD_HASH_1(text::Const, value)

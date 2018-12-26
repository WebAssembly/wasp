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

namespace wasp {
namespace binary {

inline MemArg::MemArg(u32 align_log2, u32 offset)
    : align_log2(align_log2), offset(offset) {}

inline Limits::Limits(u32 min) : min(min) {}

inline Limits::Limits(u32 min, u32 max) : min(min), max(max) {}

inline Locals::Locals(Index count, ValueType type) : count(count), type(type) {}

inline KnownSection::KnownSection(SectionId id, SpanU8 data)
    : id(id), data(data) {}

inline CustomSection::CustomSection(string_view name, SpanU8 data)
    : name(name), data(data) {}

template <typename T>
Section::Section(T&& contents) : contents(std::move(contents)) {}

inline bool Section::is_known() const {
  return contents.index() == 0;
}

inline bool Section::is_custom() const {
  return contents.index() == 1;
}

inline KnownSection& Section::known() {
  return get<KnownSection>(contents);
}

inline const KnownSection& Section::known() const {
  return get<KnownSection>(contents);
}

inline CustomSection& Section::custom() {
  return get<CustomSection>(contents);
}

inline const CustomSection& Section::custom() const {
  return get<CustomSection>(contents);
}

inline FunctionType::FunctionType(ValueTypes&& param_types,
                                  ValueTypes&& result_types)
    : param_types(std::move(param_types)),
      result_types(std::move(result_types)) {}

inline TypeEntry::TypeEntry(FunctionType&& type) : type(std::move(type)) {}

inline TableType::TableType(Limits limits, ElementType elemtype)
    : limits(limits), elemtype(elemtype) {}

inline MemoryType::MemoryType(Limits limits) : limits(limits) {}

inline GlobalType::GlobalType(ValueType valtype, Mutability mut)
    : valtype(valtype), mut(mut) {}

template <typename T>
Import::Import(string_view module, string_view name, T&& desc)
    : module(module), name(name), desc(std::move(desc)) {}

inline ExternalKind Import::kind() const {
  return static_cast<ExternalKind>(desc.index());
}

inline bool Import::is_function() const {
  return kind() == ExternalKind::Function;
}

inline bool Import::is_table() const {
  return kind() == ExternalKind::Table;
}

inline bool Import::is_memory() const {
  return kind() == ExternalKind::Memory;
}

inline bool Import::is_global() const {
  return kind() == ExternalKind::Global;
}

inline Index& Import::index() {
  return get<Index>(desc);
}

inline const Index& Import::index() const {
  return get<Index>(desc);
}

inline TableType& Import::table_type() {
  return get<TableType>(desc);
}

inline const TableType& Import::table_type() const {
  return get<TableType>(desc);
}

inline MemoryType& Import::memory_type() {
  return get<MemoryType>(desc);
}

inline const MemoryType& Import::memory_type() const {
  return get<MemoryType>(desc);
}

inline GlobalType& Import::global_type() {
  return get<GlobalType>(desc);
}

inline const GlobalType& Import::global_type() const {
  return get<GlobalType>(desc);
}

inline Expression::Expression(SpanU8 data) : data(data) {}

inline ConstantExpression::ConstantExpression(SpanU8 data) : data(data) {}

inline CallIndirectImmediate::CallIndirectImmediate(Index index, u8 reserved)
    : index(index), reserved(reserved) {}

inline BrTableImmediate::BrTableImmediate(std::vector<Index>&& targets,
                                          Index default_target)
    : targets(std::move(targets)), default_target(default_target) {}

inline Instruction::Instruction(Opcode opcode)
    : opcode(opcode), immediate(EmptyImmediate{}) {}

template <typename T>
Instruction::Instruction(Opcode opcode, T&& value)
    : opcode(opcode), immediate(std::move(value)) {}

inline bool Instruction::has_empty_immediate() const {
  return holds_alternative<EmptyImmediate>(immediate);
}

inline bool Instruction::has_block_type_immediate() const {
  return holds_alternative<BlockType>(immediate);
}

inline bool Instruction::has_index_immediate() const {
  return holds_alternative<Index>(immediate);
}

inline bool Instruction::has_call_indirect_immediate() const {
  return holds_alternative<CallIndirectImmediate>(immediate);
}

inline bool Instruction::has_br_table_immediate() const {
  return holds_alternative<BrTableImmediate>(immediate);
}

inline bool Instruction::has_u8_immediate() const {
  return holds_alternative<u8>(immediate);
}

inline bool Instruction::has_mem_arg_immediate() const {
  return holds_alternative<MemArg>(immediate);
}

inline bool Instruction::has_s32_immediate() const {
  return holds_alternative<s32>(immediate);
}

inline bool Instruction::has_s64_immediate() const {
  return holds_alternative<s64>(immediate);
}

inline bool Instruction::has_f32_immediate() const {
  return holds_alternative<f32>(immediate);
}

inline bool Instruction::has_f64_immediate() const {
  return holds_alternative<f64>(immediate);
}

inline EmptyImmediate& Instruction::empty_immediate() {
  return get<EmptyImmediate>(immediate);
}

inline const EmptyImmediate& Instruction::empty_immediate() const {
  return get<EmptyImmediate>(immediate);
}

inline BlockType& Instruction::block_type_immediate() {
  return get<BlockType>(immediate);
}

inline const BlockType& Instruction::block_type_immediate() const {
  return get<BlockType>(immediate);
}

inline Index& Instruction::index_immediate() {
  return get<Index>(immediate);
}

inline const Index& Instruction::index_immediate() const {
  return get<Index>(immediate);
}

inline CallIndirectImmediate& Instruction::call_indirect_immediate() {
  return get<CallIndirectImmediate>(immediate);
}

inline const CallIndirectImmediate& Instruction::call_indirect_immediate()
    const {
  return get<CallIndirectImmediate>(immediate);
}

inline BrTableImmediate& Instruction::br_table_immediate() {
  return get<BrTableImmediate>(immediate);
}

inline const BrTableImmediate& Instruction::br_table_immediate() const {
  return get<BrTableImmediate>(immediate);
}

inline u8& Instruction::u8_immediate() {
  return get<u8>(immediate);
}

inline const u8& Instruction::u8_immediate() const {
  return get<u8>(immediate);
}

inline MemArg& Instruction::mem_arg_immediate() {
  return get<MemArg>(immediate);
}

inline const MemArg& Instruction::mem_arg_immediate() const {
  return get<MemArg>(immediate);
}

inline s32& Instruction::s32_immediate() {
  return get<s32>(immediate);
}

inline const s32& Instruction::s32_immediate() const {
  return get<s32>(immediate);
}

inline s64& Instruction::s64_immediate() {
  return get<s64>(immediate);
}

inline const s64& Instruction::s64_immediate() const {
  return get<s64>(immediate);
}

inline f32& Instruction::f32_immediate() {
  return get<f32>(immediate);
}

inline const f32& Instruction::f32_immediate() const {
  return get<f32>(immediate);
}

inline f64& Instruction::f64_immediate() {
  return get<f64>(immediate);
}

inline const f64& Instruction::f64_immediate() const {
  return get<f64>(immediate);
}

inline Function::Function(Index type_index) : type_index(type_index) {}

inline Table::Table(TableType table_type) : table_type(table_type) {}

inline Memory::Memory(MemoryType memory_type) : memory_type(memory_type) {}

inline Global::Global(GlobalType global_type, ConstantExpression init)
    : global_type(global_type), init(init) {}

inline Export::Export(ExternalKind kind, string_view name, Index index)
    : kind(kind), name(name), index(index) {}

inline Start::Start(Index func_index) : func_index(func_index) {}

inline ElementSegment::ElementSegment(Index table_index,
                                      ConstantExpression offset,
                                      std::vector<Index>&& init)
    : table_index(table_index), offset(offset), init(std::move(init)) {}

inline Code::Code(std::vector<Locals>&& locals, Expression body)
    : locals(std::move(locals)), body(body) {}

inline DataSegment::DataSegment(Index memory_index,
                                ConstantExpression offset,
                                SpanU8 init)
    : memory_index(memory_index), offset(offset), init(init) {}

inline NameAssoc::NameAssoc(Index index, string_view name)
    : index(index), name(name) {}

inline IndirectNameAssoc::IndirectNameAssoc(Index index, NameMap&& name_map)
    : index(index), name_map(name_map) {}

inline NameSubsection::NameSubsection(NameSubsectionId id, SpanU8 data)
    : id(id), data(data) {}

}  // namespace binary
}  // namespace wasp

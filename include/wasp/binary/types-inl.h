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

namespace wasp {
namespace binary {

// ElementSegment
inline bool ElementSegment::has_indexes() const {
  return desc.index() == 0;
}

inline bool ElementSegment::has_expressions() const {
  return desc.index() == 1;
}

inline ElementSegment::IndexesInit& ElementSegment::indexes() {
  return get<IndexesInit>(desc);
}

inline const ElementSegment::IndexesInit& ElementSegment::indexes() const {
  return get<IndexesInit>(desc);
}

inline ElementSegment::ExpressionsInit& ElementSegment::expressions() {
  return get<ExpressionsInit>(desc);
}

inline const ElementSegment::ExpressionsInit& ElementSegment::expressions()
    const {
  return get<ExpressionsInit>(desc);
}

// Import
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

inline bool Import::is_event() const {
  return kind() == ExternalKind::Event;
}

inline At<Index>& Import::index() {
  return get<At<Index>>(desc);
}

inline const At<Index>& Import::index() const {
  return get<At<Index>>(desc);
}

inline At<TableType>& Import::table_type() {
  return get<At<TableType>>(desc);
}

inline const At<TableType>& Import::table_type() const {
  return get<At<TableType>>(desc);
}

inline At<MemoryType>& Import::memory_type() {
  return get<At<MemoryType>>(desc);
}

inline const At<MemoryType>& Import::memory_type() const {
  return get<At<MemoryType>>(desc);
}

inline At<GlobalType>& Import::global_type() {
  return get<At<GlobalType>>(desc);
}

inline const At<GlobalType>& Import::global_type() const {
  return get<At<GlobalType>>(desc);
}

inline At<EventType>& Import::event_type() {
  return get<At<EventType>>(desc);
}

inline const At<EventType>& Import::event_type() const {
  return get<At<EventType>>(desc);
}

// Instruction
inline bool Instruction::has_empty_immediate() const {
  return holds_alternative<EmptyImmediate>(immediate);
}

inline bool Instruction::has_block_type_immediate() const {
  return holds_alternative<At<BlockType>>(immediate);
}

inline bool Instruction::has_index_immediate() const {
  return holds_alternative<At<Index>>(immediate);
}

inline bool Instruction::has_call_indirect_immediate() const {
  return holds_alternative<At<CallIndirectImmediate>>(immediate);
}

inline bool Instruction::has_br_table_immediate() const {
  return holds_alternative<At<BrTableImmediate>>(immediate);
}

inline bool Instruction::has_br_on_exn_immediate() const {
  return holds_alternative<At<BrOnExnImmediate>>(immediate);
}

inline bool Instruction::has_u8_immediate() const {
  return holds_alternative<At<u8>>(immediate);
}

inline bool Instruction::has_mem_arg_immediate() const {
  return holds_alternative<At<MemArgImmediate>>(immediate);
}

inline bool Instruction::has_s32_immediate() const {
  return holds_alternative<At<s32>>(immediate);
}

inline bool Instruction::has_s64_immediate() const {
  return holds_alternative<At<s64>>(immediate);
}

inline bool Instruction::has_f32_immediate() const {
  return holds_alternative<At<f32>>(immediate);
}

inline bool Instruction::has_f64_immediate() const {
  return holds_alternative<At<f64>>(immediate);
}

inline bool Instruction::has_v128_immediate() const {
  return holds_alternative<At<v128>>(immediate);
}

inline bool Instruction::has_init_immediate() const {
  return holds_alternative<At<InitImmediate>>(immediate);
}

inline bool Instruction::has_copy_immediate() const {
  return holds_alternative<At<CopyImmediate>>(immediate);
}

inline bool Instruction::has_shuffle_immediate() const {
  return holds_alternative<At<ShuffleImmediate>>(immediate);
}

inline bool Instruction::has_value_types_immediate() const {
  return holds_alternative<ValueTypes>(immediate);
}

inline EmptyImmediate& Instruction::empty_immediate() {
  return get<EmptyImmediate>(immediate);
}

inline const EmptyImmediate& Instruction::empty_immediate() const {
  return get<EmptyImmediate>(immediate);
}

inline At<BlockType>& Instruction::block_type_immediate() {
  return get<At<BlockType>>(immediate);
}

inline const At<BlockType>& Instruction::block_type_immediate() const {
  return get<At<BlockType>>(immediate);
}

inline At<Index>& Instruction::index_immediate() {
  return get<At<Index>>(immediate);
}

inline const At<Index>& Instruction::index_immediate() const {
  return get<At<Index>>(immediate);
}

inline At<CallIndirectImmediate>& Instruction::call_indirect_immediate() {
  return get<At<CallIndirectImmediate>>(immediate);
}

inline const At<CallIndirectImmediate>& Instruction::call_indirect_immediate()
    const {
  return get<At<CallIndirectImmediate>>(immediate);
}

inline At<BrTableImmediate>& Instruction::br_table_immediate() {
  return get<At<BrTableImmediate>>(immediate);
}

inline const At<BrTableImmediate>& Instruction::br_table_immediate() const {
  return get<At<BrTableImmediate>>(immediate);
}

inline At<BrOnExnImmediate>& Instruction::br_on_exn_immediate() {
  return get<At<BrOnExnImmediate>>(immediate);
}

inline const At<BrOnExnImmediate>& Instruction::br_on_exn_immediate() const {
  return get<At<BrOnExnImmediate>>(immediate);
}

inline At<u8>& Instruction::u8_immediate() {
  return get<At<u8>>(immediate);
}

inline const At<u8>& Instruction::u8_immediate() const {
  return get<At<u8>>(immediate);
}

inline At<MemArgImmediate>& Instruction::mem_arg_immediate() {
  return get<At<MemArgImmediate>>(immediate);
}

inline const At<MemArgImmediate>& Instruction::mem_arg_immediate() const {
  return get<At<MemArgImmediate>>(immediate);
}

inline At<s32>& Instruction::s32_immediate() {
  return get<At<s32>>(immediate);
}

inline const At<s32>& Instruction::s32_immediate() const {
  return get<At<s32>>(immediate);
}

inline At<s64>& Instruction::s64_immediate() {
  return get<At<s64>>(immediate);
}

inline const At<s64>& Instruction::s64_immediate() const {
  return get<At<s64>>(immediate);
}

inline At<f32>& Instruction::f32_immediate() {
  return get<At<f32>>(immediate);
}

inline const At<f32>& Instruction::f32_immediate() const {
  return get<At<f32>>(immediate);
}

inline At<f64>& Instruction::f64_immediate() {
  return get<At<f64>>(immediate);
}

inline const At<f64>& Instruction::f64_immediate() const {
  return get<At<f64>>(immediate);
}

inline At<v128>& Instruction::v128_immediate() {
  return get<At<v128>>(immediate);
}

inline const At<v128>& Instruction::v128_immediate() const {
  return get<At<v128>>(immediate);
}

inline At<InitImmediate>& Instruction::init_immediate() {
  return get<At<InitImmediate>>(immediate);
}

inline const At<InitImmediate>& Instruction::init_immediate() const {
  return get<At<InitImmediate>>(immediate);
}

inline At<CopyImmediate>& Instruction::copy_immediate() {
  return get<At<CopyImmediate>>(immediate);
}

inline const At<CopyImmediate>& Instruction::copy_immediate() const {
  return get<At<CopyImmediate>>(immediate);
}

inline At<ShuffleImmediate>& Instruction::shuffle_immediate() {
  return get<At<ShuffleImmediate>>(immediate);
}

inline const At<ShuffleImmediate>& Instruction::shuffle_immediate() const {
  return get<At<ShuffleImmediate>>(immediate);
}

inline ValueTypes& Instruction::value_types_immediate() {
  return get<ValueTypes>(immediate);
}

inline const ValueTypes& Instruction::value_types_immediate() const {
  return get<ValueTypes>(immediate);
}

// Section
inline bool Section::is_known() const {
  return contents.index() == 0;
}

inline bool Section::is_custom() const {
  return contents.index() == 1;
}

inline At<KnownSection>& Section::known() {
  return get<At<KnownSection>>(contents);
}

inline const At<KnownSection>& Section::known() const {
  return get<At<KnownSection>>(contents);
}

inline At<CustomSection>& Section::custom() {
  return get<At<CustomSection>>(contents);
}

inline const At<CustomSection>& Section::custom() const {
  return get<At<CustomSection>>(contents);
}

inline At<SectionId> Section::id() const {
  return is_known() ? known()->id : MakeAt(SectionId::Custom);
}

inline SpanU8 Section::data() const {
  return is_known() ? known()->data : custom()->data;
}

}  // namespace binary
}  // namespace wasp

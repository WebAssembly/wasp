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

#include "wasp/binary/types.h"

#include "wasp/base/hash.h"
#include "wasp/base/macros.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

namespace wasp {
namespace binary {

ConstantExpression::ConstantExpression(const At<Instruction>& instruction)
    : instructions{{instruction}} {}

ConstantExpression::ConstantExpression(const InstructionList& instructions)
    : instructions{instructions} {}

DataSegment::DataSegment(OptAt<Index> memory_index,
                         OptAt<ConstantExpression> offset,
                         SpanU8 init)
    : type{SegmentType::Active},
      memory_index{memory_index},
      offset{offset},
      init{init} {}

DataSegment::DataSegment(SpanU8 init)
    : type{SegmentType::Passive}, init{init} {}

ElementExpression::ElementExpression(const At<Instruction>& instruction)
    : instructions{{instruction}} {}

ElementExpression::ElementExpression(const InstructionList& instructions)
    : instructions{instructions} {}

ElementSegment::ElementSegment(At<Index> table_index,
                               At<ConstantExpression> offset,
                               const ElementList& elements)
    : type{SegmentType::Active},
      table_index{table_index},
      offset{offset},
      elements{elements} {}

ElementSegment::ElementSegment(SegmentType type, const ElementList& elements)
    : type{type}, elements{elements} {}

bool ElementSegment::has_indexes() const {
  return elements.index() == 0;
}

bool ElementSegment::has_expressions() const {
  return elements.index() == 1;
}

ElementListWithIndexes& ElementSegment::indexes() {
  return get<ElementListWithIndexes>(elements);
}

const ElementListWithIndexes& ElementSegment::indexes() const {
  return get<ElementListWithIndexes>(elements);
}

ElementListWithExpressions& ElementSegment::expressions() {
  return get<ElementListWithExpressions>(elements);
}

const ElementListWithExpressions& ElementSegment::expressions() const {
  return get<ElementListWithExpressions>(elements);
}

At<ReferenceType> ElementSegment::elemtype() const {
  if (has_indexes()) {
    switch (indexes().kind) {
      case ExternalKind::Function:
        return ReferenceType::Funcref;

      case ExternalKind::Table:
      case ExternalKind::Memory:
      case ExternalKind::Global:
      case ExternalKind::Event:
        return ReferenceType::Externref;
    }
  } else {
    return expressions().elemtype;
  }
  WASP_UNREACHABLE();
}


Export::Export(At<ExternalKind> kind, At<string_view> name, At<Index> index)
    : kind{kind}, name{name}, index{index} {}

Export::Export(ExternalKind kind, string_view name, Index index)
    : kind{kind}, name{name}, index{index} {}

Import::Import(At<string_view> module, At<string_view> name, At<Index> desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(At<string_view> module, At<string_view> name, At<TableType> desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(At<string_view> module,
               At<string_view> name,
               At<MemoryType> desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(At<string_view> module,
               At<string_view> name,
               At<GlobalType> desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(At<string_view> module, At<string_view> name, At<EventType> desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(string_view module, string_view name, Index desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(string_view module, string_view name, TableType desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(string_view module, string_view name, MemoryType desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(string_view module, string_view name, GlobalType desc)
    : module{module}, name{name}, desc{desc} {}

Import::Import(string_view module, string_view name, EventType desc)
    : module{module}, name{name}, desc{desc} {}

ExternalKind Import::kind() const {
  return static_cast<ExternalKind>(desc.index());
}

bool Import::is_function() const {
  return kind() == ExternalKind::Function;
}

bool Import::is_table() const {
  return kind() == ExternalKind::Table;
}

bool Import::is_memory() const {
  return kind() == ExternalKind::Memory;
}

bool Import::is_global() const {
  return kind() == ExternalKind::Global;
}

bool Import::is_event() const {
  return kind() == ExternalKind::Event;
}

At<Index>& Import::index() {
  return get<At<Index>>(desc);
}

const At<Index>& Import::index() const {
  return get<At<Index>>(desc);
}

At<TableType>& Import::table_type() {
  return get<At<TableType>>(desc);
}

const At<TableType>& Import::table_type() const {
  return get<At<TableType>>(desc);
}

At<MemoryType>& Import::memory_type() {
  return get<At<MemoryType>>(desc);
}

const At<MemoryType>& Import::memory_type() const {
  return get<At<MemoryType>>(desc);
}

At<GlobalType>& Import::global_type() {
  return get<At<GlobalType>>(desc);
}

const At<GlobalType>& Import::global_type() const {
  return get<At<GlobalType>>(desc);
}

At<EventType>& Import::event_type() {
  return get<At<EventType>>(desc);
}

const At<EventType>& Import::event_type() const {
  return get<At<EventType>>(desc);
}

Instruction::Instruction(At<Opcode> opcode)
    : opcode(opcode), immediate() {}

Instruction::Instruction(At<Opcode> opcode, At<s32> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<s64> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<f32> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<f64> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<v128> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<Index> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BlockType> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BrOnExnImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BrTableImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<CallIndirectImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<CopyImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<InitImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<MemArgImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<ReferenceType> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<SelectImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<SimdLaneImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<ShuffleImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, s32 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, s64 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, f32 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, f64 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, Index immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, SimdLaneImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

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

bool Instruction::has_index_immediate() const {
  return holds_alternative<At<Index>>(immediate);
}


bool Instruction::has_block_type_immediate() const {
  return holds_alternative<At<BlockType>>(immediate);
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

At<Index>& Instruction::index_immediate() {
  return get<At<Index>>(immediate);
}

const At<Index>& Instruction::index_immediate() const {
  return get<At<Index>>(immediate);
}

At<BlockType>& Instruction::block_type_immediate() {
  return get<At<BlockType>>(immediate);
}

const At<BlockType>& Instruction::block_type_immediate() const {
  return get<At<BlockType>>(immediate);
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

const At<CallIndirectImmediate>& Instruction::call_indirect_immediate()
    const {
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


Section::Section(At<KnownSection> contents) : contents{contents} {}

Section::Section(At<CustomSection> contents) : contents{contents} {}

Section::Section(KnownSection contents) : contents{contents} {}

Section::Section(CustomSection contents) : contents{contents} {}

bool Section::is_known() const {
  return contents.index() == 0;
}

bool Section::is_custom() const {
  return contents.index() == 1;
}

At<KnownSection>& Section::known() {
  return get<At<KnownSection>>(contents);
}

const At<KnownSection>& Section::known() const {
  return get<At<KnownSection>>(contents);
}

At<CustomSection>& Section::custom() {
  return get<At<CustomSection>>(contents);
}

const At<CustomSection>& Section::custom() const {
  return get<At<CustomSection>>(contents);
}

At<SectionId> Section::id() const {
  return is_known() ? known()->id : MakeAt(SectionId::Custom);
}

SpanU8 Section::data() const {
  return is_known() ? known()->data : custom()->data;
}


WASP_BINARY_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_BINARY_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

bool operator==(const Module& lhs, const Module& rhs) {
  return lhs.types == rhs.types && lhs.imports == rhs.imports &&
         lhs.functions == rhs.functions && lhs.tables == rhs.tables &&
         lhs.memories == rhs.memories && lhs.globals == rhs.globals &&
         lhs.events == rhs.events && lhs.exports == rhs.exports &&
         lhs.starts == rhs.starts &&
         lhs.element_segments == rhs.element_segments &&
         lhs.data_counts == rhs.data_counts && lhs.codes == rhs.codes &&
         lhs.data_segments == rhs.data_segments;
}

bool operator!=(const Module& lhs, const Module& rhs) { return !(lhs == rhs); }

}  // namespace binary
}  // namespace wasp

WASP_BINARY_STRUCTS(WASP_STD_HASH_VARGS)
WASP_BINARY_CONTAINERS(WASP_STD_HASH_CONTAINER)

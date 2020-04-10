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

#include "src/base/operator_eq_ne_macros.h"
#include "src/base/std_hash_macros.h"

namespace wasp {
namespace binary {

DataSegment::DataSegment(OptAt<Index> memory_index,
                         OptAt<ConstantExpression> offset,
                         SpanU8 init)
    : type{SegmentType::Active},
      memory_index{memory_index},
      offset{offset},
      init{init} {}

DataSegment::DataSegment(SpanU8 init)
    : type{SegmentType::Passive}, init{init} {}

ElementSegment::ElementSegment(At<Index> table_index,
                               At<ConstantExpression> offset,
                               At<ExternalKind> kind,
                               const Indexes& init)
    : type{SegmentType::Active},
      table_index{table_index},
      offset{offset},
      desc{IndexesInit{kind, init}} {}

ElementSegment::ElementSegment(At<Index> table_index,
                               At<ConstantExpression> offset,
                               At<ElementType> element_type,
                               const ElementExpressions& init)
    : type{SegmentType::Active},
      table_index{table_index},
      offset{offset},
      desc{ExpressionsInit{element_type, init}} {}

ElementSegment::ElementSegment(SegmentType type,
                               At<ExternalKind> kind,
                               const Indexes& init)
    : type{type}, desc{IndexesInit{kind, init}} {}

ElementSegment::ElementSegment(SegmentType type,
                               At<ElementType> element_type,
                               const ElementExpressions& init)
    : type{type}, desc{ExpressionsInit{element_type, init}} {}

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

Instruction::Instruction(At<Opcode> opcode)
    : opcode(opcode), immediate(EmptyImmediate{}) {}

Instruction::Instruction(At<Opcode> opcode, EmptyImmediate immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BlockType> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<Index> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<CallIndirectImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BrTableImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<BrOnExnImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<u8> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<MemArgImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

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

Instruction::Instruction(At<Opcode> opcode, At<InitImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<CopyImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, At<ShuffleImmediate> immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(At<Opcode> opcode, const ValueTypes& immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode)
    : opcode(opcode), immediate(EmptyImmediate{}) {}

Instruction::Instruction(Opcode opcode, EmptyImmediate immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, BlockType immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, Index immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, CallIndirectImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, BrTableImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, BrOnExnImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, u8 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, MemArgImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, s32 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, s64 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, f32 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, f64 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, v128 immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, InitImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, CopyImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, ShuffleImmediate immediate)
    : opcode(opcode), immediate(MakeAt(immediate)) {}

Instruction::Instruction(Opcode opcode, const ValueTypes& immediate)
    : opcode(opcode), immediate(immediate) {}

Section::Section(At<KnownSection> contents) : contents{contents} {}

Section::Section(At<CustomSection> contents) : contents{contents} {}

Section::Section(KnownSection contents) : contents{contents} {}

Section::Section(CustomSection contents) : contents{contents} {}

WASP_OPERATOR_EQ_NE_2(BrOnExnImmediate, target, event_index)
WASP_OPERATOR_EQ_NE_2(BrTableImmediate, targets, default_target)
WASP_OPERATOR_EQ_NE_2(CallIndirectImmediate, index, table_index)
WASP_OPERATOR_EQ_NE_2(Code, locals, body)
WASP_OPERATOR_EQ_NE_1(ConstantExpression, instruction)
WASP_OPERATOR_EQ_NE_2(CopyImmediate, src_index, dst_index)
WASP_OPERATOR_EQ_NE_2(CustomSection, name, data)
WASP_OPERATOR_EQ_NE_1(DataCount, count)
WASP_OPERATOR_EQ_NE_4(DataSegment, type, memory_index, offset, init)
WASP_OPERATOR_EQ_NE_1(ElementExpression, instruction)
WASP_OPERATOR_EQ_NE_4(ElementSegment, type, table_index, offset, desc)
WASP_OPERATOR_EQ_NE_2(ElementSegment::IndexesInit, kind, init)
WASP_OPERATOR_EQ_NE_2(ElementSegment::ExpressionsInit, element_type, init)
WASP_OPERATOR_EQ_NE_0(EmptyImmediate)
WASP_OPERATOR_EQ_NE_1(Event, event_type)
WASP_OPERATOR_EQ_NE_2(EventType, attribute, type_index)
WASP_OPERATOR_EQ_NE_3(Export, kind, name, index)
WASP_OPERATOR_EQ_NE_1(Expression, data)
WASP_OPERATOR_EQ_NE_1(Function, type_index)
WASP_OPERATOR_EQ_NE_2(FunctionType, param_types, result_types)
WASP_OPERATOR_EQ_NE_2(Global, global_type, init)
WASP_OPERATOR_EQ_NE_2(GlobalType, valtype, mut)
WASP_OPERATOR_EQ_NE_3(Import, module, name, desc)
WASP_OPERATOR_EQ_NE_2(InitImmediate, segment_index, dst_index)
WASP_OPERATOR_EQ_NE_2(Instruction, opcode, immediate)
WASP_OPERATOR_EQ_NE_2(KnownSection, id, data)
WASP_OPERATOR_EQ_NE_2(Locals, count, type)
WASP_OPERATOR_EQ_NE_2(MemArgImmediate, align_log2, offset)
WASP_OPERATOR_EQ_NE_1(Memory, memory_type)
WASP_OPERATOR_EQ_NE_1(MemoryType, limits)
WASP_OPERATOR_EQ_NE_1(Section, contents)
WASP_OPERATOR_EQ_NE_1(Start, func_index)
WASP_OPERATOR_EQ_NE_1(Table, table_type)
WASP_OPERATOR_EQ_NE_2(TableType, limits, elemtype)
WASP_OPERATOR_EQ_NE_1(TypeEntry, type)

}  // namespace binary
}  // namespace wasp

WASP_STD_HASH_2(::wasp::binary::BrOnExnImmediate, target, event_index);
WASP_STD_HASH_2(::wasp::binary::CallIndirectImmediate, index, table_index)
WASP_STD_HASH_1(::wasp::binary::ConstantExpression, instruction)
WASP_STD_HASH_2(::wasp::binary::CopyImmediate, src_index, dst_index)
WASP_STD_HASH_2(::wasp::binary::CustomSection, name, data)
WASP_STD_HASH_1(::wasp::binary::DataCount, count)
WASP_STD_HASH_4(::wasp::binary::DataSegment, type, memory_index, offset, init)
WASP_STD_HASH_1(::wasp::binary::ElementExpression, instruction)
WASP_STD_HASH_4(::wasp::binary::ElementSegment, type, table_index, offset, desc)
WASP_STD_HASH_0(::wasp::binary::EmptyImmediate)
WASP_STD_HASH_1(::wasp::binary::Event, event_type)
WASP_STD_HASH_2(::wasp::binary::EventType, attribute, type_index)
WASP_STD_HASH_3(::wasp::binary::Export, kind, name, index)
WASP_STD_HASH_1(::wasp::binary::Expression, data)
WASP_STD_HASH_1(::wasp::binary::Function, type_index)
WASP_STD_HASH_2(::wasp::binary::Global, global_type, init)
WASP_STD_HASH_2(::wasp::binary::GlobalType, valtype, mut)
WASP_STD_HASH_3(::wasp::binary::Import, module, name, desc)
WASP_STD_HASH_2(::wasp::binary::InitImmediate, segment_index, dst_index)
WASP_STD_HASH_2(::wasp::binary::Instruction, opcode, immediate)
WASP_STD_HASH_2(::wasp::binary::KnownSection, id, data)
WASP_STD_HASH_2(::wasp::binary::Locals, count, type)
WASP_STD_HASH_2(::wasp::binary::MemArgImmediate, align_log2, offset)
WASP_STD_HASH_1(::wasp::binary::Memory, memory_type)
WASP_STD_HASH_1(::wasp::binary::MemoryType, limits)
WASP_STD_HASH_1(::wasp::binary::Section, contents)
WASP_STD_HASH_1(::wasp::binary::Start, func_index)
WASP_STD_HASH_1(::wasp::binary::Table, table_type)
WASP_STD_HASH_2(::wasp::binary::TableType, limits, elemtype)
WASP_STD_HASH_1(::wasp::binary::TypeEntry, type)

namespace std {
size_t hash<::wasp::binary::BrTableImmediate>::operator()(
    const ::wasp::binary::BrTableImmediate& v) const {
  return ::wasp::HashState::combine(0, ::wasp::HashContainer(v.targets),
                                    v.default_target);
}

size_t hash<::wasp::binary::Code>::operator()(
    const ::wasp::binary::Code& v) const {
  return ::wasp::HashState::combine(0, ::wasp::HashContainer(v.locals), v.body);
}

size_t hash<::wasp::binary::ElementSegment::IndexesInit>::operator()(
    const ::wasp::binary::ElementSegment::IndexesInit& v) const {
  return ::wasp::HashState::combine(0, v.kind, ::wasp::HashContainer(v.init));
}

size_t hash<::wasp::binary::ElementSegment::ExpressionsInit>::operator()(
    const ::wasp::binary::ElementSegment::ExpressionsInit& v) const {
  return ::wasp::HashState::combine(0, v.element_type,
                                    ::wasp::HashContainer(v.init));
}

size_t hash<::wasp::binary::FunctionType>::operator()(
    const ::wasp::binary::FunctionType& v) const {
  return ::wasp::HashState::combine(0, ::wasp::HashContainer(v.param_types),
                                    ::wasp::HashContainer(v.result_types));
}

size_t hash<::wasp::binary::ShuffleImmediate>::operator()(
    const ::wasp::binary::ShuffleImmediate& v) const {
  return ::wasp::HashContainer(v);
}

size_t hash<::wasp::binary::ValueTypes>::operator()(
    const ::wasp::binary::ValueTypes& v) const {
  return ::wasp::HashContainer(v);
}

}  // namespace std

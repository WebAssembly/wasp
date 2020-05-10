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

#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

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
                               const ElementList& elements)
    : type{SegmentType::Active},
      table_index{table_index},
      offset{offset},
      elements{elements} {}

ElementSegment::ElementSegment(SegmentType type, const ElementList& elements)
    : type{type}, elements{elements} {}

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

Section::Section(At<KnownSection> contents) : contents{contents} {}

Section::Section(At<CustomSection> contents) : contents{contents} {}

Section::Section(KnownSection contents) : contents{contents} {}

Section::Section(CustomSection contents) : contents{contents} {}

WASP_BINARY_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_BINARY_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

}  // namespace binary
}  // namespace wasp

WASP_BINARY_STRUCTS(WASP_STD_HASH_VARGS)
WASP_BINARY_CONTAINERS(WASP_STD_HASH_CONTAINER)

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

#include "src/binary/types.h"

namespace wasp {
namespace binary {

Limits::Limits(u32 min) : min(min) {}

Limits::Limits(u32 min, u32 max) : min(min), max(max) {}

Instruction::Instruction(Opcode opcode)
    : opcode(opcode), immediate(EmptyImmediate{}) {}

Instruction::Instruction(Opcode opcode, EmptyImmediate immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, BlockType immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, Index immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, CallIndirectImmediate immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, BrTableImmediate immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, u8 immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, MemArg immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, s32 immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, s64 immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, f32 immediate)
    : opcode(opcode), immediate(immediate) {}

Instruction::Instruction(Opcode opcode, f64 immediate)
    : opcode(opcode), immediate(immediate) {}

#define WASP_OPERATOR_EQ_NE_0(Name)                                  \
  bool operator==(const Name& lhs, const Name& rhs) { return true; } \
  bool operator!=(const Name& lhs, const Name& rhs) { return false; }

#define WASP_OPERATOR_EQ_NE_1(Name, f1)               \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1;                          \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#define WASP_OPERATOR_EQ_NE_2(Name, f1, f2)           \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2;      \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#define WASP_OPERATOR_EQ_NE_3(Name, f1, f2, f3)                      \
  bool operator==(const Name& lhs, const Name& rhs) {                \
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2 && lhs.f3 == rhs.f3; \
  }                                                                  \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

WASP_OPERATOR_EQ_NE_2(MemArg, align_log2, offset)
WASP_OPERATOR_EQ_NE_2(Limits, min, max)
WASP_OPERATOR_EQ_NE_2(Locals, count, type)
WASP_OPERATOR_EQ_NE_2(KnownSection, id, data)
WASP_OPERATOR_EQ_NE_2(CustomSection, name, data)
WASP_OPERATOR_EQ_NE_1(Section, contents)
WASP_OPERATOR_EQ_NE_2(FunctionType, param_types, result_types)
WASP_OPERATOR_EQ_NE_1(TypeEntry, type)
WASP_OPERATOR_EQ_NE_2(TableType, limits, elemtype)
WASP_OPERATOR_EQ_NE_1(MemoryType, limits)
WASP_OPERATOR_EQ_NE_2(GlobalType, valtype, mut)
WASP_OPERATOR_EQ_NE_3(Import, module, name, desc)
WASP_OPERATOR_EQ_NE_1(Expression, data)
WASP_OPERATOR_EQ_NE_1(ConstantExpression, data)
WASP_OPERATOR_EQ_NE_2(Instruction, opcode, immediate)
WASP_OPERATOR_EQ_NE_0(EmptyImmediate)
WASP_OPERATOR_EQ_NE_2(CallIndirectImmediate, index, reserved)
WASP_OPERATOR_EQ_NE_2(BrTableImmediate, targets, default_target)
WASP_OPERATOR_EQ_NE_1(Function, type_index)
WASP_OPERATOR_EQ_NE_1(Table, table_type)
WASP_OPERATOR_EQ_NE_1(Memory, memory_type)
WASP_OPERATOR_EQ_NE_2(Global, global_type, init)
WASP_OPERATOR_EQ_NE_3(Export, kind, name, index)
WASP_OPERATOR_EQ_NE_1(Start, func_index)
WASP_OPERATOR_EQ_NE_3(ElementSegment, table_index, offset, init)
WASP_OPERATOR_EQ_NE_2(Code, locals, body)
WASP_OPERATOR_EQ_NE_3(DataSegment, memory_index, offset, init)
WASP_OPERATOR_EQ_NE_2(NameAssoc, index, name)
WASP_OPERATOR_EQ_NE_2(IndirectNameAssoc, index, name_map)
WASP_OPERATOR_EQ_NE_2(NameSubsection, id, data)

#undef WASP_OPERATOR_EQ_NE_0
#undef WASP_OPERATOR_EQ_NE_1
#undef WASP_OPERATOR_EQ_NE_2
#undef WASP_OPERATOR_EQ_NE_3

}  // namespace binary
}  // namespace wasp

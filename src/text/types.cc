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

#include "include/wasp/text/types.h"

#include <algorithm>

#include "src/base/operator_eq_ne_macros.h"

namespace wasp {
namespace text {

Token::Token() : loc{}, type{TokenType::Eof}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type)
    : loc{loc}, type{type}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type, Opcode opcode)
    : loc{loc}, type{type}, immediate{opcode} {}

Token::Token(Location loc, TokenType type, ValueType valtype)
    : loc{loc}, type{type}, immediate{valtype} {}

Token::Token(Location loc, TokenType type, LiteralInfo info)
    : loc{loc}, type{type}, immediate{info} {}

Token::Token(Location loc, TokenType type, Text text)
    : loc{loc}, type{type}, immediate{text} {}

Token::Token(Location loc, TokenType type, Immediate immediate)
    : loc{loc}, type{type}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode) : opcode{opcode} {}

Instruction::Instruction(At<Opcode> opcode, At<u32> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<u64> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<f32> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<f64> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<v128> immediate)
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

Instruction::Instruction(At<Opcode> opcode, At<SelectImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<Var> immediate)
    : opcode{opcode}, immediate{immediate} {}

ElementSegment::ElementSegment(OptAt<BindVar> name,
                               OptAt<Var> table,
                               const InstructionList& offset,
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
                         const InstructionList& offset,
                         const TextList& data)
    : name{name},
      type{SegmentType::Active},
      memory{memory},
      offset{offset},
      data{data} {}

DataSegment::DataSegment(OptAt<BindVar> name, const TextList& data)
    : name{name}, type{SegmentType::Passive}, data{data} {}

WASP_OPERATOR_EQ_NE_4(LiteralInfo, sign, kind, base, has_underscores)
WASP_OPERATOR_EQ_NE_2(Text, text, byte_size)
WASP_OPERATOR_EQ_NE_3(Token, loc, type, immediate)
WASP_OPERATOR_EQ_NE_2(BoundValueType, name, type)
WASP_OPERATOR_EQ_NE_2(InlineImport, module, name)
WASP_OPERATOR_EQ_NE_1(InlineExport, name)
WASP_OPERATOR_EQ_NE_2(BoundFunctionType, params, results)
WASP_OPERATOR_EQ_NE_2(FunctionType, params, results)
WASP_OPERATOR_EQ_NE_2(FunctionTypeUse, type_use, type)
WASP_OPERATOR_EQ_NE_3(FunctionDesc, name, type_use, type)
WASP_OPERATOR_EQ_NE_1(TypeEntry, type)
WASP_OPERATOR_EQ_NE_2(Instruction, opcode, immediate)
WASP_OPERATOR_EQ_NE_2(BlockImmediate, label, type)
WASP_OPERATOR_EQ_NE_2(BrOnExnImmediate, target, event)
WASP_OPERATOR_EQ_NE_2(BrTableImmediate, targets, default_target)
WASP_OPERATOR_EQ_NE_2(CallIndirectImmediate, table, type)
WASP_OPERATOR_EQ_NE_2(CopyImmediate, dst, src)
WASP_OPERATOR_EQ_NE_2(InitImmediate, segment, dst)
WASP_OPERATOR_EQ_NE_2(MemArgImmediate, align, offset)
WASP_OPERATOR_EQ_NE_2(TableType, limits, elemtype)
WASP_OPERATOR_EQ_NE_2(TableDesc, name, type)
WASP_OPERATOR_EQ_NE_1(MemoryType, limits)
WASP_OPERATOR_EQ_NE_2(MemoryDesc, name, type)
WASP_OPERATOR_EQ_NE_2(GlobalType, valtype, mut)
WASP_OPERATOR_EQ_NE_2(GlobalDesc, name, type)
WASP_OPERATOR_EQ_NE_2(EventType, attribute, type)
WASP_OPERATOR_EQ_NE_2(EventDesc, name, type)
WASP_OPERATOR_EQ_NE_4(Table, desc, import, exports, elements)
WASP_OPERATOR_EQ_NE_3(Memory, desc, import, exports)
WASP_OPERATOR_EQ_NE_3(Event, desc, import, exports)
WASP_OPERATOR_EQ_NE_3(Import, module, name, desc)
WASP_OPERATOR_EQ_NE_3(Export, kind, name, var)
WASP_OPERATOR_EQ_NE_1(Start, var)
WASP_OPERATOR_EQ_NE_2(ElementListWithVars, kind, list)
WASP_OPERATOR_EQ_NE_2(ElementListWithExpressions, elemtype, list)
WASP_OPERATOR_EQ_NE_5(ElementSegment, name, type, table, offset, elements)
WASP_OPERATOR_EQ_NE_4(DataSegment, name, memory, offset, data)
WASP_OPERATOR_EQ_NE_3(ScriptModule, name, kind, module)
WASP_OPERATOR_EQ_NE_3(InvokeAction, module, name, consts)
WASP_OPERATOR_EQ_NE_2(GetAction, module, name)
WASP_OPERATOR_EQ_NE_1(ModuleAssertion, module)
WASP_OPERATOR_EQ_NE_2(ActionAssertion, action, message)
WASP_OPERATOR_EQ_NE_2(ReturnAssertion, action, results)
WASP_OPERATOR_EQ_NE_2(Assertion, kind, desc)
WASP_OPERATOR_EQ_NE_2(Register, name, module)

bool operator==(const VarList& lhs, const VarList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const VarList& lhs, const VarList& rhs) {
  return !(lhs == rhs);
}

bool operator==(const InlineExportList& lhs, const InlineExportList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const InlineExportList& lhs, const InlineExportList& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Function& lhs, const Function& rhs) {
  return lhs.desc == rhs.desc &&
         std::equal(lhs.locals.begin(), lhs.locals.end(), rhs.locals.begin(),
                    rhs.locals.end()) &&
         lhs.instructions == rhs.instructions && lhs.import == rhs.import &&
         lhs.exports == rhs.exports;
}

bool operator!=(const Function& lhs, const Function& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Global& lhs, const Global& rhs) {
  return lhs.desc == rhs.desc && lhs.init == rhs.init &&
         lhs.import == rhs.import && lhs.exports == rhs.exports;
}

bool operator!=(const Global& lhs, const Global& rhs) {
  return !(lhs == rhs);
}

bool operator==(const InstructionList& lhs, const InstructionList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const InstructionList& lhs, const InstructionList& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Module& lhs, const Module& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const Module& lhs, const Module& rhs) {
  return !(lhs == rhs);
}

bool operator==(const ConstList& lhs, const ConstList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const ConstList& lhs, const ConstList& rhs) {
  return !(lhs == rhs);
}

bool operator==(const ReturnResultList& lhs, const ReturnResultList& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const ReturnResultList& lhs, const ReturnResultList& rhs) {
  return !(lhs == rhs);
}

template <typename T, size_t N>
bool operator==(const std::array<FloatResult<T>, N>& lhs,
                const std::array<FloatResult<T>, N>& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, size_t N>
bool operator!=(const std::array<FloatResult<T>, N>& lhs,
                const std::array<FloatResult<T>, N>& rhs) {
  return !(lhs == rhs);
}

}  // namespace text
}  // namespace wasp

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

#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

namespace wasp {
namespace text {

Token::Token() : loc{}, type{TokenType::Eof}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type)
    : loc{loc}, type{type}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type, OpcodeInfo info)
    : loc{loc}, type{type}, immediate{info} {}

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

WASP_TEXT_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_TEXT_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

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

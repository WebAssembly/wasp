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

Instruction::Instruction(At<Opcode> opcode, At<ShuffleImmediate> immediate)
    : opcode{opcode}, immediate{immediate} {}

Instruction::Instruction(At<Opcode> opcode, At<Var> immediate)
    : opcode{opcode}, immediate{immediate} {}

auto Function::ToImport() -> OptAt<Import> {
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

auto Function::ToExports(Index this_index) -> ExportList {
  return MakeExportList(ExternalKind::Function, this_index, exports);
}

auto Table::ToImport() -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Table::ToExports(Index this_index) -> ExportList {
  return MakeExportList(ExternalKind::Table, this_index, exports);
}

auto Table::ToElementSegment(Index this_index) -> OptAt<ElementSegment> {
  if (!elements) {
    return nullopt;
  }
  return ElementSegment{
      nullopt, Var{this_index},
      InstructionList{Instruction{MakeAt(Opcode::I32Const), MakeAt(u32{0})}},
      *elements};
}

auto Memory::ToImport() -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Memory::ToExports(Index this_index) -> ExportList {
  return MakeExportList(ExternalKind::Memory, this_index, exports);
}

auto Memory::ToDataSegment(Index this_index) -> OptAt<DataSegment> {
  if (!data) {
    return nullopt;
  }
  return DataSegment{
      nullopt, Var{this_index},
      InstructionList{Instruction{MakeAt(Opcode::I32Const), MakeAt(u32{0})}},
      *data};
}

auto Global::ToImport() -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Global::ToExports(Index this_index) -> ExportList {
  return MakeExportList(ExternalKind::Global, this_index, exports);
}

auto Event::ToImport() -> OptAt<Import> {
  if (!import) {
    return nullopt;
  }
  return MakeAt(import->loc(),
                Import{import->value().module, import->value().name, desc});
}

auto Event::ToExports(Index this_index) -> ExportList {
  return MakeExportList(ExternalKind::Event, this_index, exports);
}

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

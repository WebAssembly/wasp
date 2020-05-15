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

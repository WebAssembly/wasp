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

#ifndef WASP_TEXT_TYPES_H_
#define WASP_TEXT_TYPES_H_

#include <iosfwd>
#include <utility>

#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/std_hash_macros.h"
#include "wasp/base/string_view.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"
#include "wasp/text/read/token.h"

namespace wasp {
namespace text {

using Var = variant<u32, string_view>;
using VarList = std::vector<At<Var>>;
using BindVar = string_view;

using TextList = std::vector<At<Text>>;

// TODO: Rename?
using ValueTypeList = wasp::ValueTypes;

struct FunctionType {
  ValueTypeList params;
  ValueTypeList results;
};

struct FunctionTypeUse {
  OptAt<Var> type_use;
  At<FunctionType> type;
};

struct BlockImmediate {
  OptAt<BindVar> label;
  FunctionTypeUse type;
};

struct BrOnExnImmediate {
  At<Var> target;
  At<Var> event;
};

struct BrTableImmediate {
  VarList targets;
  At<Var> default_target;
};

struct CallIndirectImmediate {
  OptAt<Var> table;
  FunctionTypeUse type;
};

struct CopyImmediate {
  OptAt<Var> dst;
  OptAt<Var> src;
};

struct InitImmediate {
  At<Var> segment;
  OptAt<Var> dst;
};

struct MemArgImmediate {
  OptAt<u32> align;
  OptAt<u32> offset;
};

using SelectImmediate = ValueTypeList;

struct Instruction {
  explicit Instruction(At<Opcode>);
  explicit Instruction(At<Opcode>, At<u32>);
  explicit Instruction(At<Opcode>, At<u64>);
  explicit Instruction(At<Opcode>, At<f32>);
  explicit Instruction(At<Opcode>, At<f64>);
  explicit Instruction(At<Opcode>, At<v128>);
  explicit Instruction(At<Opcode>, At<BlockImmediate>);
  explicit Instruction(At<Opcode>, At<BrOnExnImmediate>);
  explicit Instruction(At<Opcode>, At<BrTableImmediate>);
  explicit Instruction(At<Opcode>, At<CallIndirectImmediate>);
  explicit Instruction(At<Opcode>, At<CopyImmediate>);
  explicit Instruction(At<Opcode>, At<InitImmediate>);
  explicit Instruction(At<Opcode>, At<MemArgImmediate>);
  explicit Instruction(At<Opcode>, At<SelectImmediate>);
  explicit Instruction(At<Opcode>, At<ShuffleImmediate>);
  explicit Instruction(At<Opcode>, At<Var>);

  At<Opcode> opcode;
  variant<monostate,
          At<u32>,
          At<u64>,
          At<f32>,
          At<f64>,
          At<v128>,
          At<BlockImmediate>,
          At<BrOnExnImmediate>,
          At<BrTableImmediate>,
          At<CallIndirectImmediate>,
          At<CopyImmediate>,
          At<InitImmediate>,
          At<MemArgImmediate>,
          At<SelectImmediate>,
          At<ShuffleImmediate>,
          At<Var>>
      immediate;
};

using InstructionList = std::vector<At<Instruction>>;

// Section 1: Type

struct BoundValueType {
  OptAt<BindVar> name;
  At<ValueType> type;
};

using BoundValueTypeList = std::vector<At<BoundValueType>>;

struct BoundFunctionType {
  BoundValueTypeList params;
  ValueTypeList results;
};

struct TypeEntry {
  OptAt<BindVar> bind_var;
  At<BoundFunctionType> type;
};

// Section 2: Import

struct FunctionDesc {
  OptAt<BindVar> name;
  // Not using FunctionTypeUse, since that doesn't allow for bound params.
  OptAt<Var> type_use;
  At<BoundFunctionType> type;
};

struct TableType {
  At<Limits> limits;
  At<ElementType> elemtype;
};

struct TableDesc {
  OptAt<BindVar> name;
  At<TableType> type;
};

struct MemoryType {
  At<Limits> limits;
};

struct MemoryDesc {
  OptAt<BindVar> name;
  At<MemoryType> type;
};

struct GlobalType {
  At<ValueType> valtype;
  At<Mutability> mut;
};

struct GlobalDesc {
  OptAt<BindVar> name;
  At<GlobalType> type;
};

struct EventType {
  EventAttribute attribute;
  FunctionTypeUse type;
};

struct EventDesc {
  OptAt<BindVar> name;
  At<EventType> type;
};

struct Import {
  At<Text> module;
  At<Text> name;
  // TODO: Use At on variant members?
  variant<FunctionDesc, TableDesc, MemoryDesc, GlobalDesc, EventDesc> desc;
};

struct InlineImport {
  At<Text> module;
  At<Text> name;
};

// Section 3: Function

struct InlineExport {
  At<Text> name;
};

using InlineExportList = std::vector<At<InlineExport>>;

struct Export;

using ExportList = std::vector<At<Export>>;

struct Function {
  auto ToImport() -> OptAt<Import>;
  auto ToExports(Index this_index) -> ExportList;

  FunctionDesc desc;
  BoundValueTypeList locals;
  InstructionList instructions;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Section 4: Table

using ElementExpression = InstructionList;
using ElementExpressionList = std::vector<ElementExpression>;

struct ElementListWithExpressions {
  At<ElementType> elemtype;
  ElementExpressionList list;
};

struct ElementListWithVars {
  At<ExternalKind> kind;
  VarList list;
};

using ElementList = variant<ElementListWithVars, ElementListWithExpressions>;

struct ElementSegment;

struct Table {
  auto ToImport() -> OptAt<Import>;
  auto ToExports(Index this_index) -> ExportList;
  auto ToElementSegment(Index this_index) -> OptAt<ElementSegment>;

  TableDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<ElementList> elements;
};

// Section 5: Memory

struct DataSegment;

struct Memory {
  auto ToImport() -> OptAt<Import>;
  auto ToExports(Index this_index) -> ExportList;
  auto ToDataSegment(Index this_index) -> OptAt<DataSegment>;

  MemoryDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<TextList> data;
};

// Section 6: Global

struct Global {
  auto ToImport() -> OptAt<Import>;
  auto ToExports(Index this_index) -> ExportList;

  GlobalDesc desc;
  InstructionList init;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Section 7: Export

struct Export {
  At<ExternalKind> kind;
  At<Text> name;
  At<Var> var;
};

// Section 8: Start

struct Start {
  At<Var> var;
};

// Section 9: Elem

struct ElementSegment {
  // Active.
  explicit ElementSegment(OptAt<BindVar> name,
                          OptAt<Var> table,
                          const InstructionList& offset,
                          const ElementList&);

  // Passive or declared.
  explicit ElementSegment(OptAt<BindVar> name,
                          SegmentType,
                          const ElementList&);

  OptAt<BindVar> name;
  SegmentType type;
  OptAt<Var> table;
  optional<InstructionList> offset;
  ElementList elements;
};

// Section 10: Code (handled above in Func)

// Section 11: Data

struct DataSegment {
  // Active.
  explicit DataSegment(OptAt<BindVar> name,
                       OptAt<Var> memory,
                       const InstructionList& offset,
                       const TextList&);

  // Passive.
  explicit DataSegment(OptAt<BindVar> name, const TextList&);

  OptAt<BindVar> name;
  SegmentType type;
  OptAt<Var> memory;
  optional<InstructionList> offset;
  TextList data;
};

// Section 12: DataCount

// Section 13: Event

struct Event {
  auto ToImport() -> OptAt<Import>;
  auto ToExports(Index this_index) -> ExportList;

  EventDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
};

// Module

using ModuleItem = variant<TypeEntry,
                           Import,
                           Function,
                           Table,
                           Memory,
                           Global,
                           Export,
                           Start,
                           ElementSegment,
                           DataSegment,
                           Event>;

using Module = std::vector<At<ModuleItem>>;

// Script

using ModuleVar = string_view;

enum class ScriptModuleKind {
  Binary,  // (module bin "...")
  Text,    // (module ...)
  Quote    // (module quote "...")
};

struct ScriptModule {
  OptAt<BindVar> name;
  ScriptModuleKind kind;
  variant<Module, TextList> module;
};

struct RefNullConst {};

struct RefHostConst {
  At<u32> var;
};

using Const = variant<u32, u64, f32, f64, v128, RefNullConst, RefHostConst>;
using ConstList = std::vector<At<Const>>;

struct InvokeAction {
  OptAt<ModuleVar> module;
  At<Text> name;
  ConstList consts;
};

struct GetAction {
  OptAt<ModuleVar> module;
  At<Text> name;
};

using Action = variant<InvokeAction, GetAction>;

enum class AssertionKind {
  Malformed,
  Invalid,
  Unlinkable,
  ActionTrap,
  Return,
  ModuleTrap,
  Exhaustion,
};

struct ModuleAssertion {
  At<ScriptModule> module;
  At<Text> message;
};

struct ActionAssertion {
  At<Action> action;
  At<Text> message;
};

enum class NanKind { Canonical, Arithmetic };

template <typename T>
using FloatResult = variant<T, NanKind>;
using F32Result = FloatResult<f32>;
using F64Result = FloatResult<f64>;

using F32x4Result = std::array<F32Result, 4>;
using F64x2Result = std::array<F64Result, 2>;

struct RefAnyResult {};
struct RefFuncResult {};

using ReturnResult = variant<u32,
                             u64,
                             v128,
                             F32Result,
                             F64Result,
                             F32x4Result,
                             F64x2Result,
                             RefNullConst,
                             RefHostConst,
                             RefAnyResult,
                             RefFuncResult>;
using ReturnResultList = std::vector<At<ReturnResult>>;

struct ReturnAssertion {
  At<Action> action;
  ReturnResultList results;
};

struct Assertion {
  AssertionKind kind;
  variant<ModuleAssertion, ActionAssertion, ReturnAssertion> desc;
};

struct Register {
  At<Text> name;
  OptAt<ModuleVar> module;
};

using Command = variant<ScriptModule, Register, Action, Assertion>;

using Script = std::vector<At<Command>>;

#define WASP_TEXT_ENUMS(WASP_V) \
  WASP_V(text::TokenType)             \
  WASP_V(text::Sign)                  \
  WASP_V(text::LiteralKind)           \
  WASP_V(text::Base)                  \
  WASP_V(text::HasUnderscores)        \
  WASP_V(text::ScriptModuleKind)      \
  WASP_V(text::AssertionKind)         \
  WASP_V(text::NanKind)

#define WASP_TEXT_STRUCTS(WASP_V)                                        \
  WASP_V(text::LiteralInfo, 4, sign, kind, base, has_underscores)        \
  WASP_V(text::OpcodeInfo, 2, opcode, features)                          \
  WASP_V(text::Text, 2, text, byte_size)                                 \
  WASP_V(text::Token, 3, loc, type, immediate)                           \
  WASP_V(text::BoundValueType, 2, name, type)                            \
  WASP_V(text::BoundFunctionType, 2, params, results)                    \
  WASP_V(text::FunctionType, 2, params, results)                         \
  WASP_V(text::FunctionTypeUse, 2, type_use, type)                       \
  WASP_V(text::FunctionDesc, 3, name, type_use, type)                    \
  WASP_V(text::TypeEntry, 2, bind_var, type)                             \
  WASP_V(text::Instruction, 2, opcode, immediate)                        \
  WASP_V(text::BlockImmediate, 2, label, type)                           \
  WASP_V(text::BrOnExnImmediate, 2, target, event)                       \
  WASP_V(text::BrTableImmediate, 2, targets, default_target)             \
  WASP_V(text::CallIndirectImmediate, 2, table, type)                    \
  WASP_V(text::CopyImmediate, 2, dst, src)                               \
  WASP_V(text::InitImmediate, 2, segment, dst)                           \
  WASP_V(text::MemArgImmediate, 2, align, offset)                        \
  WASP_V(text::TableType, 2, limits, elemtype)                           \
  WASP_V(text::TableDesc, 2, name, type)                                 \
  WASP_V(text::MemoryType, 1, limits)                                    \
  WASP_V(text::MemoryDesc, 2, name, type)                                \
  WASP_V(text::GlobalType, 2, valtype, mut)                              \
  WASP_V(text::GlobalDesc, 2, name, type)                                \
  WASP_V(text::EventType, 2, attribute, type)                            \
  WASP_V(text::EventDesc, 2, name, type)                                 \
  WASP_V(text::InlineImport, 2, module, name)                            \
  WASP_V(text::InlineExport, 1, name)                                    \
  WASP_V(text::Function, 5, desc, locals, instructions, import, exports) \
  WASP_V(text::Table, 4, desc, import, exports, elements)                \
  WASP_V(text::Memory, 4, desc, import, exports, data)                   \
  WASP_V(text::Global, 4, desc, init, import, exports)                   \
  WASP_V(text::Event, 3, desc, import, exports)                          \
  WASP_V(text::Import, 3, module, name, desc)                            \
  WASP_V(text::Export, 3, kind, name, var)                               \
  WASP_V(text::Start, 1, var)                                            \
  WASP_V(text::ElementListWithExpressions, 2, elemtype, list)            \
  WASP_V(text::ElementListWithVars, 2, kind, list)                       \
  WASP_V(text::ElementSegment, 5, name, type, table, offset, elements)   \
  WASP_V(text::DataSegment, 5, name, type, memory, offset, data)         \
  WASP_V(text::ScriptModule, 3, name, kind, module)                      \
  WASP_V(text::RefNullConst, 0)                                          \
  WASP_V(text::RefHostConst, 1, var)                                     \
  WASP_V(text::InvokeAction, 3, module, name, consts)                    \
  WASP_V(text::GetAction, 2, module, name)                               \
  WASP_V(text::ModuleAssertion, 2, module, message)                      \
  WASP_V(text::ActionAssertion, 2, action, message)                      \
  WASP_V(text::RefAnyResult, 0)                                          \
  WASP_V(text::RefFuncResult, 0)                                         \
  WASP_V(text::ReturnAssertion, 2, action, results)                      \
  WASP_V(text::Assertion, 2, kind, desc)                                 \
  WASP_V(text::Register, 2, name, module)

#define WASP_TEXT_CONTAINERS(WASP_V)  \
  WASP_V(text::VarList)               \
  WASP_V(text::TextList)              \
  WASP_V(text::InstructionList)       \
  WASP_V(text::BoundValueTypeList)    \
  WASP_V(text::InlineExportList)      \
  WASP_V(text::ElementExpressionList) \
  WASP_V(text::Module)                \
  WASP_V(text::ConstList)             \
  WASP_V(text::F32x4Result)           \
  WASP_V(text::F64x2Result)           \
  WASP_V(text::ReturnResultList)      \
  WASP_V(text::Script)

WASP_TEXT_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_TEXT_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

bool operator==(const BoundValueTypeList& lhs, const ValueTypeList& rhs);
bool operator==(const ValueTypeList& lhs, const BoundValueTypeList& rhs);
bool operator!=(const BoundValueTypeList& lhs, const ValueTypeList& rhs);
bool operator!=(const ValueTypeList& lhs, const BoundValueTypeList& rhs);

// Used for gtest.

WASP_TEXT_ENUMS(WASP_DECLARE_PRINT_TO)
WASP_TEXT_STRUCTS(WASP_DECLARE_PRINT_TO)
WASP_TEXT_CONTAINERS(WASP_DECLARE_PRINT_TO)

}  // namespace text
}  // namespace wasp

WASP_TEXT_STRUCTS(WASP_DECLARE_STD_HASH)
WASP_TEXT_CONTAINERS(WASP_DECLARE_STD_HASH)

#endif  // WASP_TEXT_TYPES_H_

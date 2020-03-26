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

#include <utility>

#include "wasp/base/at.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/v128.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"

namespace wasp {
namespace text {

enum class TokenType {
#define WASP_V(Name) Name,
#include "wasp/text/token_type.def"
#undef WASP_V
};

enum class Sign { None, Plus, Minus };
enum class LiteralKind { Normal, Nan, NanPayload, Infinity };
enum class Base { Decimal, Hex };
enum class HasUnderscores { No, Yes };

struct LiteralInfo {
  static LiteralInfo HexNat(HasUnderscores);
  static LiteralInfo Nat(HasUnderscores);
  static LiteralInfo Number(Sign, HasUnderscores);
  static LiteralInfo HexNumber(Sign, HasUnderscores);
  static LiteralInfo Infinity(Sign);
  static LiteralInfo Nan(Sign);
  static LiteralInfo NanPayload(Sign, HasUnderscores);

  explicit LiteralInfo(LiteralKind);
  explicit LiteralInfo(Sign, LiteralKind, Base, HasUnderscores);

  Sign sign;
  LiteralKind kind;
  Base base;
  HasUnderscores has_underscores;
};

struct Text {
  string_view text;
  u32 byte_size;
};

struct Token {
  using Immediate = variant<monostate, Opcode, ValueType, LiteralInfo, Text>;

  Token();
  Token(Location, TokenType);
  Token(Location, TokenType, Opcode);
  Token(Location, TokenType, ValueType);
  Token(Location, TokenType, LiteralInfo);
  Token(Location, TokenType, Text);
  Token(Location, TokenType, Immediate);

  SpanU8 span_u8() const;
  At<string_view> string_view() const;

  bool has_opcode() const;
  bool has_value_type() const;
  bool has_literal_info() const;
  bool has_text() const;

  At<Opcode> opcode() const;
  At<ValueType> value_type() const;
  LiteralInfo literal_info() const;
  Text text() const;

  Location loc;
  TokenType type;
  Immediate immediate;
};

using Var = variant<u32, string_view>;
using VarList = std::vector<At<Var>>;
using BindVar = string_view;

using TextList = std::vector<At<Text>>;

using ValueTypeList = std::vector<At<ValueType>>;

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

struct Function {
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

struct Table {
  TableDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<ElementList> elements;
};

// Section 5: Memory

struct Memory {
  MemoryDesc desc;
  OptAt<InlineImport> import;
  InlineExportList exports;
  optional<TextList> data;
};

// Section 6: Global

struct Global {
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

#define WASP_TYPES(WASP_V)           \
  WASP_V(LiteralInfo)                \
  WASP_V(Text)                       \
  WASP_V(Token)                      \
  WASP_V(BoundValueType)             \
  WASP_V(BoundFunctionType)          \
  WASP_V(FunctionType)               \
  WASP_V(FunctionTypeUse)            \
  WASP_V(FunctionDesc)               \
  WASP_V(TypeEntry)                  \
  WASP_V(Instruction)                \
  WASP_V(BlockImmediate)             \
  WASP_V(BrOnExnImmediate)           \
  WASP_V(BrTableImmediate)           \
  WASP_V(CallIndirectImmediate)      \
  WASP_V(CopyImmediate)              \
  WASP_V(InitImmediate)              \
  WASP_V(MemArgImmediate)            \
  WASP_V(TableType)                  \
  WASP_V(TableDesc)                  \
  WASP_V(MemoryType)                 \
  WASP_V(MemoryDesc)                 \
  WASP_V(GlobalType)                 \
  WASP_V(GlobalDesc)                 \
  WASP_V(EventType)                  \
  WASP_V(EventDesc)                  \
  WASP_V(InlineImport)               \
  WASP_V(InlineExport)               \
  WASP_V(InlineExportList)           \
  WASP_V(Function)                   \
  WASP_V(Table)                      \
  WASP_V(Memory)                     \
  WASP_V(Global)                     \
  WASP_V(Event)                      \
  WASP_V(Import)                     \
  WASP_V(Export)                     \
  WASP_V(Start)                      \
  WASP_V(InstructionList)            \
  WASP_V(ElementListWithExpressions) \
  WASP_V(ElementListWithVars)        \
  WASP_V(ElementSegment)             \
  WASP_V(DataSegment)

#define WASP_DECLARE_OPERATOR_EQ_NE(Type)    \
  bool operator==(const Type&, const Type&); \
  bool operator!=(const Type&, const Type&);

WASP_TYPES(WASP_DECLARE_OPERATOR_EQ_NE)

#undef WASP_DECLARE_OPERATOR_EQ_NE

}  // namespace text
}  // namespace wasp

#include "wasp/text/types-inl.h"

#endif  // WASP_TEXT_TYPES_H_

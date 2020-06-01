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

#ifndef WASP_TEXT_READ_H_
#define WASP_TEXT_READ_H_

#include "wasp/base/at.h"
#include "wasp/base/optional.h"
#include "wasp/text/read/token.h"
#include "wasp/text/types.h"

namespace wasp {
namespace text {

struct Context;
struct NameMap;
class Tokenizer;

auto Expect(Tokenizer&, Context&, TokenType expected) -> optional<Token>;
auto ExpectLpar(Tokenizer&, Context&, TokenType expected) -> optional<Token>;

auto ReadNat32(Tokenizer&, Context&) -> OptAt<u32>;
template <typename T>
auto ReadInt(Tokenizer&, Context&) -> OptAt<T>;
template <typename T>
auto ReadFloat(Tokenizer&, Context&) -> OptAt<T>;

auto ReadVar(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadVarOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadVarList(Tokenizer&, Context&) -> optional<VarList>;
auto ReadNonEmptyVarList(Tokenizer&, Context&) -> optional<VarList>;

auto ReadVarUseOpt(Tokenizer&, Context&, TokenType) -> OptAt<Var>;
auto ReadTypeUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadFunctionTypeUse(Tokenizer&, Context&) -> optional<FunctionTypeUse>;

auto ReadText(Tokenizer&, Context&) -> OptAt<Text>;
auto ReadUtf8Text(Tokenizer&, Context&) -> OptAt<Text>;
auto ReadTextList(Tokenizer&, Context&) -> optional<TextList>;

// Section 1: Type

auto ReadBindVarOpt(Tokenizer&, Context&, NameMap&) -> OptAt<BindVar>;

auto ReadBoundValueTypeList(Tokenizer&, Context&, NameMap&, TokenType)
    -> optional<BoundValueTypeList>;
auto ReadBoundParamList(Tokenizer&, Context&, NameMap&)
    -> optional<BoundValueTypeList>;

auto ReadUnboundValueTypeList(Tokenizer&, Context&, TokenType)
    -> optional<ValueTypeList>;
auto ReadParamList(Tokenizer&, Context&) -> optional<ValueTypeList>;
auto ReadResultList(Tokenizer&, Context&) -> optional<ValueTypeList>;

auto ReadValueType(Tokenizer&, Context&) -> OptAt<ValueType>;
auto ReadValueTypeList(Tokenizer&, Context&) -> optional<ValueTypeList>;

auto ReadBoundFunctionType(Tokenizer&, Context&, NameMap&)
    -> OptAt<BoundFunctionType>;
auto ReadTypeEntry(Tokenizer&, Context&) -> OptAt<TypeEntry>;

// Section 2: Import

auto ReadInlineImportOpt(Tokenizer&, Context&) -> OptAt<InlineImport>;
auto ReadImport(Tokenizer&, Context&) -> OptAt<Import>;

// Section 3: Function

auto ReadLocalList(Tokenizer&, Context&, NameMap&)
    -> optional<BoundValueTypeList>;
auto ReadFunctionType(Tokenizer&, Context&) -> OptAt<FunctionType>;
auto ReadFunction(Tokenizer&, Context&) -> OptAt<Function>;

// Section 4: Table

auto ReadLimits(Tokenizer&, Context&) -> OptAt<Limits>;
auto ReadReferenceKind(Tokenizer&, Context&) -> OptAt<ReferenceType>;
auto ReadReferenceType(Tokenizer&, Context&) -> OptAt<ReferenceType>;
auto ReadReferenceTypeOpt(Tokenizer&, Context&) -> OptAt<ReferenceType>;

auto ReadTableType(Tokenizer&, Context&) -> OptAt<TableType>;
auto ReadTable(Tokenizer&, Context&) -> OptAt<Table>;

// Section 5: Memory

auto ReadMemoryType(Tokenizer&, Context&) -> OptAt<MemoryType>;
auto ReadMemory(Tokenizer&, Context&) -> OptAt<Memory>;

// Section 6: Global

auto ReadConstantExpression(Tokenizer&, Context&) -> OptAt<ConstantExpression>;
auto ReadGlobalType(Tokenizer&, Context&) -> OptAt<GlobalType>;
auto ReadGlobal(Tokenizer&, Context&) -> OptAt<Global>;

// Section 7: Export

auto ReadInlineExport(Tokenizer&, Context&) -> OptAt<InlineExport>;
auto ReadInlineExportList(Tokenizer&, Context&) -> optional<InlineExportList>;
auto ReadExport(Tokenizer&, Context&) -> OptAt<Export>;

// Section 8: Start

auto ReadStart(Tokenizer&, Context&) -> OptAt<Start>;

// Section 9: Elem

auto ReadOffsetExpression(Tokenizer&, Context&) -> OptAt<ConstantExpression>;
auto ReadElementExpression(Tokenizer&, Context&) -> OptAt<ElementExpression>;
auto ReadElementExpressionList(Tokenizer&, Context&)
    -> optional<ElementExpressionList>;
auto ReadTableUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadElementSegment(Tokenizer&, Context&) -> OptAt<ElementSegment>;

// Section 10: Code

auto ReadNameEqNatOpt(Tokenizer&, Context&, TokenType, u32) -> OptAt<u32>;
auto ReadAlignOpt(Tokenizer&, Context&) -> OptAt<u32>;
auto ReadOffsetOpt(Tokenizer&, Context&) -> OptAt<u32>;

auto ReadSimdLane(Tokenizer&, Context&) -> OptAt<u8>;
auto ReadSimdShuffleImmediate(Tokenizer&, Context&) -> OptAt<ShuffleImmediate>;
template <typename T, size_t N>
auto ReadSimdValues(Tokenizer&, Context&) -> OptAt<v128>;

bool IsPlainInstruction(Token);
bool IsBlockInstruction(Token);
bool IsExpression(Tokenizer&);
bool IsElementExpression(Tokenizer&);
bool IsInstruction(Tokenizer&);

auto ReadPlainInstruction(Tokenizer&, Context&) -> OptAt<Instruction>;
auto ReadLabelOpt(Tokenizer&, Context&) -> OptAt<BindVar>;
bool ReadEndLabelOpt(Tokenizer&, Context&, OptAt<BindVar>);
auto ReadBlockImmediate(Tokenizer&, Context&) -> OptAt<BlockImmediate>;
void EndBlock(Context&);

bool ReadOpcodeOpt(Tokenizer&, Context&, InstructionList&, TokenType);
bool ExpectOpcode(Tokenizer&, Context&, InstructionList&, TokenType);
bool ReadBlockInstruction(Tokenizer&, Context&, InstructionList&);
bool ReadInstruction(Tokenizer&, Context&, InstructionList&);
bool ReadInstructionList(Tokenizer&, Context&, InstructionList&);
bool ReadExpression(Tokenizer&, Context&, InstructionList&);
bool ReadExpressionList(Tokenizer&, Context&, InstructionList&);

// Section 11: Data

auto ReadMemoryUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadDataSegment(Tokenizer&, Context&) -> OptAt<DataSegment>;

// Section 12: DataCount

// Section 13: Event

auto ReadEventType(Tokenizer&, Context&) -> OptAt<EventType>;
auto ReadEvent(Tokenizer&, Context&) -> OptAt<Event>;

// Module

bool IsModuleItem(Tokenizer&);
auto ReadModuleItem(Tokenizer&, Context&) -> OptAt<ModuleItem>;
auto ReadModule(Tokenizer&, Context&) -> optional<Module>;

// Script

auto ReadModuleVarOpt(Tokenizer&, Context&) -> OptAt<ModuleVar>;
auto ReadScriptModule(Tokenizer&, Context&) -> OptAt<ScriptModule>;

// Action
bool IsConst(Tokenizer&);
auto ReadConst(Tokenizer&, Context&) -> OptAt<Const>;
auto ReadConstList(Tokenizer&, Context&) -> optional<ConstList>;
auto ReadInvokeAction(Tokenizer&, Context&) -> OptAt<InvokeAction>;
auto ReadGetAction(Tokenizer&, Context&) -> OptAt<GetAction>;
auto ReadAction(Tokenizer&, Context&) -> OptAt<Action>;

// Assertion
auto ReadModuleAssertion(Tokenizer&, Context&) -> OptAt<ModuleAssertion>;
auto ReadActionAssertion(Tokenizer&, Context&) -> OptAt<ActionAssertion>;
template <typename T>
auto ReadFloatResult(Tokenizer&, Context&) -> OptAt<FloatResult<T>>;
template <typename T, size_t N>
auto ReadSimdFloatResult(Tokenizer&, Context&) -> OptAt<ReturnResult>;

bool IsReturnResult(Tokenizer&);
auto ReadReturnResult(Tokenizer&, Context&) -> OptAt<ReturnResult>;
auto ReadReturnResultList(Tokenizer&, Context&) -> optional<ReturnResultList>;
auto ReadReturnAssertion(Tokenizer&, Context&) -> OptAt<ReturnAssertion>;
auto ReadAssertion(Tokenizer&, Context&) -> OptAt<Assertion>;

auto ReadRegister(Tokenizer&, Context&) -> OptAt<Register>;

bool IsCommand(Tokenizer&);
auto ReadCommand(Tokenizer&, Context&) -> OptAt<Command>;
auto ReadScript(Tokenizer&, Context&) -> optional<Script>;

}
}

#endif  // WASP_TEXT_READ_H_

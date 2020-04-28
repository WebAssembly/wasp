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

#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/optional.h"
#include "wasp/text/context.h"
#include "wasp/text/lex.h"
#include "wasp/text/types.h"

namespace wasp {
namespace text {

auto Expect(Tokenizer&, Context&, TokenType expected) -> optional<Token>;
auto ExpectLpar(Tokenizer&, Context&, TokenType expected) -> optional<Token>;

auto ReadNat32(Tokenizer&, Context&) -> At<u32>;
template <typename T>
auto ReadInt(Tokenizer&, Context&) -> At<T>;
template <typename T>
auto ReadFloat(Tokenizer&, Context&) -> At<T>;

auto ReadVar(Tokenizer&, Context&) -> At<Var>;
auto ReadVarOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadVarList(Tokenizer&, Context&) -> VarList;

auto ReadVarUseOpt(Tokenizer&, Context&, TokenType) -> OptAt<Var>;
auto ReadTypeUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadFunctionTypeUse(Tokenizer&, Context&) -> FunctionTypeUse;

auto ReadText(Tokenizer&, Context&) -> At<Text>;
auto ReadUtf8Text(Tokenizer&, Context&) -> At<Text>;
auto ReadTextList(Tokenizer&, Context&) -> TextList;

// Section 1: Type

auto ReadBindVarOpt(Tokenizer&, Context&, NameMap&) -> OptAt<BindVar>;

auto ReadBoundValueTypeList(Tokenizer&, Context&, NameMap&, TokenType)
    -> BoundValueTypeList;
auto ReadBoundParamList(Tokenizer&, Context&, NameMap&) -> BoundValueTypeList;

auto ReadUnboundValueTypeList(Tokenizer&, Context&, TokenType) -> ValueTypeList;
auto ReadParamList(Tokenizer&, Context&) -> ValueTypeList;
auto ReadResultList(Tokenizer&, Context&) -> ValueTypeList;

auto ReadValueType(Tokenizer&, Context&) -> At<ValueType>;
auto ReadValueTypeList(Tokenizer&, Context&) -> ValueTypeList;

auto ReadBoundFunctionType(Tokenizer&, Context&, NameMap&)
    -> At<BoundFunctionType>;
auto ReadTypeEntry(Tokenizer&, Context&) -> At<TypeEntry>;

// Section 2: Import

auto ReadInlineImportOpt(Tokenizer&, Context&) -> OptAt<InlineImport>;
auto ReadImport(Tokenizer&, Context&) -> At<Import>;

// Section 3: Function

auto ReadLocalList(Tokenizer&, Context&, NameMap&) -> BoundValueTypeList;
auto ReadFunctionType(Tokenizer&, Context&) -> At<FunctionType>;
auto ReadFunction(Tokenizer&, Context&) -> At<Function>;

// Section 4: Table

auto ReadLimits(Tokenizer&, Context&) -> At<Limits>;
auto ReadElementType(Tokenizer&, Context&) -> At<ElementType>;
auto ReadElementTypeOpt(Tokenizer&, Context&) -> OptAt<ElementType>;

auto ReadTableType(Tokenizer&, Context&) -> At<TableType>;
auto ReadTable(Tokenizer&, Context&) -> At<Table>;

// Section 5: Memory

auto ReadMemoryType(Tokenizer&, Context&) -> At<MemoryType>;
auto ReadMemory(Tokenizer&, Context&) -> At<Memory>;

// Section 6: Global

auto ReadGlobalType(Tokenizer&, Context&) -> At<GlobalType>;
auto ReadGlobal(Tokenizer&, Context&) -> At<Global>;

// Section 7: Export

auto ReadInlineExportOpt(Tokenizer&, Context&) -> OptAt<InlineExport>;
auto ReadInlineExportList(Tokenizer&, Context&) -> InlineExportList;
auto ReadExport(Tokenizer&, Context&) -> At<Export>;

// Section 8: Start

auto ReadStart(Tokenizer&, Context&) -> At<Start>;

// Section 9: Elem

auto ReadOffsetExpression(Tokenizer&, Context&) -> InstructionList;
auto ReadElementExpression(Tokenizer&, Context&) -> ElementExpression;
auto ReadElementExpressionList(Tokenizer&, Context&) -> ElementExpressionList;
auto ReadTableUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadElementSegment(Tokenizer&, Context&) -> At<ElementSegment>;

// Section 10: Code

auto ReadNameEqNatOpt(Tokenizer&, Context&, TokenType, u32) -> OptAt<u32>;
auto ReadAlignOpt(Tokenizer&, Context&) -> OptAt<u32>;
auto ReadOffsetOpt(Tokenizer&, Context&) -> OptAt<u32>;

auto ReadSimdLane(Tokenizer&, Context&) -> At<u32>;
template <typename T, size_t N>
auto ReadSimdValues(Tokenizer&, Context&) -> At<v128>;

bool IsPlainInstruction(Token);
bool IsBlockInstruction(Token);
bool IsExpression(Tokenizer&);
bool IsElementExpression(Tokenizer&);
bool IsInstruction(Tokenizer&);

auto ReadPlainInstruction(Tokenizer&, Context&) -> At<Instruction>;
auto ReadLabelOpt(Tokenizer&, Context&) -> OptAt<BindVar>;
void ReadEndLabelOpt(Tokenizer&, Context&, OptAt<BindVar>);
auto ReadBlockImmediate(Tokenizer&, Context&) -> At<BlockImmediate>;
void EndBlock(Context&);

bool ReadOpcodeOpt(Tokenizer&, Context&, InstructionList&, TokenType);
void ExpectOpcode(Tokenizer&, Context&, InstructionList&, TokenType);
void ReadBlockInstruction(Tokenizer&, Context&, InstructionList&);
void ReadInstruction(Tokenizer&, Context&, InstructionList&);
void ReadInstructionList(Tokenizer&, Context&, InstructionList&);
void ReadExpression(Tokenizer&, Context&, InstructionList&);
void ReadExpressionList(Tokenizer&, Context&, InstructionList&);

// Section 11: Data

auto ReadMemoryUseOpt(Tokenizer&, Context&) -> OptAt<Var>;
auto ReadDataSegment(Tokenizer&, Context&) -> At<DataSegment>;

// Section 12: DataCount

// Section 13: Event

auto ReadEventType(Tokenizer&, Context&) -> At<EventType>;
auto ReadEvent(Tokenizer&, Context&) -> At<Event>;

// Module

auto ReadModuleItem(Tokenizer&, Context&) -> At<ModuleItem>;
auto ReadModule(Tokenizer&, Context&) -> Module;

// Script

auto ReadModuleVarOpt(Tokenizer&, Context&) -> OptAt<ModuleVar>;
auto ReadScriptModule(Tokenizer&, Context&) -> At<ScriptModule>;

// Action
auto ReadConst(Tokenizer&, Context&) -> At<Const>;
auto ReadConstList(Tokenizer&, Context&) -> ConstList;
auto ReadInvokeAction(Tokenizer&, Context&) -> At<InvokeAction>;
auto ReadGetAction(Tokenizer&, Context&) -> At<GetAction>;
auto ReadAction(Tokenizer&, Context&) -> At<Action>;

// Assertion
auto ReadModuleAssertion(Tokenizer&, Context&) -> At<ModuleAssertion>;
auto ReadActionAssertion(Tokenizer&, Context&) -> At<ActionAssertion>;
template <typename T>
auto ReadFloatResult(Tokenizer&, Context&) -> At<FloatResult<T>>;
template <typename T, size_t N>
auto ReadSimdFloatResult(Tokenizer&, Context&) -> At<ReturnResult>;
auto ReadReturnResult(Tokenizer&, Context&) -> At<ReturnResult>;
auto ReadReturnResultList(Tokenizer&, Context&) -> ReturnResultList;
auto ReadReturnAssertion(Tokenizer&, Context&) -> At<ReturnAssertion>;
auto ReadAssertion(Tokenizer&, Context&) -> At<Assertion>;

auto ReadRegister(Tokenizer&, Context&) -> At<Register>;

auto ReadCommand(Tokenizer&, Context&) -> At<Command>;
auto ReadScript(Tokenizer&, Context&) -> Script;

}
}

#endif  // WASP_TEXT_READ_H_

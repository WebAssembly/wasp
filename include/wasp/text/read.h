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

namespace wasp::text {

struct ReadCtx;
class Tokenizer;

auto Expect(Tokenizer&, ReadCtx&, TokenType expected) -> optional<Token>;
auto ExpectLpar(Tokenizer&, ReadCtx&, TokenType expected) -> optional<Token>;

template <typename T>
auto ReadNat(Tokenizer&, ReadCtx&) -> OptAt<T>;
auto ReadNat32(Tokenizer&, ReadCtx&) -> OptAt<u32>;
template <typename T>
auto ReadInt(Tokenizer&, ReadCtx&) -> OptAt<T>;
template <typename T>
auto ReadFloat(Tokenizer&, ReadCtx&) -> OptAt<T>;

auto ReadVar(Tokenizer&, ReadCtx&) -> OptAt<Var>;
auto ReadVarOpt(Tokenizer&, ReadCtx&) -> OptAt<Var>;
auto ReadVarList(Tokenizer&, ReadCtx&) -> optional<VarList>;
auto ReadNonEmptyVarList(Tokenizer&, ReadCtx&) -> optional<VarList>;

auto ReadVarUseOpt(Tokenizer&, ReadCtx&, TokenType) -> OptAt<Var>;
auto ReadTypeUseOpt(Tokenizer&, ReadCtx&) -> OptAt<Var>;
auto ReadFunctionTypeUse(Tokenizer&, ReadCtx&) -> optional<FunctionTypeUse>;

auto ReadText(Tokenizer&, ReadCtx&) -> OptAt<Text>;
auto ReadUtf8Text(Tokenizer&, ReadCtx&) -> OptAt<Text>;
auto ReadTextList(Tokenizer&, ReadCtx&) -> optional<TextList>;

// Section 1: Type

auto ReadBindVarOpt(Tokenizer&, ReadCtx&) -> OptAt<BindVar>;

auto ReadBoundValueTypeList(Tokenizer&, ReadCtx&, TokenType)
    -> optional<BoundValueTypeList>;
auto ReadBoundParamList(Tokenizer&, ReadCtx&) -> optional<BoundValueTypeList>;

auto ReadUnboundValueTypeList(Tokenizer&, ReadCtx&, TokenType)
    -> optional<ValueTypeList>;
auto ReadParamList(Tokenizer&, ReadCtx&) -> optional<ValueTypeList>;
auto ReadResultList(Tokenizer&, ReadCtx&) -> optional<ValueTypeList>;

auto ReadRtt(Tokenizer&, ReadCtx&) -> OptAt<Rtt>;

auto ReadValueType(Tokenizer&, ReadCtx&) -> OptAt<ValueType>;
auto ReadValueTypeList(Tokenizer&, ReadCtx&) -> optional<ValueTypeList>;

auto ReadBoundFunctionType(Tokenizer&, ReadCtx&) -> OptAt<BoundFunctionType>;

auto ReadStorageType(Tokenizer&, ReadCtx&) -> OptAt<StorageType>;
auto ReadFieldTypeContents(Tokenizer&, ReadCtx&) -> OptAt<FieldType>;
auto ReadFieldType(Tokenizer&, ReadCtx&) -> OptAt<FieldType>;
auto ReadFieldTypeList(Tokenizer&, ReadCtx&) -> optional<FieldTypeList>;

auto ReadStructType(Tokenizer&, ReadCtx&) -> OptAt<StructType>;
auto ReadArrayType(Tokenizer&, ReadCtx&) -> OptAt<ArrayType>;

auto ReadDefinedType(Tokenizer&, ReadCtx&) -> OptAt<DefinedType>;

// Section 2: Import

auto ReadInlineImportOpt(Tokenizer&, ReadCtx&) -> OptAt<InlineImport>;
auto ReadImport(Tokenizer&, ReadCtx&) -> OptAt<Import>;

// Section 3: Function

auto ReadLocalList(Tokenizer&, ReadCtx&) -> optional<BoundValueTypeList>;
auto ReadFunctionType(Tokenizer&, ReadCtx&) -> OptAt<FunctionType>;
auto ReadFunction(Tokenizer&, ReadCtx&) -> OptAt<Function>;

// Section 4: Table

enum class LimitsKind { Memory, Table };
enum class AllowFuncref { No, Yes };

auto ReadIndexTypeOpt(Tokenizer&, ReadCtx&) -> OptAt<IndexType>;
auto ReadLimits(Tokenizer&, ReadCtx&, LimitsKind) -> OptAt<Limits>;
auto ReadHeapType(Tokenizer&, ReadCtx&) -> OptAt<HeapType>;
auto ReadRefType(Tokenizer&, ReadCtx&) -> OptAt<RefType>;
auto ReadReferenceType(Tokenizer&, ReadCtx&, AllowFuncref = AllowFuncref::Yes)
    -> OptAt<ReferenceType>;
auto ReadReferenceTypeOpt(Tokenizer&,
                          ReadCtx&,
                          AllowFuncref = AllowFuncref::Yes)
    -> OptAt<ReferenceType>;

auto ReadTableType(Tokenizer&, ReadCtx&) -> OptAt<TableType>;
auto ReadTable(Tokenizer&, ReadCtx&) -> OptAt<Table>;

// Section 5: Memory

template <typename T>
bool ReadIntsIntoBuffer(Tokenizer&, ReadCtx&, Buffer&);
template <typename T>
bool ReadFloatsIntoBuffer(Tokenizer&, ReadCtx&, Buffer&);

auto ReadSimdConst(Tokenizer&, ReadCtx&) -> OptAt<v128>;
bool ReadSimdConstsIntoBuffer(Tokenizer&, ReadCtx&, Buffer&);

auto ReadNumericData(Tokenizer&, ReadCtx&) -> OptAt<NumericData>;
auto ReadDataItem(Tokenizer&, ReadCtx&) -> OptAt<DataItem>;
auto ReadDataItemList(Tokenizer&, ReadCtx&) -> optional<DataItemList>;
auto ReadMemoryType(Tokenizer&, ReadCtx&) -> OptAt<MemoryType>;
auto ReadMemory(Tokenizer&, ReadCtx&) -> OptAt<Memory>;

// Section 6: Global

auto ReadConstantExpression(Tokenizer&, ReadCtx&) -> OptAt<ConstantExpression>;
auto ReadGlobalType(Tokenizer&, ReadCtx&) -> OptAt<GlobalType>;
auto ReadGlobal(Tokenizer&, ReadCtx&) -> OptAt<Global>;

// Section 7: Export

auto ReadInlineExport(Tokenizer&, ReadCtx&) -> OptAt<InlineExport>;
auto ReadInlineExportList(Tokenizer&, ReadCtx&) -> optional<InlineExportList>;
auto ReadExport(Tokenizer&, ReadCtx&) -> OptAt<Export>;

// Section 8: Start

auto ReadStart(Tokenizer&, ReadCtx&) -> OptAt<Start>;

// Section 9: Elem

auto ReadOffsetExpression(Tokenizer&, ReadCtx&) -> OptAt<ConstantExpression>;
auto ReadElementExpression(Tokenizer&, ReadCtx&) -> OptAt<ElementExpression>;
auto ReadElementExpressionList(Tokenizer&, ReadCtx&)
    -> optional<ElementExpressionList>;
auto ReadTableUseOpt(Tokenizer&, ReadCtx&) -> OptAt<Var>;
auto ReadElementSegment(Tokenizer&, ReadCtx&) -> OptAt<ElementSegment>;

// Section 10: Code

auto ReadNameEqNatOpt(Tokenizer&, ReadCtx&, TokenType, u32) -> OptAt<u32>;
auto ReadAlignOpt(Tokenizer&, ReadCtx&) -> OptAt<u32>;
auto ReadOffsetOpt(Tokenizer&, ReadCtx&) -> OptAt<u32>;
auto ReadMemArgImmediate(Tokenizer&, ReadCtx&) -> OptAt<MemArgImmediate>;

auto ReadSimdLane(Tokenizer&, ReadCtx&) -> OptAt<u8>;
auto ReadSimdShuffleImmediate(Tokenizer&, ReadCtx&) -> OptAt<ShuffleImmediate>;
template <typename T, size_t N>
auto ReadSimdValues(Tokenizer&, ReadCtx&) -> OptAt<v128>;

auto ReadHeapType2Immediate(Tokenizer&, ReadCtx&) -> OptAt<HeapType2Immediate>;

bool IsPlainInstruction(Token);
bool IsBlockInstruction(Token);
bool IsExpression(Tokenizer&);
bool IsElementExpression(Tokenizer&);
bool IsInstruction(Tokenizer&);

auto ReadPlainInstruction(Tokenizer&, ReadCtx&) -> OptAt<Instruction>;
auto ReadLabelOpt(Tokenizer&, ReadCtx&) -> OptAt<BindVar>;
bool ReadEndLabelOpt(Tokenizer&, ReadCtx&, OptAt<BindVar>);
auto ReadBlockImmediate(Tokenizer&, ReadCtx&) -> OptAt<BlockImmediate>;
auto ReadLetImmediate(Tokenizer&, ReadCtx&) -> OptAt<LetImmediate>;
void EndBlock(ReadCtx&);

bool ReadOpcodeOpt(Tokenizer&, ReadCtx&, InstructionList&, TokenType);
bool ExpectOpcode(Tokenizer&, ReadCtx&, InstructionList&, TokenType);
bool ReadBlockInstruction(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadLetInstruction(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadInstruction(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadInstructionList(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadRparAsEndInstruction(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadExpression(Tokenizer&, ReadCtx&, InstructionList&);
bool ReadExpressionList(Tokenizer&, ReadCtx&, InstructionList&);

// Section 11: Data

auto ReadMemoryUseOpt(Tokenizer&, ReadCtx&) -> OptAt<Var>;
auto ReadDataSegment(Tokenizer&, ReadCtx&) -> OptAt<DataSegment>;

// Section 12: DataCount

// Section 13: Event

auto ReadEventType(Tokenizer&, ReadCtx&) -> OptAt<EventType>;
auto ReadEvent(Tokenizer&, ReadCtx&) -> OptAt<Event>;

// Module

bool IsModuleItem(Tokenizer&);
auto ReadModuleItem(Tokenizer&, ReadCtx&) -> OptAt<ModuleItem>;
auto ReadModule(Tokenizer&, ReadCtx&) -> optional<Module>;
auto ReadSingleModule(Tokenizer&, ReadCtx&) -> optional<Module>;

// Script

auto ReadModuleVarOpt(Tokenizer&, ReadCtx&) -> OptAt<ModuleVar>;
auto ReadScriptModule(Tokenizer&, ReadCtx&) -> OptAt<ScriptModule>;

// Action
bool IsConst(Tokenizer&);
auto ReadConst(Tokenizer&, ReadCtx&) -> OptAt<Const>;
auto ReadConstList(Tokenizer&, ReadCtx&) -> optional<ConstList>;
auto ReadInvokeAction(Tokenizer&, ReadCtx&) -> OptAt<InvokeAction>;
auto ReadGetAction(Tokenizer&, ReadCtx&) -> OptAt<GetAction>;
auto ReadAction(Tokenizer&, ReadCtx&) -> OptAt<Action>;

// Assertion
auto ReadModuleAssertion(Tokenizer&, ReadCtx&) -> OptAt<ModuleAssertion>;
auto ReadActionAssertion(Tokenizer&, ReadCtx&) -> OptAt<ActionAssertion>;
template <typename T>
auto ReadFloatResult(Tokenizer&, ReadCtx&) -> OptAt<FloatResult<T>>;
template <typename T, size_t N>
auto ReadSimdFloatResult(Tokenizer&, ReadCtx&) -> OptAt<ReturnResult>;

bool IsReturnResult(Tokenizer&);
auto ReadReturnResult(Tokenizer&, ReadCtx&) -> OptAt<ReturnResult>;
auto ReadReturnResultList(Tokenizer&, ReadCtx&) -> optional<ReturnResultList>;
auto ReadReturnAssertion(Tokenizer&, ReadCtx&) -> OptAt<ReturnAssertion>;
auto ReadAssertion(Tokenizer&, ReadCtx&) -> OptAt<Assertion>;

auto ReadRegister(Tokenizer&, ReadCtx&) -> OptAt<Register>;

bool IsCommand(Tokenizer&);
auto ReadCommand(Tokenizer&, ReadCtx&) -> OptAt<Command>;
auto ReadScript(Tokenizer&, ReadCtx&) -> optional<Script>;

}  // namespace wasp::text

#endif  // WASP_TEXT_READ_H_

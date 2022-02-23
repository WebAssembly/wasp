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

#include "wasp/binary/formatters.h"

#include "gtest/gtest.h"
#include "test/binary/constants.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/linking_section/formatters.h"
#include "wasp/binary/name_section/formatters.h"

#include "wasp/base/concat.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(BinaryFormattersTest, RefType) {
  EXPECT_EQ(R"((ref func))"_sv, concat(RefType_Func));
  EXPECT_EQ(R"((ref null func))"_sv, concat(RefType_NullFunc));
  EXPECT_EQ(R"((ref extern))"_sv, concat(RefType_Extern));
  EXPECT_EQ(R"((ref null extern))"_sv, concat(RefType_NullExtern));
  EXPECT_EQ(R"((ref eq))"_sv, concat(RefType_Eq));
  EXPECT_EQ(R"((ref null eq))"_sv, concat(RefType_NullEq));
  EXPECT_EQ(R"((ref i31))"_sv, concat(RefType_I31));
  EXPECT_EQ(R"((ref null i31))"_sv, concat(RefType_NullI31));
  EXPECT_EQ(R"((ref any))"_sv, concat(RefType_Any));
  EXPECT_EQ(R"((ref null any))"_sv, concat(RefType_NullAny));

  EXPECT_EQ(R"((ref 0))", concat(RefType_0));
  EXPECT_EQ(R"((ref null 0))", concat(RefType_Null0));
}

TEST(BinaryFormattersTest, ReferenceType) {
  // ReferenceKind
  EXPECT_EQ(R"(externref)", concat(RT_Externref));
  EXPECT_EQ(R"(funcref)", concat(RT_Funcref));
  EXPECT_EQ(R"(eqref)", concat(RT_Eqref));
  EXPECT_EQ(R"(i31ref)", concat(RT_I31ref));
  EXPECT_EQ(R"(anyref)", concat(RT_Anyref));

  // RefType
  EXPECT_EQ(R"((ref func))"_sv, concat(RT_RefFunc));
  EXPECT_EQ(R"((ref null func))"_sv, concat(RT_RefNullFunc));
  EXPECT_EQ(R"((ref extern))"_sv, concat(RT_RefExtern));
  EXPECT_EQ(R"((ref null extern))"_sv, concat(RT_RefNullExtern));
  EXPECT_EQ(R"((ref eq))"_sv, concat(RT_RefEq));
  EXPECT_EQ(R"((ref null eq))"_sv, concat(RT_RefNullEq));
  EXPECT_EQ(R"((ref i31))"_sv, concat(RT_RefI31));
  EXPECT_EQ(R"((ref null i31))"_sv, concat(RT_RefNullI31));
  EXPECT_EQ(R"((ref any))"_sv, concat(RT_RefAny));
  EXPECT_EQ(R"((ref null any))"_sv, concat(RT_RefNullAny));
  EXPECT_EQ(R"((ref 0))", concat(RT_Ref0));
  EXPECT_EQ(R"((ref null 0))", concat(RT_RefNull0));
}

TEST(BinaryFormattersTest, Rtt) {
  EXPECT_EQ(R"((rtt 0 func))", concat(RTT_0_Func));
  EXPECT_EQ(R"((rtt 0 extern))", concat(RTT_0_Extern));
  EXPECT_EQ(R"((rtt 0 eq))", concat(RTT_0_Eq));
  EXPECT_EQ(R"((rtt 0 i31))", concat(RTT_0_I31));
  EXPECT_EQ(R"((rtt 0 any))", concat(RTT_0_Any));
}

TEST(BinaryFormattersTest, ValueType) {
  // NumericType
  EXPECT_EQ(R"(i32)", concat(VT_I32));
  EXPECT_EQ(R"(i64)", concat(VT_I64));
  EXPECT_EQ(R"(f32)", concat(VT_F32));
  EXPECT_EQ(R"(f64)", concat(VT_F64));
  EXPECT_EQ(R"(v128)", concat(VT_V128));

  // ReferenceType
  EXPECT_EQ(R"(externref)", concat(VT_Externref));
  EXPECT_EQ(R"(funcref)", concat(VT_Funcref));
  EXPECT_EQ(R"(eqref)", concat(VT_Eqref));
  EXPECT_EQ(R"(i31ref)", concat(VT_I31ref));
  EXPECT_EQ(R"(anyref)", concat(VT_Anyref));
  EXPECT_EQ(R"((ref func))"_sv, concat(VT_RefFunc));
  EXPECT_EQ(R"((ref null func))"_sv, concat(VT_RefNullFunc));
  EXPECT_EQ(R"((ref extern))"_sv, concat(VT_RefExtern));
  EXPECT_EQ(R"((ref null extern))"_sv, concat(VT_RefNullExtern));
  EXPECT_EQ(R"((ref eq))"_sv, concat(VT_RefEq));
  EXPECT_EQ(R"((ref null eq))"_sv, concat(VT_RefNullEq));
  EXPECT_EQ(R"((ref i31))"_sv, concat(VT_RefI31));
  EXPECT_EQ(R"((ref null i31))"_sv, concat(VT_RefNullI31));
  EXPECT_EQ(R"((ref any))"_sv, concat(VT_RefAny));
  EXPECT_EQ(R"((ref null any))"_sv, concat(VT_RefNullAny));
  EXPECT_EQ(R"((ref 0))", concat(VT_Ref0));
  EXPECT_EQ(R"((ref null 0))", concat(VT_RefNull0));

  // Rtt
  EXPECT_EQ(R"((rtt 0 func))", concat(VT_RTT_0_Func));
  EXPECT_EQ(R"((rtt 0 extern))", concat(VT_RTT_0_Extern));
  EXPECT_EQ(R"((rtt 0 eq))", concat(VT_RTT_0_Eq));
  EXPECT_EQ(R"((rtt 0 i31))", concat(VT_RTT_0_I31));
  EXPECT_EQ(R"((rtt 0 any))", concat(VT_RTT_0_Any));
}

TEST(BinaryFormattersTest, BlockType) {
  EXPECT_EQ(R"([i32])", concat(BT_I32));
  EXPECT_EQ(R"([])", concat(BT_Void));
  EXPECT_EQ(R"(type[100])", concat(BlockType(Index{100})));
}

TEST(BinaryFormattersTest, StorageType) {
  // ValueType
  EXPECT_EQ(R"(i32)", concat(StorageType{VT_I32}));

  // PackedType
  EXPECT_EQ(R"(i8)", concat(StorageType{PackedType::I8}));
  EXPECT_EQ(R"(i16)", concat(StorageType{PackedType::I16}));
}

TEST(BinaryFormattersTest, FieldType) {
  EXPECT_EQ(R"(const i32)",
            concat(FieldType{StorageType{VT_I32}, Mutability::Const}));
}

TEST(BinaryFormattersTest, FieldTypeList) {
  EXPECT_EQ(R"([const i32 var f32])",
            concat(FieldTypeList{
                FieldType{StorageType{VT_I32}, Mutability::Const},
                FieldType{StorageType{VT_F32}, Mutability::Var},
            }));
}

TEST(BinaryFormattersTest, StructType) {
  EXPECT_EQ(R"((struct [const i32 var f32]))",
            concat(StructType{FieldTypeList{
                FieldType{StorageType{VT_I32}, Mutability::Const},
                FieldType{StorageType{VT_F32}, Mutability::Var},
            }}));
}

TEST(TextFormattersTest, ArrayType) {
  EXPECT_EQ(
      R"((array const i32))",
      concat(ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}}));
}

TEST(BinaryFormattersTest, ExternalKind) {
  EXPECT_EQ(R"(func)", concat(ExternalKind::Function));
}

TEST(BinaryFormattersTest, TagAttribute) {
  EXPECT_EQ(R"(exception)", concat(TagAttribute::Exception));
}

TEST(BinaryFormattersTest, Mutability) {
  EXPECT_EQ(R"(const)", concat(Mutability::Const));
}

TEST(BinaryFormattersTest, SegmentType) {
  EXPECT_EQ(R"(active)", concat(SegmentType::Active));
  EXPECT_EQ(R"(passive)", concat(SegmentType::Passive));
}

TEST(BinaryFormattersTest, Shared) {
  EXPECT_EQ(R"(unshared)", concat(Shared::No));
  EXPECT_EQ(R"(shared)", concat(Shared::Yes));
}

TEST(BinaryFormattersTest, NameSubsectionKind) {
  EXPECT_EQ(R"(locals)", concat(NameSubsectionId::LocalNames));
}

TEST(BinaryFormattersTest, LetImmediate) {
  EXPECT_EQ(R"({type [], locals []})",
            concat(LetImmediate{BT_Void, LocalsList{}}));
  EXPECT_EQ(
      R"({type type[0], locals [i32 ** 2]})",
      concat(LetImmediate{BlockType{Index{0}}, LocalsList{Locals{2, VT_I32}}}));
}

TEST(BinaryFormattersTest, MemArgImmediate) {
  EXPECT_EQ(R"({align 1, offset 2})", concat(MemArgImmediate{1, 2}));
}

TEST(BinaryFormattersTest, Limits) {
  EXPECT_EQ(R"({min 1})", concat(Limits{1}));
  EXPECT_EQ(R"({min 1, max 2})", concat(Limits{1, 2}));
  EXPECT_EQ(R"({min 1, max 2, shared})", concat(Limits{1, 2, Shared::Yes}));
  EXPECT_EQ(R"({min 1, i64})",
            concat(Limits{1, nullopt, Shared::No, IndexType::I64}));
  EXPECT_EQ(R"({min 1, max 2, i64})",
            concat(Limits{1, 2, Shared::No, IndexType::I64}));
}

TEST(BinaryFormattersTest, Locals) {
  EXPECT_EQ(R"(i32 ** 3)", concat(Locals{3, VT_I32}));
}

TEST(BinaryFormattersTest, KnownSection) {
  EXPECT_EQ(R"({id type, contents "\00\01\02"})",
            concat(KnownSection{SectionId::Type, "\x00\x01\x02"_su8}));
}

TEST(BinaryFormattersTest, CustomSection) {
  EXPECT_EQ(R"({name "custom", contents "\00\01\02"})",
            concat(CustomSection{"custom"_sv, "\x00\x01\x02"_su8}));
}

TEST(BinaryFormattersTest, Section) {
  auto span = "\x00\x01\x02"_su8;
  EXPECT_EQ(R"({id type, contents "\00\01\02"})",
            concat(Section{KnownSection{SectionId::Type, span}}));

  EXPECT_EQ(R"({name "custom", contents "\00\01\02"})",
            concat(Section{CustomSection{"custom"_sv, span}}));

  EXPECT_EQ(R"({id 100, contents "\00\01\02"})",
            concat(Section{KnownSection{static_cast<SectionId>(100), span}}));
}

TEST(BinaryFormattersTest, DefinedType) {
  EXPECT_EQ(R"([] -> [])", concat(DefinedType{FunctionType{{}, {}}}));
  EXPECT_EQ(R"([i32] -> [])", concat(DefinedType{FunctionType{{VT_I32}, {}}}));
}

TEST(BinaryFormattersTest, FunctionType) {
  EXPECT_EQ(R"([] -> [])", concat(FunctionType{{}, {}}));
  EXPECT_EQ(R"([i32] -> [])", concat(FunctionType{{VT_I32}, {}}));
  EXPECT_EQ(R"([i32 f32] -> [i64 f64])",
            concat(FunctionType{{VT_I32, VT_F32}, {VT_I64, VT_F64}}));
}

TEST(BinaryFormattersTest, TableType) {
  EXPECT_EQ(R"({min 1, max 2} funcref)",
            concat(TableType{Limits{1, 2}, RT_Funcref}));
}

TEST(BinaryFormattersTest, GlobalType) {
  EXPECT_EQ(R"(const f32)", concat(GlobalType{VT_F32, Mutability::Const}));
  EXPECT_EQ(R"(var i32)", concat(GlobalType{VT_I32, Mutability::Var}));
}

TEST(BinaryFormattersTest, TagType) {
  EXPECT_EQ(R"(exception 0)", concat(TagType{TagAttribute::Exception, 0}));
}

TEST(BinaryFormattersTest, Import) {
  // Function
  EXPECT_EQ(R"({module "a", name "b", desc func 3})",
            concat(Import{"a"_sv, "b"_sv, Index{3}}));

  // Table
  EXPECT_EQ(R"({module "c", name "d", desc table {min 1} funcref})",
            concat(Import{"c"_sv, "d"_sv, TableType{Limits{1}, RT_Funcref}}));

  // Memory
  EXPECT_EQ(R"({module "e", name "f", desc memory {min 0, max 4}})",
            concat(Import{"e"_sv, "f"_sv, MemoryType{Limits{0, 4}}}));

  // Global
  EXPECT_EQ(
      R"({module "g", name "h", desc global var i32})",
      concat(Import{"g"_sv, "h"_sv, GlobalType{VT_I32, Mutability::Var}}));

  // Tag
  EXPECT_EQ(
      R"({module "i", name "j", desc tag exception 0})",
      concat(Import{"i"_sv, "j"_sv, TagType{TagAttribute::Exception, 0}}));
}

TEST(BinaryFormattersTest, Export) {
  EXPECT_EQ(R"({name "f", desc func 0})",
            concat(Export{ExternalKind::Function, "f"_sv, Index{0}}));
  EXPECT_EQ(R"({name "t", desc table 1})",
            concat(Export{ExternalKind::Table, "t"_sv, Index{1}}));
  EXPECT_EQ(R"({name "m", desc memory 2})",
            concat(Export{ExternalKind::Memory, "m"_sv, Index{2}}));
  EXPECT_EQ(R"({name "g", desc global 3})",
            concat(Export{ExternalKind::Global, "g"_sv, Index{3}}));
  EXPECT_EQ(R"({name "e", desc tag 4})",
            concat(Export{ExternalKind::Tag, "e"_sv, Index{4}}));
}

TEST(BinaryFormattersTest, Expression) {
  EXPECT_EQ(R"("\00\01\02")", concat(Expression{"\00\01\02"_su8}));
}

TEST(BinaryFormattersTest, ConstantExpression) {
  EXPECT_EQ(R"(i32.add end)",
            concat(ConstantExpression{Instruction{Opcode::I32Add}}));
}

TEST(BinaryFormattersTest, ElementExpression) {
  EXPECT_EQ(R"(ref.null end)",
            concat(ElementExpression{Instruction{Opcode::RefNull}}));
}

TEST(BinaryFormattersTest, Opcode) {
  EXPECT_EQ(R"(memory.grow)", concat(Opcode::MemoryGrow));
}

TEST(BinaryFormattersTest, CallIndirectImmediate) {
  EXPECT_EQ(R"(1 0)", concat(CallIndirectImmediate{1u, 0}));
}

TEST(BinaryFormattersTest, BrTableImmediate) {
  EXPECT_EQ(R"([] 100)", concat(BrTableImmediate{{}, 100}));
  EXPECT_EQ(R"([1 2] 3)", concat(BrTableImmediate{{1, 2}, 3}));
}

TEST(BinaryFormattersTest, BrOnCastImmediate) {
  EXPECT_EQ(R"(1 func 0)", concat(BrOnCastImmediate{1, {HT_Func, HT_0}}));
}

TEST(BinaryFormattersTest, HeapType2Immediate) {
  EXPECT_EQ(R"(func 0)", concat(HeapType2Immediate{HT_Func, HT_0}));
}

TEST(BinaryFormattersTest, InitImmediate) {
  EXPECT_EQ(R"(1 0)", concat(InitImmediate{1u, 0}));
}

TEST(BinaryFormattersTest, CopyImmediate) {
  EXPECT_EQ(R"(0 0)", concat(CopyImmediate{0, 0}));
}

TEST(BinaryFormattersTest, FuncBindImmediate) {
  EXPECT_EQ(R"(0)", concat(FuncBindImmediate{0}));
}

TEST(BinaryFormattersTest, RttSubImmediate) {
  EXPECT_EQ(R"(1 func 0)", concat(RttSubImmediate{1, {HT_Func, HT_0}}));
}

TEST(BinaryFormattersTest, StructFieldImmediate) {
  EXPECT_EQ(R"(0 1)", concat(StructFieldImmediate{0, 1}));
}

TEST(BinaryFormattersTest, Instruction) {
  // nop
  EXPECT_EQ(R"(nop)", concat(Instruction{Opcode::Nop}));
  // block (result i32)
  EXPECT_EQ(R"(block [i32])", concat(Instruction{Opcode::Block, BT_I32}));
  // br 3
  EXPECT_EQ(R"(br 3)", concat(Instruction{Opcode::Br, Index{3u}}));
  // br_table 0 1 4
  EXPECT_EQ(R"(br_table [0 1] 4)",
            concat(Instruction{Opcode::BrTable, BrTableImmediate{{0, 1}, 4}}));
  // call_indirect 1 (w/ a reserved value of 0)
  EXPECT_EQ(
      R"(call_indirect 1 0)",
      concat(Instruction{Opcode::CallIndirect, CallIndirectImmediate{1, 0}}));
  // br_on_cast 1 func 0
  EXPECT_EQ(R"(br_on_cast 1 func 0)",
            concat(Instruction{Opcode::BrOnCast,
                               BrOnCastImmediate{1, {HT_Func, HT_0}}}));
  // memory.size (w/ a reserved value of 0)
  // TODO: Fix reserved byte output
#if 0
  EXPECT_EQ(R"(memory.size 0)", concat(Instruction{Opcode::MemorySize, u8{0}}));
#endif
  // let
  EXPECT_EQ(
      R"(let {type type[0], locals []})",
      concat(Instruction{Opcode::Let, LetImmediate{BlockType{Index{0}}, {}}}));
  // i32.load offset=10 align=4 (alignment is stored as power-of-two)
  EXPECT_EQ(R"(i32.load {align 2, offset 10})",
            concat(Instruction{Opcode::I32Load, MemArgImmediate{2, 10}}));
  // i32.const 100
  EXPECT_EQ(R"(i32.const 100)",
            concat(Instruction{Opcode::I32Const, s32{100}}));
  // i64.const 1000
  EXPECT_EQ(R"(i64.const 1000)",
            concat(Instruction{Opcode::I64Const, s64{1000}}));
  // f32.const 1.5
  EXPECT_EQ(R"(f32.const 1.5)",
            concat(Instruction{Opcode::F32Const, f32{1.5}}));
  // f64.const 6.25
  EXPECT_EQ(R"(f64.const 6.25)",
            concat(Instruction{Opcode::F64Const, f64{6.25}}));
  // v128.const i32x4 1 2 3 4
  EXPECT_EQ(R"(v128.const 0x1 0x2 0x3 0x4)",
            concat(Instruction{Opcode::V128Const,
                               v128{s32{1}, s32{2}, s32{3}, s32{4}}}));
  // ref.null func
  EXPECT_EQ(R"(ref.null func)", concat(Instruction{Opcode::RefNull, HT_Func}));
  // ref.test 0 func
  EXPECT_EQ(
      R"(ref.test 0 func)",
      concat(Instruction{Opcode::RefTest, HeapType2Immediate{HT_0, HT_Func}}));
  // memory.init 0 10
  EXPECT_EQ(R"(memory.init 0 10)",
            concat(Instruction{Opcode::MemoryInit, InitImmediate{0, 10}}));
  // memory.copy 1 2
  EXPECT_EQ(R"(memory.copy 1 2)",
            concat(Instruction{Opcode::MemoryCopy, CopyImmediate{1, 2}}));
  // func.bind 2
  EXPECT_EQ(R"(func.bind 2)",
            concat(Instruction{Opcode::FuncBind, FuncBindImmediate{2}}));
  // rtt.sub 1 func 0
  EXPECT_EQ(
      R"(rtt.sub 1 func 0)",
      concat(Instruction{Opcode::RttSub, RttSubImmediate{1, {HT_Func, HT_0}}}));
  // i8x16.shuffle
  EXPECT_EQ(R"(i8x16.shuffle [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16])",
            concat(Instruction{Opcode::I8X16Shuffle,
                               ShuffleImmediate{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                 11, 12, 13, 14, 15, 16}}}));
  // select (result i32)
  EXPECT_EQ(R"(select [i32])",
            concat(Instruction{Opcode::SelectT, ValueTypeList{VT_I32}}));
  // struct.get 1 2
  EXPECT_EQ(R"(struct.get 1 2)",
            concat(Instruction{Opcode::StructGet, StructFieldImmediate{1, 2}}));
}

TEST(BinaryFormattersTest, Function) {
  EXPECT_EQ(R"({type 1})", concat(Function{Index{1u}}));
}

TEST(BinaryFormattersTest, Table) {
  EXPECT_EQ(R"({type {min 1} funcref})",
            concat(Table{TableType{Limits{1}, RT_Funcref}}));
}

TEST(BinaryFormattersTest, Memory) {
  EXPECT_EQ(R"({type {min 2, max 3}})",
            concat(Memory{MemoryType{Limits{2, 3}}}));
}

TEST(BinaryFormattersTest, Global) {
  EXPECT_EQ(R"({type const i32, init i32.const 0 end})",
            concat(Global{
                GlobalType{VT_I32, Mutability::Const},
                ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}}));
}

TEST(BinaryFormattersTest, Start) {
  EXPECT_EQ(R"({func 1})", concat(Start{Index{1u}}));
}

TEST(BinaryFormattersTest, ElementSegment_Active) {
  EXPECT_EQ(R"({type func, init [2 3], mode active {table 1, offset nop end}})",
            concat(ElementSegment{
                Index{1u}, ConstantExpression{Instruction{Opcode::Nop}},
                ElementListWithIndexes{ExternalKind::Function, {2u, 3u}}}));
}

TEST(BinaryFormattersTest, ElementSegment_Passive) {
  EXPECT_EQ(
      R"({type funcref, init [ref.func 2 end ref.null end], mode passive})",
      concat(ElementSegment{
          SegmentType::Passive,
          ElementListWithExpressions{
              RT_Funcref,
              {ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}},
               ElementExpression{Instruction{Opcode::RefNull}}}}}));
}

TEST(BinaryFormattersTest, Code) {
  EXPECT_EQ(R"({locals [i32 ** 1], body "\0b"})",
            concat(Code{{Locals{1, VT_I32}}, Expression{"\x0b"_su8}}));
}

TEST(BinaryFormattersTest, DataSegment_Active) {
  EXPECT_EQ(
      R"({init "\12\34", mode active {memory 0, offset i32.const 0 end}})",
      concat(DataSegment{
          Index{0u}, ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
          "\x12\x34"_su8}));
}

TEST(BinaryFormattersTest, DataSegment_Passive) {
  EXPECT_EQ(R"({init "\12\34", mode passive})",
            concat(DataSegment{"\x12\x34"_su8}));
}

TEST(BinaryFormattersTest, DataCount) {
  EXPECT_EQ(R"({count 1})", concat(DataCount{1u}));
}

TEST(BinaryFormattersTest, NameAssoc) {
  EXPECT_EQ(R"(3 "hi")", concat(NameAssoc{3u, "hi"_sv}));
}

TEST(BinaryFormattersTest, IndirectNameAssoc) {
  EXPECT_EQ(R"(0 [1 "first" 2 "second"])",
            concat(IndirectNameAssoc{
                0u, {NameAssoc{1u, "first"_sv}, NameAssoc{2u, "second"_sv}}}));
}

TEST(BinaryFormattersTest, NameSubsection) {
  EXPECT_EQ(R"(module "\00\00\00")",
            concat(NameSubsection{NameSubsectionId::ModuleName, "\0\0\0"_su8}));

  EXPECT_EQ(
      R"(functions "\00\00\00")",
      concat(NameSubsection{NameSubsectionId::FunctionNames, "\0\0\0"_su8}));

  EXPECT_EQ(R"(locals "\00\00\00")",
            concat(NameSubsection{NameSubsectionId::LocalNames, "\0\0\0"_su8}));
}

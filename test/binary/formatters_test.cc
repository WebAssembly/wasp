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

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(BinaryFormattersTest, ValueType) {
  EXPECT_EQ(R"(i32)", format("{}", VT_I32));
  EXPECT_EQ(R"(  f32)", format("{:>5s}", VT_F32));
}

TEST(BinaryFormattersTest, BlockType) {
  EXPECT_EQ(R"([i32])", format("{}", BT_I32));
  EXPECT_EQ(R"([])", format("{}", BT_Void));
  EXPECT_EQ(R"(type[100])", format("{}", BlockType(Index{100})));
  EXPECT_EQ(R"(   [f64])", format("{:>8s}", BT_F64));
}

TEST(BinaryFormattersTest, ReferenceType) {
  EXPECT_EQ(R"(funcref)", format("{}", RT_Funcref));
  EXPECT_EQ(R"(   funcref)", format("{:>10s}", RT_Funcref));
}

TEST(BinaryFormattersTest, ExternalKind) {
  EXPECT_EQ(R"(func)", format("{}", ExternalKind::Function));
  EXPECT_EQ(R"(  global)", format("{:>8s}", ExternalKind::Global));
}

TEST(BinaryFormattersTest, EventAttribute) {
  EXPECT_EQ(R"(exception)", format("{}", EventAttribute::Exception));
  EXPECT_EQ(R"(  exception)", format("{:>11s}", EventAttribute::Exception));
}

TEST(BinaryFormattersTest, Mutability) {
  EXPECT_EQ(R"(const)", format("{}", Mutability::Const));
  EXPECT_EQ(R"(   var)", format("{:>6s}", Mutability::Var));
}

TEST(BinaryFormattersTest, SegmentType) {
  EXPECT_EQ(R"(active)", format("{}", SegmentType::Active));
  EXPECT_EQ(R"(passive)", format("{}", SegmentType::Passive));
  EXPECT_EQ(R"(  active)", format("{:>8s}", SegmentType::Active));
  EXPECT_EQ(R"(  passive)", format("{:>9s}", SegmentType::Passive));
}

TEST(BinaryFormattersTest, Shared) {
  EXPECT_EQ(R"(unshared)", format("{}", Shared::No));
  EXPECT_EQ(R"(shared)", format("{}", Shared::Yes));
  EXPECT_EQ(R"(  unshared)", format("{:>10s}", Shared::No));
  EXPECT_EQ(R"(  shared)", format("{:>8s}", Shared::Yes));
}

TEST(BinaryFormattersTest, NameSubsectionKind) {
  EXPECT_EQ(R"(locals)", format("{}", NameSubsectionId::LocalNames));
  EXPECT_EQ(R"(  module)", format("{:>8s}", NameSubsectionId::ModuleName));
}

TEST(BinaryFormattersTest, LetImmediate) {
  EXPECT_EQ(R"({type [], locals []})",
            format("{}", LetImmediate{BT_Void, LocalsList{}}));
  EXPECT_EQ(R"({type type[0], locals [i32 ** 2]})",
            format("{}", LetImmediate{BlockType{Index{0}},
                                      LocalsList{Locals{2, VT_I32}}}));
}

TEST(BinaryFormattersTest, MemArgImmediate) {
  EXPECT_EQ(R"({align 1, offset 2})", format("{}", MemArgImmediate{1, 2}));
  EXPECT_EQ(R"({align 0, offset 0} )", format("{:20s}", MemArgImmediate{0, 0}));
}

TEST(BinaryFormattersTest, Limits) {
  EXPECT_EQ(R"({min 1})", format("{}", Limits{1}));
  EXPECT_EQ(R"({min 1, max 2})", format("{}", Limits{1, 2}));
  EXPECT_EQ(R"({min 1, max 2, shared})",
            format("{}", Limits{1, 2, Shared::Yes}));
  EXPECT_EQ(R"(  {min 0})", format("{:>9s}", Limits{0}));
}

TEST(BinaryFormattersTest, Locals) {
  EXPECT_EQ(R"(i32 ** 3)", format("{}", Locals{3, VT_I32}));
  EXPECT_EQ(R"(  i32 ** 3)", format("{:>10s}", Locals{3, VT_I32}));
}

TEST(BinaryFormattersTest, KnownSection) {
  EXPECT_EQ(
      R"({id type, contents "\00\01\02"})",
      format("{}", KnownSection{SectionId::Type, "\x00\x01\x02"_su8}));

  EXPECT_EQ(
      R"(   {id code, contents ""})",
      format("{:>25s}", KnownSection{SectionId::Code, ""_su8}));
}

TEST(BinaryFormattersTest, CustomSection) {
  EXPECT_EQ(R"({name "custom", contents "\00\01\02"})",
            format("{}", CustomSection{"custom"_sv, "\x00\x01\x02"_su8}));

  EXPECT_EQ(R"(   {name "", contents ""})",
            format("{:>25s}", CustomSection{""_sv, ""_su8}));
}

TEST(BinaryFormattersTest, Section) {
  auto span = "\x00\x01\x02"_su8;
  EXPECT_EQ(R"({id type, contents "\00\01\02"})",
            format("{}", Section{KnownSection{SectionId::Type, span}}));

  EXPECT_EQ(R"({name "custom", contents "\00\01\02"})",
            format("{}", Section{CustomSection{"custom"_sv, span}}));

  EXPECT_EQ(
      R"({id 100, contents "\00\01\02"})",
      format("{}", Section{KnownSection{static_cast<SectionId>(100), span}}));

  EXPECT_EQ(R"(   {id data, contents ""})",
            format("{:>25s}", Section{KnownSection{SectionId::Data, ""_su8}}));
}

TEST(BinaryFormattersTest, TypeEntry) {
  EXPECT_EQ(R"([] -> [])", format("{}", TypeEntry{FunctionType{{}, {}}}));
  EXPECT_EQ(R"([i32] -> [])",
            format("{}", TypeEntry{FunctionType{{VT_I32}, {}}}));
  EXPECT_EQ(R"(  [] -> [])",
            format("{:>10s}", TypeEntry{FunctionType{{}, {}}}));
}

TEST(BinaryFormattersTest, FunctionType) {
  EXPECT_EQ(R"([] -> [])", format("{}", FunctionType{{}, {}}));
  EXPECT_EQ(R"([i32] -> [])", format("{}", FunctionType{{VT_I32}, {}}));
  EXPECT_EQ(R"([i32 f32] -> [i64 f64])",
            format("{}", FunctionType{{VT_I32, VT_F32}, {VT_I64, VT_F64}}));
  EXPECT_EQ(R"(  [] -> [])", format("{:>10s}", FunctionType{{}, {}}));
}

TEST(BinaryFormattersTest, TableType) {
  EXPECT_EQ(R"({min 1, max 2} funcref)",
            format("{}", TableType{Limits{1, 2}, RT_Funcref}));
  EXPECT_EQ(R"(  {min 0} funcref)",
            format("{:>17s}", TableType{Limits{0}, RT_Funcref}));
}

TEST(BinaryFormattersTest, GlobalType) {
  EXPECT_EQ(R"(const f32)",
            format("{}", GlobalType{VT_F32, Mutability::Const}));
  EXPECT_EQ(R"(var i32)", format("{}", GlobalType{VT_I32, Mutability::Var}));
  EXPECT_EQ(R"(   var f64)",
            format("{:>10s}", GlobalType{VT_F64, Mutability::Var}));
}

TEST(BinaryFormattersTest, EventType) {
  EXPECT_EQ(R"(exception 0)",
            format("{}", EventType{EventAttribute::Exception, 0}));
  EXPECT_EQ(R"(  exception 0)",
            format("{:>13s}", EventType{EventAttribute::Exception, 0}));
}

TEST(BinaryFormattersTest, Import) {
  // Function
  EXPECT_EQ(R"({module "a", name "b", desc func 3})",
            format("{}", Import{"a"_sv, "b"_sv, Index{3}}));

  // Table
  EXPECT_EQ(
      R"({module "c", name "d", desc table {min 1} funcref})",
      format("{}", Import{"c"_sv, "d"_sv, TableType{Limits{1}, RT_Funcref}}));

  // Memory
  EXPECT_EQ(R"({module "e", name "f", desc memory {min 0, max 4}})",
            format("{}", Import{"e"_sv, "f"_sv, MemoryType{Limits{0, 4}}}));

  // Global
  EXPECT_EQ(R"({module "g", name "h", desc global var i32})",
            format("{}", Import{"g"_sv, "h"_sv,
                                GlobalType{VT_I32, Mutability::Var}}));

  // Event
  EXPECT_EQ(R"({module "i", name "j", desc event exception 0})",
            format("{}", Import{"i"_sv, "j"_sv,
                                EventType{EventAttribute::Exception, 0}}));

  EXPECT_EQ(R"(  {module "", name "", desc func 0})",
            format("{:>35s}", Import{""_sv, ""_sv, Index{0}}));
}

TEST(BinaryFormattersTest, Export) {
  EXPECT_EQ(R"({name "f", desc func 0})",
            format("{}", Export{ExternalKind::Function, "f"_sv, Index{0}}));
  EXPECT_EQ(R"({name "t", desc table 1})",
            format("{}", Export{ExternalKind::Table, "t"_sv, Index{1}}));
  EXPECT_EQ(R"({name "m", desc memory 2})",
            format("{}", Export{ExternalKind::Memory, "m"_sv, Index{2}}));
  EXPECT_EQ(R"({name "g", desc global 3})",
            format("{}", Export{ExternalKind::Global, "g"_sv, Index{3}}));
  EXPECT_EQ(R"({name "e", desc event 4})",
            format("{}", Export{ExternalKind::Event, "e"_sv, Index{4}}));
  EXPECT_EQ(R"(    {name "", desc memory 0})",
            format("{:>28s}", Export{ExternalKind::Memory, ""_sv, Index{0}}));
}

TEST(BinaryFormattersTest, Expression) {
  EXPECT_EQ(R"("\00\01\02")", format("{}", Expression{"\00\01\02"_su8}));
  EXPECT_EQ(R"(   "\00")", format("{:>8s}", Expression{"\00"_su8}));
}

TEST(BinaryFormattersTest, ConstantExpression) {
  EXPECT_EQ(R"(i32.add end)",
            format("{}", ConstantExpression{Instruction{Opcode::I32Add}}));
  EXPECT_EQ(R"(   nop end)",
            format("{:>10s}", ConstantExpression{Instruction{Opcode::Nop}}));
}

TEST(BinaryFormattersTest, ElementExpression) {
  EXPECT_EQ(R"(ref.null end)",
            format("{}", ElementExpression{Instruction{Opcode::RefNull}}));
  EXPECT_EQ(R"(   ref.func 0 end)",
            format("{:>17s}",
                   ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}));
}

TEST(BinaryFormattersTest, Opcode) {
  EXPECT_EQ(R"(memory.grow)", format("{}", Opcode::MemoryGrow));
  EXPECT_EQ(R"(   nop)", format("{:>6s}", Opcode::Nop));
}

TEST(BinaryFormattersTest, CallIndirectImmediate) {
  EXPECT_EQ(R"(1 0)", format("{}", CallIndirectImmediate{1u, 0}));
  EXPECT_EQ(R"(  10 0)", format("{:>6s}", CallIndirectImmediate{10u, 0}));
}

TEST(BinaryFormattersTest, BrTableImmediate) {
  EXPECT_EQ(R"([] 100)", format("{}", BrTableImmediate{{}, 100}));
  EXPECT_EQ(R"([1 2] 3)", format("{}", BrTableImmediate{{1, 2}, 3}));
  EXPECT_EQ(R"(  [42] 0)", format("{:>8s}", BrTableImmediate{{42}, 0}));
}

TEST(BinaryFormattersTest, BrOnExnImmediate) {
  EXPECT_EQ(R"(0 100)", format("{}", BrOnExnImmediate{0, 100}));
  EXPECT_EQ(R"(  42 0)", format("{:>6s}", BrOnExnImmediate{42, 0}));
}

TEST(BinaryFormattersTest, InitImmediate) {
  EXPECT_EQ(R"(1 0)", format("{}", InitImmediate{1u, 0}));
  EXPECT_EQ(R"(  10 0)", format("{:>6s}", InitImmediate{10u, 0}));
}

TEST(BinaryFormattersTest, CopyImmediate) {
  EXPECT_EQ(R"(0 0)", format("{}", CopyImmediate{0, 0}));
  EXPECT_EQ(R"(   0 0)", format("{:>6s}", CopyImmediate{0, 0}));
}

TEST(BinaryFormattersTest, Instruction) {
  // nop
  EXPECT_EQ(R"(nop)", format("{}", Instruction{Opcode::Nop}));
  // block (result i32)
  EXPECT_EQ(R"(block [i32])", format("{}", Instruction{Opcode::Block, BT_I32}));
  // br 3
  EXPECT_EQ(R"(br 3)", format("{}", Instruction{Opcode::Br, Index{3u}}));
  // br_table 0 1 4
  EXPECT_EQ(
      R"(br_table [0 1] 4)",
      format("{}", Instruction{Opcode::BrTable, BrTableImmediate{{0, 1}, 4}}));
  // call_indirect 1 (w/ a reserved value of 0)
  EXPECT_EQ(
      R"(call_indirect 1 0)",
      format("{}",
             Instruction{Opcode::CallIndirect, CallIndirectImmediate{1, 0}}));
  // br_on_exn 1 2
  EXPECT_EQ(R"(br_on_exn 1 2)",
            format("{}", Instruction{Opcode::BrOnExn, BrOnExnImmediate{1, 2}}));
  // memory.size (w/ a reserved value of 0)
  EXPECT_EQ(R"(memory.size 0)",
            format("{}", Instruction{Opcode::MemorySize, u8{0}}));
  // let
  EXPECT_EQ(R"(let {type type[0], locals []})",
            format("{}", Instruction{Opcode::Let,
                                     LetImmediate{BlockType{Index{0}}, {}}}));
  // i32.load offset=10 align=4 (alignment is stored as power-of-two)
  EXPECT_EQ(R"(i32.load {align 2, offset 10})",
            format("{}", Instruction{Opcode::I32Load, MemArgImmediate{2, 10}}));
  // i32.const 100
  EXPECT_EQ(R"(i32.const 100)",
            format("{}", Instruction{Opcode::I32Const, s32{100}}));
  // i64.const 1000
  EXPECT_EQ(R"(i64.const 1000)",
            format("{}", Instruction{Opcode::I64Const, s64{1000}}));
  // f32.const 1.5
  EXPECT_EQ(R"(f32.const 1.500000)",
            format("{}", Instruction{Opcode::F32Const, f32{1.5}}));
  // f64.const 6.25
  EXPECT_EQ(R"(f64.const 6.250000)",
            format("{}", Instruction{Opcode::F64Const, f64{6.25}}));
  // v128.const i32x4 1 2 3 4
  EXPECT_EQ(R"(v128.const 0x1 0x2 0x3 0x4)",
            format("{}", Instruction{Opcode::V128Const,
                                     v128{s32{1}, s32{2}, s32{3}, s32{4}}}));
  // memory.init 0 10
  EXPECT_EQ(
      R"(memory.init 0 10)",
      format("{}", Instruction{Opcode::MemoryInit, InitImmediate{0, 10}}));
  // memory.copy 1 2
  EXPECT_EQ(R"(memory.copy 1 2)",
            format("{}", Instruction{Opcode::MemoryCopy, CopyImmediate{1, 2}}));
  // v8x16.shuffle
  EXPECT_EQ(
      R"(v8x16.shuffle [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16])",
      format("{}", Instruction{Opcode::V8X16Shuffle,
                               ShuffleImmediate{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                 11, 12, 13, 14, 15, 16}}}));
  // select (result i32)
  EXPECT_EQ(R"(select [i32])",
            format("{}", Instruction{Opcode::SelectT, ValueTypeList{VT_I32}}));

  EXPECT_EQ(R"(   i32.add)", format("{:>10s}", Instruction{Opcode::I32Add}));
}

TEST(BinaryFormattersTest, Function) {
  EXPECT_EQ(R"({type 1})", format("{}", Function{Index{1u}}));
  EXPECT_EQ(R"(  {type 1})", format("{:>10s}", Function{Index{1u}}));
}

TEST(BinaryFormattersTest, Table) {
  EXPECT_EQ(R"({type {min 1} funcref})",
            format("{}", Table{TableType{Limits{1}, RT_Funcref}}));
  EXPECT_EQ(R"(   {type {min 1} funcref})",
            format("{:>25s}", Table{TableType{Limits{1}, RT_Funcref}}));
}

TEST(BinaryFormattersTest, Memory) {
  EXPECT_EQ(R"({type {min 2, max 3}})",
            format("{}", Memory{MemoryType{Limits{2, 3}}}));
  EXPECT_EQ(R"( {type {min 0}})",
            format("{:>15s}", Memory{MemoryType{Limits{0}}}));
}

TEST(BinaryFormattersTest, Global) {
  EXPECT_EQ(R"({type const i32, init i32.const 0 end})",
            format("{}", Global{GlobalType{VT_I32, Mutability::Const},
                                ConstantExpression{
                                    Instruction{Opcode::I32Const, s32{0}}}}));
  EXPECT_EQ(
      R"(  {type var f32, init nop end})",
      format("{:>30s}", Global{GlobalType{VT_F32, Mutability::Var},
                               ConstantExpression{Instruction{Opcode::Nop}}}));
}

TEST(BinaryFormattersTest, Start) {
  EXPECT_EQ(R"({func 1})", format("{}", Start{Index{1u}}));
  EXPECT_EQ(R"(  {func 1})", format("{:>10s}", Start{Index{1u}}));
}

TEST(BinaryFormattersTest, ElementSegment_Active) {
  EXPECT_EQ(
      R"({type func, init [2 3], mode active {table 1, offset nop end}})",
      format("{}",
             ElementSegment{
                 Index{1u}, ConstantExpression{Instruction{Opcode::Nop}},
                 ElementListWithIndexes{ExternalKind::Function, {2u, 3u}}}));

  EXPECT_EQ(R"( {type func, init [], mode active {table 0, offset nop end}})",
            format("{:>60s}",
                   ElementSegment{
                       Index{0u}, ConstantExpression{Instruction{Opcode::Nop}},
                       ElementListWithIndexes{ExternalKind::Function, {}}}));
}

TEST(BinaryFormattersTest, ElementSegment_Passive) {
  EXPECT_EQ(
      R"({type funcref, init [ref.func 2 end ref.null end], mode passive})",
      format(
          "{}",
          ElementSegment{
              SegmentType::Passive,
              ElementListWithExpressions{
                  RT_Funcref,
                  {ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}},
                   ElementExpression{Instruction{Opcode::RefNull}}}}}));

  EXPECT_EQ(R"( {type funcref, init [], mode passive})",
            format("{:>38s}",
                   ElementSegment{SegmentType::Passive,
                                  ElementListWithExpressions{RT_Funcref, {}}}));
}

TEST(BinaryFormattersTest, Code) {
  EXPECT_EQ(R"({locals [i32 ** 1], body "\0b"})",
            format("{}", Code{{Locals{1, VT_I32}}, Expression{"\x0b"_su8}}));

  EXPECT_EQ(
      R"(     {locals [], body ""})",
      format("{:>25s}", Code{{}, Expression{""_su8}}));
}

TEST(BinaryFormattersTest, DataSegment_Active) {
  EXPECT_EQ(
      R"({init "\12\34", mode active {memory 0, offset i32.const 0 end}})",
      format("{}", DataSegment{Index{0u},
                               ConstantExpression{
                                   Instruction{Opcode::I32Const, s32{0}}},
                               "\x12\x34"_su8}));

  EXPECT_EQ(R"(  {init "", mode active {memory 0, offset nop end}})",
            format("{:>51s}",
                   DataSegment{Index{0u},
                               ConstantExpression{Instruction{Opcode::Nop}},
                               ""_su8}));
}

TEST(BinaryFormattersTest, DataSegment_Passive) {
  EXPECT_EQ(R"({init "\12\34", mode passive})",
            format("{}", DataSegment{"\x12\x34"_su8}));

  EXPECT_EQ(R"(  {init "", mode passive})",
            format("{:>25}", DataSegment{""_su8}));
}

TEST(BinaryFormattersTest, DataCount) {
  EXPECT_EQ(R"({count 1})", format("{}", DataCount{1u}));
  EXPECT_EQ(R"( {count 1})", format("{:>10s}", DataCount{1u}));
}

TEST(BinaryFormattersTest, NameAssoc) {
  EXPECT_EQ(R"(3 "hi")", format("{}", NameAssoc{3u, "hi"_sv}));
  EXPECT_EQ(R"(  0 "")", format("{:>6s}", NameAssoc{0u, ""_sv}));
}

TEST(BinaryFormattersTest, IndirectNameAssoc) {
  EXPECT_EQ(R"(0 [1 "first" 2 "second"])",
            format("{}", IndirectNameAssoc{0u,
                                           {NameAssoc{1u, "first"_sv},
                                            NameAssoc{2u, "second"_sv}}}));
  EXPECT_EQ(R"(  1 [10 "a" 100 "b"])",
            format("{:>20s}",
                   IndirectNameAssoc{
                       1u, {NameAssoc{10u, "a"_sv}, NameAssoc{100u, "b"_sv}}}));
}

TEST(BinaryFormattersTest, NameSubsection) {
  EXPECT_EQ(
      R"(module "\00\00\00")",
      format("{}", NameSubsection{NameSubsectionId::ModuleName, "\0\0\0"_su8}));

  EXPECT_EQ(R"(functions "\00\00\00")",
            format("{}", NameSubsection{NameSubsectionId::FunctionNames,
                                        "\0\0\0"_su8}));

  EXPECT_EQ(
      R"(locals "\00\00\00")",
      format("{}", NameSubsection{NameSubsectionId::LocalNames, "\0\0\0"_su8}));

  EXPECT_EQ(
      R"( locals "")",
      format("{:>10s}", NameSubsection{NameSubsectionId::LocalNames, ""_su8}));
}

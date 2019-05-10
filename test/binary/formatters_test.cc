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

#include "test/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(FormattersTest, ValueType) {
  EXPECT_EQ(R"(i32)", format("{}", ValueType::I32));
  EXPECT_EQ(R"(  f32)", format("{:>5s}", ValueType::F32));
}

TEST(FormattersTest, BlockType) {
  EXPECT_EQ(R"([i32])", format("{}", BlockType::I32));
  EXPECT_EQ(R"([])", format("{}", BlockType::Void));
  EXPECT_EQ(R"(100)", format("{}", BlockType(100)));
  EXPECT_EQ(R"(   [f64])", format("{:>8s}", BlockType::F64));
}

TEST(FormattersTest, ElementType) {
  EXPECT_EQ(R"(funcref)", format("{}", ElementType::Funcref));
  EXPECT_EQ(R"(   funcref)", format("{:>10s}", ElementType::Funcref));
}

TEST(FormattersTest, ExternalKind) {
  EXPECT_EQ(R"(func)", format("{}", ExternalKind::Function));
  EXPECT_EQ(R"(  global)", format("{:>8s}", ExternalKind::Global));
}

TEST(FormattersTest, Mutability) {
  EXPECT_EQ(R"(const)", format("{}", Mutability::Const));
  EXPECT_EQ(R"(   var)", format("{:>6s}", Mutability::Var));
}

TEST(FormattersTest, SegmentType) {
  EXPECT_EQ(R"(active)", format("{}", SegmentType::Active));
  EXPECT_EQ(R"(passive)", format("{}", SegmentType::Passive));
  EXPECT_EQ(R"(  active)", format("{:>8s}", SegmentType::Active));
  EXPECT_EQ(R"(  passive)", format("{:>9s}", SegmentType::Passive));
}

TEST(FormattersTest, Shared) {
  EXPECT_EQ(R"(unshared)", format("{}", Shared::No));
  EXPECT_EQ(R"(shared)", format("{}", Shared::Yes));
  EXPECT_EQ(R"(  unshared)", format("{:>10s}", Shared::No));
  EXPECT_EQ(R"(  shared)", format("{:>8s}", Shared::Yes));
}

TEST(FormattersTest, NameSubsectionKind) {
  EXPECT_EQ(R"(locals)", format("{}", NameSubsectionId::LocalNames));
  EXPECT_EQ(R"(  module)", format("{:>8s}", NameSubsectionId::ModuleName));
}

TEST(FormattersTest, MemArgImmediate) {
  EXPECT_EQ(R"({align 1, offset 2})", format("{}", MemArgImmediate{1, 2}));
  EXPECT_EQ(R"({align 0, offset 0} )", format("{:20s}", MemArgImmediate{0, 0}));
}

TEST(FormattersTest, Limits) {
  EXPECT_EQ(R"({min 1})", format("{}", Limits{1}));
  EXPECT_EQ(R"({min 1, max 2})", format("{}", Limits{1, 2}));
  EXPECT_EQ(R"({min 1, max 2, shared})",
            format("{}", Limits{1, 2, Shared::Yes}));
  EXPECT_EQ(R"(  {min 0})", format("{:>9s}", Limits{0}));
}

TEST(FormattersTest, Locals) {
  EXPECT_EQ(R"(i32 ** 3)", format("{}", Locals{3, ValueType::I32}));
  EXPECT_EQ(R"(  i32 ** 3)", format("{:>10s}", Locals{3, ValueType::I32}));
}

TEST(FormattersTest, KnownSection) {
  EXPECT_EQ(
      R"({id type, contents "\00\01\02"})",
      format("{}", KnownSection{SectionId::Type, "\x00\x01\x02"_su8}));

  EXPECT_EQ(
      R"(   {id code, contents ""})",
      format("{:>25s}", KnownSection{SectionId::Code, ""_su8}));
}

TEST(FormattersTest, CustomSection) {
  EXPECT_EQ(
      R"({name "custom", contents "\00\01\02"})",
      format("{}", CustomSection{"custom", "\x00\x01\x02"_su8}));

  EXPECT_EQ(
      R"(   {name "", contents ""})",
      format("{:>25s}", CustomSection{"", ""_su8}));
}

TEST(FormattersTest, Section) {
  auto span = "\x00\x01\x02"_su8;
  EXPECT_EQ(
      R"({id type, contents "\00\01\02"})",
      format("{}", Section{KnownSection{SectionId::Type, span}}));

  EXPECT_EQ(
      R"({name "custom", contents "\00\01\02"})",
      format("{}", Section{CustomSection{"custom", span}}));

  EXPECT_EQ(
      R"({id 100, contents "\00\01\02"})",
      format("{}", Section{KnownSection{static_cast<SectionId>(100), span}}));

  EXPECT_EQ(
      R"(   {id data, contents ""})",
      format("{:>25s}", Section{KnownSection{SectionId::Data, ""_su8}}));
}

TEST(FormattersTest, TypeEntry) {
  EXPECT_EQ(R"([] -> [])", format("{}", TypeEntry{FunctionType{{}, {}}}));
  EXPECT_EQ(R"([i32] -> [])",
            format("{}", TypeEntry{FunctionType{{ValueType::I32}, {}}}));
  EXPECT_EQ(R"(  [] -> [])",
            format("{:>10s}", TypeEntry{FunctionType{{}, {}}}));
}

TEST(FormattersTest, FunctionType) {
  EXPECT_EQ(R"([] -> [])", format("{}", FunctionType{{}, {}}));
  EXPECT_EQ(R"([i32] -> [])", format("{}", FunctionType{{ValueType::I32}, {}}));
  EXPECT_EQ(R"([i32 f32] -> [i64 f64])",
            format("{}", FunctionType{{ValueType::I32, ValueType::F32},
                                      {ValueType::I64, ValueType::F64}}));
  EXPECT_EQ(R"(  [] -> [])", format("{:>10s}", FunctionType{{}, {}}));
}

TEST(FormattersTest, TableType) {
  EXPECT_EQ(R"({min 1, max 2} funcref)",
            format("{}", TableType{Limits{1, 2}, ElementType::Funcref}));
  EXPECT_EQ(R"(  {min 0} funcref)",
            format("{:>17s}", TableType{Limits{0}, ElementType::Funcref}));
}

TEST(FormattersTest, MemoryType) {
  EXPECT_EQ(R"({min 1, max 2})", format("{}", MemoryType{Limits{1, 2}}));
  EXPECT_EQ(R"(   {min 0})", format("{:>10s}", MemoryType{Limits{0}}));
}

TEST(FormattersTest, GlobalType) {
  EXPECT_EQ(R"(const f32)",
            format("{}", GlobalType{ValueType::F32, Mutability::Const}));
  EXPECT_EQ(R"(var i32)",
            format("{}", GlobalType{ValueType::I32, Mutability::Var}));
  EXPECT_EQ(R"(   var f64)",
            format("{:>10s}", GlobalType{ValueType::F64, Mutability::Var}));
}

TEST(FormattersTest, Import) {
  // Function
  EXPECT_EQ(R"({module "a", name "b", desc func 3})",
            format("{}", Import{"a", "b", Index{3}}));

  // Table
  EXPECT_EQ(
      R"({module "c", name "d", desc table {min 1} funcref})",
      format("{}",
             Import{"c", "d", TableType{Limits{1}, ElementType::Funcref}}));

  // Memory
  EXPECT_EQ(
      R"({module "e", name "f", desc mem {min 0, max 4}})",
      format("{}", Import{"e", "f", MemoryType{Limits{0, 4}}}));

  // Global
  EXPECT_EQ(
      R"({module "g", name "h", desc global var i32})",
      format("{}",
             Import{"g", "h", GlobalType{ValueType::I32, Mutability::Var}}));

  EXPECT_EQ(R"(  {module "", name "", desc func 0})",
            format("{:>35s}", Import{"", "", Index{0}}));
}

TEST(FormattersTest, Export) {
  EXPECT_EQ(R"({name "f", desc func 0})",
            format("{}", Export{ExternalKind::Function, "f", Index{0}}));
  EXPECT_EQ(R"({name "t", desc table 1})",
            format("{}", Export{ExternalKind::Table, "t", Index{1}}));
  EXPECT_EQ(R"({name "m", desc mem 2})",
            format("{}", Export{ExternalKind::Memory, "m", Index{2}}));
  EXPECT_EQ(R"({name "g", desc global 3})",
            format("{}", Export{ExternalKind::Global, "g", Index{3}}));
  EXPECT_EQ(R"(    {name "", desc mem 0})",
            format("{:>25s}", Export{ExternalKind::Memory, "", Index{0}}));
}

TEST(FormattersTest, Expression) {
  EXPECT_EQ(R"("\00\01\02")", format("{}", Expression{"\00\01\02"_su8}));
  EXPECT_EQ(R"(   "\00")", format("{:>8s}", Expression{"\00"_su8}));
}

TEST(FormattersTest, ConstantExpression) {
  EXPECT_EQ(R"(i32.add end)",
            format("{}", ConstantExpression{Instruction{Opcode::I32Add}}));
  EXPECT_EQ(R"(   nop end)",
            format("{:>10s}", ConstantExpression{Instruction{Opcode::Nop}}));
}

TEST(FormattersTest, ElementExpression) {
  EXPECT_EQ(R"(ref.null end)",
            format("{}", ElementExpression{Instruction{Opcode::RefNull}}));
  EXPECT_EQ(R"(   ref.func 0 end)",
            format("{:>17s}",
                   ElementExpression{Instruction{Opcode::RefFunc, Index{0}}}));
}

TEST(FormattersTest, Opcode) {
  EXPECT_EQ(R"(memory.grow)", format("{}", Opcode::MemoryGrow));
  EXPECT_EQ(R"(   nop)", format("{:>6s}", Opcode::Nop));
}

TEST(FormattersTest, CallIndirectImmediate) {
  EXPECT_EQ(R"(1 0)", format("{}", CallIndirectImmediate{1u, 0}));
  EXPECT_EQ(R"(  10 0)", format("{:>6s}", CallIndirectImmediate{10u, 0}));
}

TEST(FormattersTest, BrTableImmediate) {
  EXPECT_EQ(R"([] 100)", format("{}", BrTableImmediate{{}, 100}));
  EXPECT_EQ(R"([1 2] 3)", format("{}", BrTableImmediate{{1, 2}, 3}));
  EXPECT_EQ(R"(  [42] 0)", format("{:>8s}", BrTableImmediate{{42}, 0}));
}

TEST(FormattersTest, BrOnExnImmediate) {
  EXPECT_EQ(R"(0 100)", format("{}", BrOnExnImmediate{0, 100}));
  EXPECT_EQ(R"(  42 0)", format("{:>6s}", BrOnExnImmediate{42, 0}));
}

TEST(FormattersTest, InitImmediate) {
  EXPECT_EQ(R"(1 0)", format("{}", InitImmediate{1u, 0}));
  EXPECT_EQ(R"(  10 0)", format("{:>6s}", InitImmediate{10u, 0}));
}

TEST(FormattersTest, CopyImmediate) {
  EXPECT_EQ(R"(0 0)", format("{}", CopyImmediate{0, 0}));
  EXPECT_EQ(R"(   0 0)", format("{:>6s}", CopyImmediate{0, 0}));
}

TEST(FormattersTest, ShuffleImmediate) {
  EXPECT_EQ(R"(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)",
            format("{}", ShuffleImmediate{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                           12, 13, 14, 15}}));
  EXPECT_EQ(R"(  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)",
            format("{:>33s}", ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0}}));
}

TEST(FormattersTest, Instruction) {
  // nop
  EXPECT_EQ(R"(nop)", format("{}", Instruction{Opcode::Nop}));
  // block (result i32)
  EXPECT_EQ(R"(block [i32])",
            format("{}", Instruction{Opcode::Block, BlockType::I32}));
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
  // memory.size (w/ a reserved value of 0)
  EXPECT_EQ(R"(memory.size 0)",
            format("{}", Instruction{Opcode::MemorySize, u8{0}}));
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
  // memory.init 0 10
  EXPECT_EQ(
      R"(memory.init 0 10)",
      format("{}", Instruction{Opcode::MemoryInit, InitImmediate{0, 10}}));

  EXPECT_EQ(R"(   i32.add)", format("{:>10s}", Instruction{Opcode::I32Add}));
}

TEST(FormattersTest, Function) {
  EXPECT_EQ(R"({type 1})", format("{}", Function{Index{1u}}));
  EXPECT_EQ(R"(  {type 1})", format("{:>10s}", Function{Index{1u}}));
}

TEST(FormattersTest, Table) {
  EXPECT_EQ(R"({type {min 1} funcref})",
            format("{}", Table{TableType{Limits{1}, ElementType::Funcref}}));
  EXPECT_EQ(
      R"(   {type {min 1} funcref})",
      format("{:>25s}", Table{TableType{Limits{1}, ElementType::Funcref}}));
}

TEST(FormattersTest, Memory) {
  EXPECT_EQ(R"({type {min 2, max 3}})",
            format("{}", Memory{MemoryType{Limits{2, 3}}}));
  EXPECT_EQ(R"( {type {min 0}})",
            format("{:>15s}", Memory{MemoryType{Limits{0}}}));
}

TEST(FormattersTest, Global) {
  EXPECT_EQ(
      R"({type const i32, init i32.const 0 end})",
      format("{}",
             Global{GlobalType{ValueType::I32, Mutability::Const},
                    ConstantExpression{Instruction{Opcode::I32Const, 0U}}}));
  EXPECT_EQ(
      R"(  {type var f32, init nop end})",
      format("{:>30s}", Global{GlobalType{ValueType::F32, Mutability::Var},
                               ConstantExpression{Instruction{Opcode::Nop}}}));
}

TEST(FormattersTest, Start) {
  EXPECT_EQ(R"({func 1})", format("{}", Start{Index{1u}}));
  EXPECT_EQ(R"(  {func 1})", format("{:>10s}", Start{Index{1u}}));
}

TEST(FormattersTest, ElementSegment_Active) {
  EXPECT_EQ(
      R"({table 1, offset nop end, init [2 3]})",
      format("{}", ElementSegment{Index{1u},
                                  ConstantExpression{Instruction{Opcode::Nop}},
                                  {2u, 3u}}));

  EXPECT_EQ(
      R"( {table 0, offset nop end, init []})",
      format("{:>35s}",
             ElementSegment{
                 Index{0u}, ConstantExpression{Instruction{Opcode::Nop}}, {}}));
}

TEST(FormattersTest, ElementSegment_Passive) {
  EXPECT_EQ(
      R"({element_type funcref, init [ref.func 2 end ref.null end]})",
      format("{}",
             ElementSegment{
                 ElementType::Funcref,
                 {ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}},
                  ElementExpression{Instruction{Opcode::RefNull}}}}));

  EXPECT_EQ(
      R"( {element_type funcref, init []})",
      format("{:>32s}", ElementSegment{ElementType::Funcref, {}}));
}

TEST(FormattersTest, Code) {
  EXPECT_EQ(
      R"({locals [i32 ** 1], body "\0b"})",
      format("{}", Code{{Locals{1, ValueType::I32}}, Expression{"\x0b"_su8}}));

  EXPECT_EQ(
      R"(     {locals [], body ""})",
      format("{:>25s}", Code{{}, Expression{""_su8}}));
}

TEST(FormattersTest, DataSegment_Active) {
  EXPECT_EQ(
      R"({memory 0, offset i32.const 0 end, init "\12\34"})",
      format("{}",
             DataSegment{Index{0u},
                         ConstantExpression{Instruction{Opcode::I32Const, 0u}},
                         "\x12\x34"_su8}));

  EXPECT_EQ(
      R"(  {memory 0, offset nop end, init ""})",
      format(
          "{:>37s}",
          DataSegment{Index{0u}, ConstantExpression{Instruction{Opcode::Nop}},
                      ""_su8}));
}

TEST(FormattersTest, DataSegment_Passive) {
  EXPECT_EQ(
      R"({init "\12\34"})", format("{}", DataSegment{"\x12\x34"_su8}));

  EXPECT_EQ(
      R"(  {init ""})", format("{:>11}", DataSegment{""_su8}));
}

TEST(FormattersTest, DataCount) {
  EXPECT_EQ(R"({count 1})", format("{}", DataCount{1u}));
  EXPECT_EQ(R"( {count 1})", format("{:>10s}", DataCount{1u}));
}

TEST(FormattersTest, NameAssoc) {
  EXPECT_EQ(R"(3 "hi")", format("{}", NameAssoc{3u, "hi"}));
  EXPECT_EQ(R"(  0 "")", format("{:>6s}", NameAssoc{0u, ""}));
}

TEST(FormattersTest, IndirectNameAssoc) {
  EXPECT_EQ(
      R"(0 [1 "first" 2 "second"])",
      format("{}", IndirectNameAssoc{0u, {{1u, "first"}, {2u, "second"}}}));
  EXPECT_EQ(
      R"(  1 [10 "a" 100 "b"])",
      format("{:>20s}", IndirectNameAssoc{1u, {{10u, "a"}, {100u, "b"}}}));
}

TEST(FormattersTest, NameSubsection) {
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

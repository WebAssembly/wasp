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

#include "src/binary/formatters.h"

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::binary;

namespace {

template <typename T>
std::string TestFormat(const T& t) {
  return format("{}", t);
}

}  // namespace

TEST(FormatTest, ValueType) {
  EXPECT_EQ(R"(i32)", TestFormat(ValueType::I32));
}

TEST(FormatTest, BlockType) {
  EXPECT_EQ(R"([i32])", TestFormat(BlockType::I32));
  EXPECT_EQ(R"([])", TestFormat(BlockType::Void));
}

TEST(FormatTest, ElementType) {
  EXPECT_EQ(R"(funcref)", TestFormat(ElementType::Funcref));
}

TEST(FormatTest, ExternalKind) {
  EXPECT_EQ(R"(func)", TestFormat(ExternalKind::Function));
}

TEST(FormatTest, Mutability) {
  EXPECT_EQ(R"(const)", TestFormat(Mutability::Const));
}

TEST(FormatTest, MemArg) {
  EXPECT_EQ(R"({align 1, offset 2})", TestFormat(MemArg{1, 2}));
}

TEST(FormatTest, Limits) {
  EXPECT_EQ(R"({min 1})", TestFormat(Limits{1}));
  EXPECT_EQ(R"({min 1, max 2})", TestFormat(Limits{1, 2}));
}

TEST(FormatTest, Locals) {
  EXPECT_EQ(R"(i32 ** 3)", TestFormat(Locals{3, ValueType::I32}));
}

TEST(FormatTest, KnownSection) {
  const u8 data[] = "\x00\x01\x02";
  EXPECT_EQ(R"({id type, contents "\00\01\02"})",
            TestFormat(KnownSection<>{SectionId::Type, SpanU8{data, 3}}));
}

TEST(FormatTest, CustomSection) {
  const u8 data[] = "\x00\x01\x02";
  EXPECT_EQ(
      R"({name "custom", contents "\00\01\02"})",
      TestFormat(CustomSection<>{"custom", SpanU8{data, 3}}));
}

TEST(FormatTest, Section) {
  const u8 data[] = "\x00\x01\x02";
  EXPECT_EQ(
      R"({id type, contents "\00\01\02"})",
      TestFormat(Section<>{KnownSection<>{SectionId::Type, SpanU8{data, 3}}}));

  EXPECT_EQ(
      R"({name "custom", contents "\00\01\02"})",
      TestFormat(Section<>{CustomSection<>{"custom", SpanU8{data, 3}}}));

  EXPECT_EQ(
      R"({id 100, contents "\00"})",
      TestFormat(Section<>{
          KnownSection<>{static_cast<SectionId>(100), SpanU8{data, 1}}}));
}

TEST(FormatTest, TypeEntry) {
  EXPECT_EQ(R"([] -> [])", TestFormat(TypeEntry{FunctionType{{}, {}}}));
  EXPECT_EQ(R"([i32] -> [])",
            TestFormat(TypeEntry{FunctionType{{ValueType::I32}, {}}}));
}

TEST(FormatTest, FunctionType) {
  EXPECT_EQ(R"([] -> [])", TestFormat(FunctionType{{}, {}}));
  EXPECT_EQ(R"([i32] -> [])", TestFormat(FunctionType{{ValueType::I32}, {}}));
  EXPECT_EQ(R"([i32 f32] -> [i64 f64])",
            TestFormat(FunctionType{{ValueType::I32, ValueType::F32},
                                    {ValueType::I64, ValueType::F64}}));
}

TEST(FormatTest, TableType) {
  EXPECT_EQ(R"({min 1, max 2} funcref)",
            TestFormat(TableType{Limits{1, 2}, ElementType::Funcref}));
}

TEST(FormatTest, MemoryType) {
  EXPECT_EQ(R"({min 1, max 2})", TestFormat(MemoryType{Limits{1, 2}}));
}

TEST(FormatTest, GlobalType) {
  EXPECT_EQ(R"(const f32)",
            TestFormat(GlobalType{ValueType::F32, Mutability::Const}));
  EXPECT_EQ(R"(var i32)",
            TestFormat(GlobalType{ValueType::I32, Mutability::Var}));
}

TEST(FormatTest, Import) {
  // Function
  EXPECT_EQ(R"({module "a", name "b", desc func 3})",
            TestFormat(Import<>{"a", "b", Index{3}}));

  // Table
  EXPECT_EQ(
      R"({module "c", name "d", desc table {min 1} funcref})",
      TestFormat(
          Import<>{"c", "d", TableType{Limits{1}, ElementType::Funcref}}));

  // Memory
  EXPECT_EQ(
      R"({module "e", name "f", desc mem {min 0, max 4}})",
      TestFormat(Import<>{"e", "f", MemoryType{Limits{0, 4}}}));

  // Global
  EXPECT_EQ(
      R"({module "g", name "h", desc global var i32})",
      TestFormat(
          Import<>{"g", "h", GlobalType{ValueType::I32, Mutability::Var}}));
}

TEST(FormatTest, Export) {
  EXPECT_EQ(R"({name "f", desc func 0})",
            TestFormat(Export<>{ExternalKind::Function, "f", Index{0}}));
  EXPECT_EQ(R"({name "t", desc table 1})",
            TestFormat(Export<>{ExternalKind::Table, "t", Index{1}}));
  EXPECT_EQ(R"({name "m", desc mem 2})",
            TestFormat(Export<>{ExternalKind::Memory, "m", Index{2}}));
  EXPECT_EQ(R"({name "g", desc global 3})",
            TestFormat(Export<>{ExternalKind::Global, "g", Index{3}}));
}

TEST(FormatTest, Expression) {
  const u8 data[] = "\00\01\02";
  EXPECT_EQ(R"("\00\01\02")", TestFormat(Expression<>{SpanU8{data, 3}}));
}

TEST(FormatTest, ConstantExpression) {
  const u8 data[] = "\00\01\02";
  EXPECT_EQ(R"("\00\01\02")",
            TestFormat(ConstantExpression<>{SpanU8{data, 3}}));
}

TEST(FormatTest, Opcode) {
  EXPECT_EQ(R"(memory.grow)", TestFormat(Opcode::MemoryGrow));
}

TEST(FormatTest, CallIndirectImmediate) {
  EXPECT_EQ(R"(1 0)", TestFormat(CallIndirectImmediate{1u, 0}));
}

TEST(FormatTest, BrTableImmediate) {
  EXPECT_EQ(R"([] 100)", TestFormat(BrTableImmediate{{}, 100}));
  EXPECT_EQ(R"([1 2] 3)", TestFormat(BrTableImmediate{{1, 2}, 3}));
}

TEST(FormatTest, Instruction) {
  // nop
  EXPECT_EQ(R"(nop)", TestFormat(Instruction{Opcode::Nop}));
  // block (result i32)
  EXPECT_EQ(R"(block [i32])",
            TestFormat(Instruction{Opcode::Block, BlockType::I32}));
  // br 3
  EXPECT_EQ(R"(br 3)", TestFormat(Instruction{Opcode::Br, Index{3u}}));
  // br_table 0 1 4
  EXPECT_EQ(
      R"(br_table [0 1] 4)",
      TestFormat(Instruction{Opcode::BrTable, BrTableImmediate{{0, 1}, 4}}));
  // call_indirect 1 (w/ a reserved value of 0)
  EXPECT_EQ(
      R"(call_indirect 1 0)",
      TestFormat(
          Instruction{Opcode::CallIndirect, CallIndirectImmediate{1, 0}}));
  // memory.size (w/ a reserved value of 0)
  EXPECT_EQ(R"(memory.size 0)",
            TestFormat(Instruction{Opcode::MemorySize, u8{0}}));
  // i32.load offset=10 align=4 (alignment is stored as power-of-two)
  EXPECT_EQ(R"(i32.load {align 2, offset 10})",
            TestFormat(Instruction{Opcode::I32Load, MemArg{2, 10}}));
  // i32.const 100
  EXPECT_EQ(R"(i32.const 100)",
            TestFormat(Instruction{Opcode::I32Const, s32{100}}));
  // i64.const 1000
  EXPECT_EQ(R"(i64.const 1000)",
            TestFormat(Instruction{Opcode::I64Const, s64{1000}}));
  // f32.const 1.5
  EXPECT_EQ(R"(f32.const 1.500000)",
            TestFormat(Instruction{Opcode::F32Const, f32{1.5}}));
  // f64.const 6.25
  EXPECT_EQ(R"(f64.const 6.250000)",
            TestFormat(Instruction{Opcode::F64Const, f64{6.25}}));
}

TEST(FormatTest, Function) {
  EXPECT_EQ(R"({type 1})", TestFormat(Function{Index{1u}}));
}

TEST(FormatTest, Table) {
  EXPECT_EQ(R"({type {min 1} funcref})",
            TestFormat(Table{TableType{Limits{1}, ElementType::Funcref}}));
}

TEST(FormatTest, Memory) {
  EXPECT_EQ(R"({type {min 2, max 3}})",
            TestFormat(Memory{MemoryType{Limits{2, 3}}}));
}

TEST(FormatTest, Global) {
  const u8 expr[] = "\xfa\xce";
  EXPECT_EQ(R"({type const i32, init "\fa\ce"})",
            TestFormat(Global<>{GlobalType{ValueType::I32, Mutability::Const},
                                ConstantExpression<>{SpanU8{expr, 2}}}));
}

TEST(FormatTest, Start) {
  EXPECT_EQ(R"({func 1})", TestFormat(Start{Index{1u}}));
}

TEST(FormatTest, ElementSegment) {
  const u8 expr[] = "\x0b";
  EXPECT_EQ(
      R"({table 1, offset "\0b", init [2 3]})",
      TestFormat(ElementSegment<>{
          Index{1u}, ConstantExpression<>{SpanU8{expr, 1}}, {2u, 3u}}));
}

TEST(FormatTest, Code) {
  const u8 expr[] = "\x0b";
  EXPECT_EQ(
      R"({locals [i32 ** 1], body "\0b"})",
      TestFormat(
          Code<>{{Locals{1, ValueType::I32}}, Expression<>{SpanU8{expr, 1}}}));
}

TEST(FormatTest, DataSegment) {
  const u8 expr[] = "\x0b";
  const u8 init[] = "\x12\x34";
  EXPECT_EQ(
      R"({memory 0, offset "\0b", init "\12\34"})",
      TestFormat(DataSegment<>{Index{0u}, ConstantExpression<>{SpanU8{expr, 1}},
                               SpanU8{init, 2}}));
}

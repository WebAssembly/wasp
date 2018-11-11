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

#include "src/binary/to_string.h"

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::binary;

TEST(ToStringTest, ValType) {
  EXPECT_EQ(R"(i32)", ToString(ValType::I32));
}

TEST(ToStringTest, ExternalKind) {
  EXPECT_EQ(R"(func)", ToString(ValType::Func));
}

TEST(ToStringTest, Mutability) {
  EXPECT_EQ(R"(const)", ToString(Mutability::Const));
}

TEST(ToStringTest, MemArg) {
  EXPECT_EQ(R"({align 1, offset 2})", ToString(MemArg{1, 2}));
}

TEST(ToStringTest, Limits) {
  EXPECT_EQ(R"({min 1})", ToString(Limits{1}));
  EXPECT_EQ(R"({min 1, max 2})", ToString(Limits{1, 2}));
}

TEST(ToStringTest, LocalDecl) {
  EXPECT_EQ(R"(i32 ** 3)", ToString(LocalDecl{3, ValType::I32}));
}

TEST(ToStringTest, Section) {
  const u8 data[] = "\x00\x01\x02";
  EXPECT_EQ(R"({id 1, contents "\00\01\02"})",
            ToString(Section{1, SpanU8{data, 3}}));
}

TEST(ToStringTest, CustomSection) {
  const u8 data[] = "\x00\x01\x02";
  EXPECT_EQ(
      R"({after_id <none>, name "custom", contents "\00\01\02"})",
      ToString(CustomSection<>{absl::nullopt, "custom", SpanU8{data, 3}}));

  EXPECT_EQ(
      R"({after_id 10, name "foo", contents "\00"})",
      ToString(CustomSection<>{10u, "foo", SpanU8{data, 1}}));
}

TEST(ToStringTest, FuncType) {
  EXPECT_EQ(R"([] -> [])", ToString(FuncType{{}, {}}));
  EXPECT_EQ(R"([i32] -> [])", ToString(FuncType{{ValType::I32}, {}}));
  EXPECT_EQ(R"([i32 f32] -> [i64 f64])",
            ToString(FuncType{{ValType::I32, ValType::F32},
                              {ValType::I64, ValType::F64}}));
}

TEST(ToStringTest, TableType) {
  EXPECT_EQ(R"({min 1, max 2} anyfunc)",
            ToString(TableType{Limits{1, 2}, ValType::Anyfunc}));
}

TEST(ToStringTest, MemoryType) {
  EXPECT_EQ(R"({min 1, max 2})", ToString(MemoryType{Limits{1, 2}}));
}

TEST(ToStringTest, GlobalType) {
  EXPECT_EQ(R"(const f32)",
            ToString(GlobalType{ValType::F32, Mutability::Const}));
  EXPECT_EQ(R"(var i32)", ToString(GlobalType{ValType::I32, Mutability::Var}));
}

TEST(ToStringTest, Import) {
  // Func
  EXPECT_EQ(R"({module "a", name "b", desc func 3})",
            ToString(Import<>{"a", "b", Index{3}}));

  // Table
  EXPECT_EQ(
      R"({module "c", name "d", desc table {min 1} anyfunc})",
      ToString(Import<>{"c", "d", TableType{Limits{1}, ValType::Anyfunc}}));

  // Memory
  EXPECT_EQ(
      R"({module "e", name "f", desc memory {min 0, max 4}})",
      ToString(Import<>{"e", "f", MemoryType{Limits{0, 4}}}));

  // Global
  EXPECT_EQ(
      R"({module "g", name "h", desc global var i32})",
      ToString(Import<>{"g", "h", GlobalType{ValType::I32, Mutability::Var}}));
}

TEST(ToStringTest, Export) {
  EXPECT_EQ(R"({name "f", desc func 0})",
            ToString(Export<>{ExternalKind::Func, "f", Index{0}}));
  EXPECT_EQ(R"({name "t", desc table 1})",
            ToString(Export<>{ExternalKind::Table, "t", Index{1}}));
  EXPECT_EQ(R"({name "m", desc memory 2})",
            ToString(Export<>{ExternalKind::Memory, "m", Index{2}}));
  EXPECT_EQ(R"({name "g", desc global 3})",
            ToString(Export<>{ExternalKind::Global, "g", Index{3}}));
}

TEST(ToStringTest, Expr) {
  const u8 data[] = "\00\01\02";
  EXPECT_EQ(R"("\00\01\02")", ToString(Expr<>{SpanU8{data, 3}}));
}

TEST(ToStringTest, Opcode) {
  EXPECT_EQ(R"(40)", ToString(Opcode{0x40}));
  EXPECT_EQ(R"(fe 00000003)", ToString(Opcode{0xfe, 0x03}));
}

TEST(ToStringTest, CallIndirectImmediate) {
  EXPECT_EQ(R"(1 0)", ToString(CallIndirectImmediate{1u, 0}));
}

TEST(ToStringTest, BrTableImmediate) {
  EXPECT_EQ(R"([] 100)", ToString(BrTableImmediate{{}, 100}));
  EXPECT_EQ(R"([1 2] 3)", ToString(BrTableImmediate{{1, 2}, 3}));
}

TEST(ToStringTest, Instr) {
  // nop
  EXPECT_EQ(R"(01)", ToString(Instr{Opcode{0x01}}));
  // block i32
  EXPECT_EQ(R"(02 i32)", ToString(Instr{Opcode{0x02}, ValType::I32}));
  // br 3
  EXPECT_EQ(R"(0c 3)", ToString(Instr{Opcode{0x0c}, Index{3u}}));
  // br_table 0 1 4
  EXPECT_EQ(R"(0e [0 1] 4)",
            ToString(Instr{Opcode{0x0e}, BrTableImmediate{{0, 1}, 4}}));
  // call_indirect 1 (w/ a reserved value of 0)
  EXPECT_EQ(R"(11 1 0)",
            ToString(Instr{Opcode{0x11}, CallIndirectImmediate{1, 0}}));
  // memory.size (w/ a reserved value of 0)
  EXPECT_EQ(R"(3f 0)", ToString(Instr{Opcode{0x3f}, u8{0}}));
  // i32.load offset=10 align=4 (alignment is stored as power-of-two)
  EXPECT_EQ(R"(28 {align 2, offset 10})",
            ToString(Instr{Opcode{0x28}, MemArg{2, 10}}));
  // i32.const 100
  EXPECT_EQ(R"(41 100)", ToString(Instr{Opcode{0x41}, s32{100}}));
  // i64.const 1000
  EXPECT_EQ(R"(42 1000)", ToString(Instr{Opcode{0x42}, s64{1000}}));
  // f32.const 1.5
  EXPECT_EQ(R"(43 1.500000)", ToString(Instr{Opcode{0x43}, f32{1.5}}));
  // f64.const 6.25
  EXPECT_EQ(R"(44 6.250000)", ToString(Instr{Opcode{0x44}, f64{6.25}}));
}

TEST(ToStringTest, Func) {
  EXPECT_EQ(R"({type 1})", ToString(Func{Index{1u}}));
}

TEST(ToStringTest, Table) {
  EXPECT_EQ(R"({type {min 1} anyfunc})",
            ToString(Table{TableType{Limits{1}, ValType::Anyfunc}}));
}

TEST(ToStringTest, Memory) {
  EXPECT_EQ(R"({type {min 2, max 3}})",
            ToString(Memory{MemoryType{Limits{2, 3}}}));
}

TEST(ToStringTest, Global) {
  const u8 expr[] = "\xfa\xce";
  EXPECT_EQ(R"({type const i32, init "\fa\ce"})",
            ToString(Global<>{GlobalType{ValType::I32, Mutability::Const},
                              Expr<>{SpanU8{expr, 2}}}));
}

TEST(ToStringTest, Start) {
  EXPECT_EQ(R"({func 1})", ToString(Start{Index{1u}}));
}

TEST(ToStringTest, ElementSegment) {
  const u8 expr[] = "\x0b";
  EXPECT_EQ(
      R"({table 1, offset "\0b", init [2 3]})",
      ToString(ElementSegment<>{Index{1u}, Expr<>{SpanU8{expr, 1}}, {2u, 3u}}));
}

TEST(ToStringTest, Code) {
  const u8 expr[] = "\x0b";
  EXPECT_EQ(
      R"({locals [i32 ** 1], body "\0b"})",
      ToString(Code<>{{LocalDecl{1, ValType::I32}}, Expr<>{SpanU8{expr, 1}}}));
}

TEST(ToStringTest, DataSegment) {
  const u8 expr[] = "\x0b";
  const u8 init[] = "\x12\x34";
  EXPECT_EQ(
      R"({memory 0, offset "\0b", init "\12\34"})",
      ToString(
          DataSegment<>{Index{0u}, Expr<>{SpanU8{expr, 1}}, SpanU8{init, 2}}));
}

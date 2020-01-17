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

#include "wasp/binary/lazy_section.h"

#include "gtest/gtest.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/sections.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

namespace {

template <typename T>
void ExpectSection(const std::vector<T>& expected, LazySection<T>& sec) {
  EXPECT_EQ(expected.size(), sec.count);
  size_t i = 0;
  for (const auto& item : sec.sequence) {
    ASSERT_LT(i, expected.size());
    EXPECT_EQ(expected[i++], item);
  }
  EXPECT_EQ(expected.size(), i);
}

}  // namespace

TEST(LazySectionTest, Type) {
  Features features;
  TestErrors errors;
  auto sec = ReadTypeSection(
      "\x02"                           // Count.
      "\x60\x00\x00"                   // (param) (result)
      "\x60\x02\x7f\x7f\x01\x7f"_su8,  // (param i32 i32) (result i32)
      features, errors);

  ExpectSection(
      {
          TypeEntry{FunctionType{{}, {}}},
          TypeEntry{
              FunctionType{{ValueType::I32, ValueType::I32}, {ValueType::I32}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Import) {
  Features features;
  TestErrors errors;
  auto sec = ReadImportSection(
      "\x02"                             // Count.
      "\x01w\x01x\x00\x02"               // (import "w" "x" (func 2))
      "\x01y\x01z\x02\x01\x01\x02"_su8,  // (import "y" "z" (memory 1 2))
      features, errors);

  ExpectSection(
      {
          Import{"w", "x", 2},
          Import{"y", "z", MemoryType{Limits{1, 2}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Function) {
  Features features;
  TestErrors errors;
  auto sec = ReadFunctionSection(
      "\x03"                   // Count.
      "\x02\x80\x01\x02"_su8,  // 2, 128, 2
      features, errors);

  ExpectSection({Function{2}, Function{128}, Function{2}}, sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Table) {
  Features features;
  TestErrors errors;
  auto sec = ReadTableSection(
      "\x03"                  // Count.
      "\x70\x00\x01"          // (table 1 funcref)
      "\x70\x01\x00\x80\x01"  // (table 1 128 funcref)
      "\x70\x00\x00"_su8,     // (table 0 funcref)
      features, errors);

  ExpectSection(
      {
          Table{TableType{Limits{1}, ElementType::Funcref}},
          Table{TableType{Limits{0, 128}, ElementType::Funcref}},
          Table{TableType{Limits{0}, ElementType::Funcref}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Memory) {
  Features features;
  TestErrors errors;
  auto sec = ReadMemorySection(
      "\x03"              // Count.
      "\x00\x01"          // (memory 1)
      "\x01\x00\x80\x01"  // (memory 1 128)
      "\x00\x00"_su8,     // (memory 0)
      features, errors);

  ExpectSection(
      {
          Memory{MemoryType{Limits{1}}},
          Memory{MemoryType{Limits{0, 128}}},
          Memory{MemoryType{Limits{0}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Global) {
  Features features;
  TestErrors errors;
  auto sec = ReadGlobalSection(
      "\x02"                       // Count.
      "\x7f\x01\x41\x00\x0b"       // (global (mut i32) (i32.const 0))
      "\x7e\x00\x42\x01\x0b"_su8,  // (global i64 (i64.const 1))
      features, errors);

  ExpectSection(
      {
          Global{GlobalType{ValueType::I32, Mutability::Var},
                 ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}},
          Global{GlobalType{ValueType::I64, Mutability::Const},
                 ConstantExpression{Instruction{Opcode::I64Const, s64{1}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Export) {
  Features features;
  TestErrors errors;
  auto sec = ReadExportSection(
      "\x03"                    // Count.
      "\x03one\x00\x01"         // (export "one" (func 1))
      "\x03two\x02\x02"         // (export "two" (memory 2))
      "\x05three\x03\x02"_su8,  // (export "three" (global 2))
      features, errors);

  ExpectSection(
      {
          Export{ExternalKind::Function, "one", 1},
          Export{ExternalKind::Memory, "two", 2},
          Export{ExternalKind::Global, "three", 2},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Start) {
  Features features;
  TestErrors errors;
  auto sec = ReadStartSection("\x03"_su8, features, errors);

  EXPECT_EQ(Start{3}, sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Element) {
  Features features;
  TestErrors errors;
  auto sec = ReadElementSection(
      "\x02"                           // Count.
      "\x00\x41\x00\x0b\x02\x00\x01"   // (elem (offset i32.const 0) 0 1)
      "\x00\x41\x02\x0b\x01\x03"_su8,  // (elem (offset i32.const 2) 3)
      features, errors);

  ExpectSection(
      {
          ElementSegment{
              0,
              ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
              {0, 1}},
          ElementSegment{
              0,
              ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
              {3}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Code) {
  Features features;
  TestErrors errors;
  auto sec = ReadCodeSection(
      "\x02"                           // Count.
      "\x02\x00\x0b"                   // (func)
      "\x05\x01\x01\x7f\x6a\x0b"_su8,  // (func (local i32) i32.add)
      features, errors);

  ExpectSection(
      {
          Code{{}, "\x0b"_expr},
          Code{{Locals{1, ValueType::I32}}, "\x6a\x0b"_expr},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Data) {
  Features features;
  TestErrors errors;
  auto sec = ReadDataSection(
      "\x03"                          // Count.
      "\x00\x41\x00\x0b\x02hi"        // (data (offset i32.const 0) "hi")
      "\x00\x41\x02\x0b\x03see"       // (data (offset i32.const 2) "see")
      "\x00\x41\x05\x0b\x03you"_su8,  // (data (offset i32.const 5) "you")
      features, errors);

  ExpectSection(
      {
          DataSegment{0,
                      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}},
                      "hi"_su8},
          DataSegment{0,
                      ConstantExpression{Instruction{Opcode::I32Const, s32{2}}},
                      "see"_su8},
          DataSegment{0,
                      ConstantExpression{Instruction{Opcode::I32Const, s32{5}}},
                      "you"_su8},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, DataCount) {
  Features features;
  TestErrors errors;
  auto sec = ReadDataCountSection("\x03"_su8, features, errors);

  EXPECT_EQ(DataCount{3}, sec);
  ExpectNoErrors(errors);
}

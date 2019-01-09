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

#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

namespace {

template <typename T, typename Errors>
void ExpectSection(const std::vector<T>& expected,
                   LazySection<T, Errors>& sec) {
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
      MakeSpanU8("\x02"                        // Count.
                 "\x60\x00\x00"                // (param) (result)
                 "\x60\x02\x7f\x7f\x01\x7f"),  // (param i32 i32) (result i32)
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
      MakeSpanU8(
          "\x02"                          // Count.
          "\x01w\x01x\x00\x02"            // (import "w" "x" (func 2))
          "\x01y\x01z\x02\x01\x01\x02"),  // (import "y" "z" (memory 1 2))
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
  auto sec = ReadFunctionSection(MakeSpanU8("\x03"                // Count.
                                            "\x02\x80\x01\x02"),  // 2, 128, 2
                                 features, errors);

  ExpectSection({Function{2}, Function{128}, Function{2}}, sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Table) {
  Features features;
  TestErrors errors;
  auto sec = ReadTableSection(
      MakeSpanU8("\x03"                  // Count.
                 "\x70\x00\x01"          // (table 1 funcref)
                 "\x70\x01\x00\x80\x01"  // (table 1 128 funcref)
                 "\x70\x00\x00"),        // (table 0 funcref)
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
  auto sec = ReadMemorySection(MakeSpanU8("\x03"              // Count.
                                          "\x00\x01"          // (memory 1)
                                          "\x01\x00\x80\x01"  // (memory 1 128)
                                          "\x00\x00"),        // (memory 0)
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
      MakeSpanU8("\x02"                    // Count.
                 "\x7f\x01\x41\x00\x0b"    // (global (mut i32) (i32.const 0))
                 "\x7e\x00\x42\x01\x0b"),  // (global i64 (i64.const 1))
      features, errors);

  ExpectSection(
      {
          Global{GlobalType{ValueType::I32, Mutability::Var},
                 MakeConstantExpression("\x41\x00\x0b")},
          Global{GlobalType{ValueType::I64, Mutability::Const},
                 MakeConstantExpression("\x42\x01\x0b")},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Export) {
  Features features;
  TestErrors errors;
  auto sec = ReadExportSection(
      MakeSpanU8("\x03"                 // Count.
                 "\x03one\x00\x01"      // (export "one" (func 1))
                 "\x03two\x02\x02"      // (export "two" (memory 2))
                 "\x05three\x03\x02"),  // (export "three" (global 2))
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
  auto sec = ReadStartSection(MakeSpanU8("\x03"), features, errors);

  EXPECT_EQ(Start{3}, sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Element) {
  Features features;
  TestErrors errors;
  auto sec = ReadElementSection(
      MakeSpanU8(
          "\x02"                          // Count.
          "\x00\x41\x00\x0b\x02\x00\x01"  // (elem (offset i32.const 0) 0 1)
          "\x00\x41\x02\x0b\x01\x03"),    // (elem (offset i32.const 2) 3)
      features, errors);

  ExpectSection(
      {
          ElementSegment{0, MakeConstantExpression("\x41\x00\x0b"), {0, 1}},
          ElementSegment{0, MakeConstantExpression("\x41\x02\x0b"), {3}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Code) {
  Features features;
  TestErrors errors;
  auto sec = ReadCodeSection(
      MakeSpanU8("\x02"                        // Count.
                 "\x02\x00\x0b"                // (func)
                 "\x05\x01\x01\x7f\x6a\x0b"),  // (func (local i32) i32.add)
      features, errors);

  ExpectSection(
      {
          Code{{}, MakeExpression("\x0b")},
          Code{{Locals{1, ValueType::I32}}, MakeExpression("\x6a\x0b")},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazySectionTest, Data) {
  Features features;
  TestErrors errors;
  auto sec = ReadDataSection(
      MakeSpanU8(
          "\x03"                       // Count.
          "\x00\x41\x00\x0b\x02hi"     // (data (offset i32.const 0) "hi")
          "\x00\x41\x02\x0b\x03see"    // (data (offset i32.const 2) "see")
          "\x00\x41\x05\x0b\x03you"),  // (data (offset i32.const 5) "you")
      features, errors);

  ExpectSection(
      {
          DataSegment{0, MakeConstantExpression("\x41\x00\x0b"),
                      MakeSpanU8("hi")},
          DataSegment{0, MakeConstantExpression("\x41\x02\x0b"),
                      MakeSpanU8("see")},
          DataSegment{0, MakeConstantExpression("\x41\x05\x0b"),
                      MakeSpanU8("you")},
      },
      sec);
  ExpectNoErrors(errors);
}

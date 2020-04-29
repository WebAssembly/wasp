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
#include "test/test_utils.h"
#include "wasp/binary/read/context.h"
#include "wasp/binary/sections.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;
using namespace ::wasp::test;

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

TEST(BinaryLazySectionTest, Type) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadTypeSection(
      "\x02"                           // Count.
      "\x60\x00\x00"                   // (param) (result)
      "\x60\x02\x7f\x7f\x01\x7f"_su8,  // (param i32 i32) (result i32)
      context);

  ExpectSection(
      {
          TypeEntry{MakeAt("\x00\x00"_su8, FunctionType{{}, {}})},
          TypeEntry{
              MakeAt("\x02\x7f\x7f\x01\x7f"_su8,
                     FunctionType{{MakeAt("\x07f"_su8, ValueType::I32),
                                   MakeAt("\x07f"_su8, ValueType::I32)},
                                  {MakeAt("\x07f"_su8, ValueType::I32)}})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Import) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadImportSection(
      "\x02"                             // Count.
      "\x01w\x01x\x00\x02"               // (import "w" "x" (func 2))
      "\x01y\x01z\x02\x01\x01\x02"_su8,  // (import "y" "z" (memory 1 2))
      context);

  ExpectSection(
      {
          Import{MakeAt("\x01w"_su8, "w"_sv), MakeAt("\x01x"_su8, "x"_sv),
                 MakeAt("\x02"_su8, Index{2})},
          Import{MakeAt("\x01y"_su8, "y"_sv), MakeAt("\x01z"_su8, "z"_sv),
                 MakeAt("\x01\x01\x02"_su8,
                        MemoryType{
                            MakeAt("\x01\x01\x02"_su8,
                                   Limits{MakeAt("\x01"_su8, u32{1}),
                                          MakeAt("\x02"_su8, u32{2}),
                                          MakeAt("\x01"_su8, Shared::No)})})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Function) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadFunctionSection(
      "\x03"                   // Count.
      "\x02\x80\x01\x02"_su8,  // 2, 128, 2
      context);

  ExpectSection(
      {
          Function{MakeAt("\x02"_su8, Index{2})},
          Function{MakeAt("\x80\x01"_su8, Index{128})},
          Function{MakeAt("\x02"_su8, Index{2})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Table) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadTableSection(
      "\x03"                  // Count.
      "\x70\x00\x01"          // (table 1 funcref)
      "\x70\x01\x00\x80\x01"  // (table 0 128 funcref)
      "\x70\x00\x00"_su8,     // (table 0 funcref)
      context);

  ExpectSection(
      {
          Table{MakeAt(
              "\x70\x00\x01"_su8,
              TableType{MakeAt("\x00\x01"_su8,
                               Limits{MakeAt("\x01"_su8, Index{1}), nullopt,
                                      MakeAt("\x00"_su8, Shared::No)}),
                        MakeAt("\x70"_su8, ElementType::Funcref)})},
          Table{MakeAt("\x70\x01\x00\x80\x01"_su8,
                       TableType{MakeAt("\x01\x00\x80\x01"_su8,
                                        Limits{MakeAt("\x00"_su8, u32{0}),
                                               MakeAt("\x80\x01"_su8, u32{128}),
                                               MakeAt("\x01"_su8, Shared::No)}),
                                 MakeAt("\x70"_su8, ElementType::Funcref)})},
          Table{MakeAt(
              "\x70\x00\x00"_su8,
              TableType{MakeAt("\x00\x00"_su8,
                               Limits{MakeAt("\x00"_su8, u32{0}), nullopt,
                                      MakeAt("\x00"_su8, Shared::No)}),
                        MakeAt("\x70"_su8, ElementType::Funcref)})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Memory) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadMemorySection(
      "\x03"              // Count.
      "\x00\x01"          // (memory 1)
      "\x01\x00\x80\x01"  // (memory 0 128)
      "\x00\x00"_su8,     // (memory 0)
      context);

  ExpectSection(
      {
          Memory{MakeAt(
              "\x00\x01"_su8,
              MemoryType{MakeAt("\x00\x01"_su8,
                                Limits{MakeAt("\x01"_su8, u32{1}), nullopt,
                                       MakeAt("\x00"_su8, Shared::No)})})},
          Memory{MakeAt(
              "\x01\x00\x80\x01"_su8,
              MemoryType{MakeAt("\x01\x00\x80\x01"_su8,
                                Limits{MakeAt("\x00"_su8, u32{0}),
                                       MakeAt("\x80\x01"_su8, u32{128}),
                                       MakeAt("\x01"_su8, Shared::No)})})},
          Memory{MakeAt(
              "\x00\x00"_su8,
              MemoryType{MakeAt("\x00\x00"_su8,
                                Limits{MakeAt("\x00"_su8, u32{0}), nullopt,
                                       MakeAt("\x00"_su8, Shared::No)})})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Global) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadGlobalSection(
      "\x02"                       // Count.
      "\x7f\x01\x41\x00\x0b"       // (global (mut i32) (i32.const 0))
      "\x7e\x00\x42\x01\x0b"_su8,  // (global i64 (i64.const 1))
      context);

  ExpectSection(
      {
          Global{MakeAt("\x7f\x01"_su8,
                        GlobalType{MakeAt("\x7f"_su8, ValueType::I32),
                                   MakeAt("\x01"_su8, Mutability::Var)}),
                 MakeAt("\x41\x00\x0b"_su8,
                        ConstantExpression{MakeAt(
                            "\x41\x00"_su8,
                            Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                        MakeAt("\x00"_su8, s32{0})})})},
          Global{MakeAt("\x7e\x00"_su8,
                        GlobalType{MakeAt("\x7e"_su8, ValueType::I64),
                                   MakeAt("\x00"_su8, Mutability::Const)}),
                 MakeAt("\x42\x01\x0b"_su8,
                        ConstantExpression{MakeAt(
                            "\x42\x01"_su8,
                            Instruction{MakeAt("\x42"_su8, Opcode::I64Const),
                                        MakeAt("\x01"_su8, s64{1})})})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Export) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadExportSection(
      "\x03"                    // Count.
      "\x03one\x00\x01"         // (export "one" (func 1))
      "\x03two\x02\x02"         // (export "two" (memory 2))
      "\x05three\x03\x02"_su8,  // (export "three" (global 2))
      context);

  ExpectSection(
      {
          Export{MakeAt("\x00"_su8, ExternalKind::Function),
                 MakeAt("\x03one"_su8, "one"_sv), MakeAt("\x01"_su8, Index{1})},
          Export{MakeAt("\x02"_su8, ExternalKind::Memory),
                 MakeAt("\x03two"_su8, "two"_sv), MakeAt("\x02"_su8, Index{2})},
          Export{MakeAt("\x03"_su8, ExternalKind::Global),
                 MakeAt("\x05three"_su8, "three"_sv),
                 MakeAt("\x02"_su8, Index{2})},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Start) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadStartSection("\x03"_su8, context);

  EXPECT_EQ(Start{MakeAt("\x03"_su8, Index{3})}, sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Element) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadElementSection(
      "\x02"                           // Count.
      "\x00\x41\x00\x0b\x02\x00\x01"   // (elem (offset i32.const 0) 0 1)
      "\x00\x41\x02\x0b\x01\x03"_su8,  // (elem (offset i32.const 2) 3)
      context);

  ExpectSection(
      {
          ElementSegment{
              MakeAt("\x00"_su8, Index{0}),
              MakeAt("\x41\x00\x0b"_su8,
                     ConstantExpression{MakeAt(
                         "\x41\x00"_su8,
                         Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                     MakeAt("\x00"_su8, s32{0})})}),
              ExternalKind::Function,
              {MakeAt("\x00"_su8, Index{0}), MakeAt("\x01"_su8, Index{1})}},
          ElementSegment{
              MakeAt("\x00"_su8, Index{0}),
              MakeAt("\x41\x02\x0b"_su8,
                     ConstantExpression{MakeAt(
                         "\x41\x02"_su8,
                         Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                     MakeAt("\x02"_su8, s32{2})})}),
              ExternalKind::Function,
              {
                  MakeAt("\x03"_su8, Index{3}),
              }},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Code) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadCodeSection(
      "\x02"                           // Count.
      "\x02\x00\x0b"                   // (func)
      "\x05\x01\x01\x7f\x6a\x0b"_su8,  // (func (local i32) i32.add)
      context);

  ExpectSection(
      {
          Code{{}, MakeAt("\x0b"_su8, "\x0b"_expr)},
          Code{{MakeAt("\x01\x7f"_su8,
                       Locals{MakeAt("\x01"_su8, Index{1}),
                              MakeAt("\x7f"_su8, ValueType::I32)})},
               MakeAt("\x6a\x0b"_su8, "\x6a\x0b"_expr)},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, Data) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadDataSection(
      "\x03"                          // Count.
      "\x00\x41\x00\x0b\x02hi"        // (data (offset i32.const 0) "hi")
      "\x00\x41\x02\x0b\x03see"       // (data (offset i32.const 2) "see")
      "\x00\x41\x05\x0b\x03you"_su8,  // (data (offset i32.const 5) "you")
      context);

  ExpectSection(
      {
          DataSegment{
              MakeAt("\x00"_su8, Index{0}),
              MakeAt("\x41\x00\x0b"_su8,
                     ConstantExpression{MakeAt(
                         "\x41\x00"_su8,
                         Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                     MakeAt("\x00"_su8, s32{0})})}),
              MakeAt("\x02hi"_su8, "hi"_su8)},
          DataSegment{
              MakeAt("\x00"_su8, Index{0}),
              MakeAt("\x41\x02\x0b"_su8,
                     ConstantExpression{MakeAt(
                         "\x41\x02"_su8,
                         Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                     MakeAt("\x02"_su8, s32{2})})}),
              MakeAt("\x03see"_su8, "see"_su8)},
          DataSegment{
              MakeAt("\x00"_su8, Index{0}),
              MakeAt("\x41\x05\x0b"_su8,
                     ConstantExpression{MakeAt(
                         "\x41\x05"_su8,
                         Instruction{MakeAt("\x41"_su8, Opcode::I32Const),
                                     MakeAt("\x05"_su8, s32{5})})}),
              MakeAt("\x03you"_su8, "you"_su8),
          },
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazySectionTest, DataCount) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadDataCountSection("\x03"_su8, context);

  EXPECT_EQ(DataCount{MakeAt("\x03"_su8, Index{3})}, sec);
  ExpectNoErrors(errors);
}

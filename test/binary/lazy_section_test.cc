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
#include "test/binary/constants.h"
#include "test/binary/test_utils.h"
#include "test/test_utils.h"
#include "wasp/binary/read/read_ctx.h"
#include "wasp/binary/sections.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;
using namespace ::wasp::test;

namespace {

class BinaryLazySectionTest : public ::testing::Test {
 public:
  void SetUp() { ctx.features.DisableAll(); }

 protected:
  TestErrors errors;
  ReadCtx ctx{errors};
};

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

TEST_F(BinaryLazySectionTest, Type) {
  auto sec = ReadTypeSection(
      "\x02"                           // Count.
      "\x60\x00\x00"                   // (param) (result)
      "\x60\x02\x7f\x7f\x01\x7f"_su8,  // (param i32 i32) (result i32)
      ctx);

  ExpectSection(
      {
          DefinedType{At{"\x00\x00"_su8, FunctionType{{}, {}}}},
          DefinedType{At{
              "\x02\x7f\x7f\x01\x7f"_su8,
              FunctionType{{At{"\x07f"_su8, VT_I32}, At{"\x07f"_su8, VT_I32}},
                           {At{"\x07f"_su8, VT_I32}}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Import) {
  auto sec = ReadImportSection(
      "\x02"                             // Count.
      "\x01w\x01x\x00\x02"               // (import "w" "x" (func 2))
      "\x01y\x01z\x02\x01\x01\x02"_su8,  // (import "y" "z" (memory 1 2))
      ctx);

  ExpectSection(
      {
          Import{At{"\x01w"_su8, "w"_sv}, At{"\x01x"_su8, "x"_sv},
                 At{"\x02"_su8, Index{2}}},
          Import{At{"\x01y"_su8, "y"_sv}, At{"\x01z"_su8, "z"_sv},
                 At{"\x01\x01\x02"_su8,
                    MemoryType{At{
                        "\x01\x01\x02"_su8,
                        Limits{At{"\x01"_su8, u32{1}}, At{"\x02"_su8, u32{2}},
                               At{"\x01"_su8, Shared::No}}}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Function) {
  auto sec = ReadFunctionSection(
      "\x03"                   // Count.
      "\x02\x80\x01\x02"_su8,  // 2, 128, 2
      ctx);

  ExpectSection(
      {
          Function{At{"\x02"_su8, Index{2}}},
          Function{At{"\x80\x01"_su8, Index{128}}},
          Function{At{"\x02"_su8, Index{2}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Table) {
  auto sec = ReadTableSection(
      "\x03"                  // Count.
      "\x70\x00\x01"          // (table 1 funcref)
      "\x70\x01\x00\x80\x01"  // (table 0 128 funcref)
      "\x70\x00\x00"_su8,     // (table 0 funcref)
      ctx);

  ExpectSection(
      {
          Table{At{"\x70\x00\x01"_su8,
                   TableType{At{"\x00\x01"_su8,
                                Limits{At{"\x01"_su8, Index{1}}, nullopt,
                                       At{"\x00"_su8, Shared::No}}},
                             At{"\x70"_su8, RT_Funcref}}}},
          Table{At{"\x70\x01\x00\x80\x01"_su8,
                   TableType{At{"\x01\x00\x80\x01"_su8,
                                Limits{At{"\x00"_su8, u32{0}},
                                       At{"\x80\x01"_su8, u32{128}},
                                       At{"\x01"_su8, Shared::No}}},
                             At{"\x70"_su8, RT_Funcref}}}},
          Table{At{"\x70\x00\x00"_su8,
                   TableType{At{"\x00\x00"_su8,
                                Limits{At{"\x00"_su8, u32{0}}, nullopt,
                                       At{"\x00"_su8, Shared::No}}},
                             At{"\x70"_su8, RT_Funcref}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Memory) {
  auto sec = ReadMemorySection(
      "\x03"              // Count.
      "\x00\x01"          // (memory 1)
      "\x01\x00\x80\x01"  // (memory 0 128)
      "\x00\x00"_su8,     // (memory 0)
      ctx);

  ExpectSection(
      {
          Memory{At{"\x00\x01"_su8,
                    MemoryType{At{"\x00\x01"_su8,
                                  Limits{At{"\x01"_su8, u32{1}}, nullopt,
                                         At{"\x00"_su8, Shared::No}}}}}},
          Memory{At{"\x01\x00\x80\x01"_su8,
                    MemoryType{At{"\x01\x00\x80\x01"_su8,
                                  Limits{At{"\x00"_su8, u32{0}},
                                         At{"\x80\x01"_su8, u32{128}},
                                         At{"\x01"_su8, Shared::No}}}}}},
          Memory{At{"\x00\x00"_su8,
                    MemoryType{At{"\x00\x00"_su8,
                                  Limits{At{"\x00"_su8, u32{0}}, nullopt,
                                         At{"\x00"_su8, Shared::No}}}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Global) {
  auto sec = ReadGlobalSection(
      "\x02"                       // Count.
      "\x7f\x01\x41\x00\x0b"       // (global (mut i32) (i32.const 0))
      "\x7e\x00\x42\x01\x0b"_su8,  // (global i64 (i64.const 1))
      ctx);

  ExpectSection(
      {
          Global{
              At{"\x7f\x01"_su8, GlobalType{At{"\x7f"_su8, VT_I32},
                                            At{"\x01"_su8, Mutability::Var}}},
              At{"\x41\x00\x0b"_su8,
                 ConstantExpression{
                     At{"\x41\x00"_su8,
                        Instruction{At{"\x41"_su8, Opcode::I32Const},
                                    At{"\x00"_su8, s32{0}}}}}}},
          Global{
              At{"\x7e\x00"_su8, GlobalType{At{"\x7e"_su8, VT_I64},
                                            At{"\x00"_su8, Mutability::Const}}},
              At{"\x42\x01\x0b"_su8,
                 ConstantExpression{
                     At{"\x42\x01"_su8,
                        Instruction{At{"\x42"_su8, Opcode::I64Const},
                                    At{"\x01"_su8, s64{1}}}}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Export) {
  auto sec = ReadExportSection(
      "\x03"                    // Count.
      "\x03one\x00\x01"         // (export "one" (func 1))
      "\x03two\x02\x02"         // (export "two" (memory 2))
      "\x05three\x03\x02"_su8,  // (export "three" (global 2))
      ctx);

  ExpectSection(
      {
          Export{At{"\x00"_su8, ExternalKind::Function},
                 At{"\x03one"_su8, "one"_sv}, At{"\x01"_su8, Index{1}}},
          Export{At{"\x02"_su8, ExternalKind::Memory},
                 At{"\x03two"_su8, "two"_sv}, At{"\x02"_su8, Index{2}}},
          Export{At{"\x03"_su8, ExternalKind::Global},
                 At{"\x05three"_su8, "three"_sv}, At{"\x02"_su8, Index{2}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Start) {
  auto sec = ReadStartSection("\x03"_su8, ctx);

  EXPECT_EQ((Start{At{"\x03"_su8, Index{3}}}), sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Element) {
  auto sec = ReadElementSection(
      "\x02"                           // Count.
      "\x00\x41\x00\x0b\x02\x00\x01"   // (elem (offset i32.const 0) 0 1)
      "\x00\x41\x02\x0b\x01\x03"_su8,  // (elem (offset i32.const 2) 3)
      ctx);

  ExpectSection(
      {
          ElementSegment{At{"\x00"_su8, Index{0}},
                         At{"\x41\x00\x0b"_su8,
                            ConstantExpression{
                                At{"\x41\x00"_su8,
                                   Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x00"_su8, s32{0}}}}}},
                         ElementListWithIndexes{ExternalKind::Function,
                                                {At{"\x00"_su8, Index{0}},
                                                 At{"\x01"_su8, Index{1}}}}},
          ElementSegment{At{"\x00"_su8, Index{0}},
                         At{"\x41\x02\x0b"_su8,
                            ConstantExpression{
                                At{"\x41\x02"_su8,
                                   Instruction{At{"\x41"_su8, Opcode::I32Const},
                                               At{"\x02"_su8, s32{2}}}}}},
                         ElementListWithIndexes{ExternalKind::Function,
                                                {At{"\x03"_su8, Index{3}}}}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Code) {
  auto sec = ReadCodeSection(
      "\x02"                           // Count.
      "\x02\x00\x0b"                   // (func)
      "\x05\x01\x01\x7f\x6a\x0b"_su8,  // (func (local i32) i32.add)
      ctx);

  ExpectSection(
      {
          Code{{}, At{"\x0b"_su8, "\x0b"_expr}},
          Code{{At{"\x01\x7f"_su8,
                   Locals{At{"\x01"_su8, Index{1}}, At{"\x7f"_su8, VT_I32}}}},
               At{"\x6a\x0b"_su8, "\x6a\x0b"_expr}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, Data) {
  auto sec = ReadDataSection(
      "\x03"                          // Count.
      "\x00\x41\x00\x0b\x02hi"        // (data (offset i32.const 0) "hi")
      "\x00\x41\x02\x0b\x03see"       // (data (offset i32.const 2) "see")
      "\x00\x41\x05\x0b\x03you"_su8,  // (data (offset i32.const 5) "you")
      ctx);

  ExpectSection(
      {
          DataSegment{At{"\x00"_su8, Index{0}},
                      At{"\x41\x00\x0b"_su8,
                         ConstantExpression{
                             At{"\x41\x00"_su8,
                                Instruction{At{"\x41"_su8, Opcode::I32Const},
                                            At{"\x00"_su8, s32{0}}}}}},
                      At{"\x02hi"_su8, "hi"_su8}},
          DataSegment{At{"\x00"_su8, Index{0}},
                      At{"\x41\x02\x0b"_su8,
                         ConstantExpression{
                             At{"\x41\x02"_su8,
                                Instruction{At{"\x41"_su8, Opcode::I32Const},
                                            At{"\x02"_su8, s32{2}}}}}},
                      At{"\x03see"_su8, "see"_su8}},
          DataSegment{
              At{"\x00"_su8, Index{0}},
              At{"\x41\x05\x0b"_su8,
                 ConstantExpression{
                     At{"\x41\x05"_su8,
                        Instruction{At{"\x41"_su8, Opcode::I32Const},
                                    At{"\x05"_su8, s32{5}}}}}},
              At{"\x03you"_su8, "you"_su8},
          },
      },
      sec);
  ExpectNoErrors(errors);
}

TEST_F(BinaryLazySectionTest, DataCount) {
  auto sec = ReadDataCountSection("\x03"_su8, ctx);

  EXPECT_EQ((DataCount{At{"\x03"_su8, Index{3}}}), sec);
  ExpectNoErrors(errors);
}

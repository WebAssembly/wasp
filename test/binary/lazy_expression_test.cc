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

#include "wasp/binary/lazy_expression.h"

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "wasp/binary/read/context.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

TEST(BinaryLazyExprTest, Basic) {
  TestErrors errors;
  Context context{errors};
  auto expr = ReadExpression("\x00"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((At{"\x00"_su8, Instruction{At{"\x00"_su8, Opcode::Unreachable}}}),
            *it++);
  ASSERT_EQ(end, it);
}

TEST(BinaryLazyExprTest, Multiple) {
  TestErrors errors;
  Context context{errors};
  auto expr = ReadExpression("\x01\x01"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}}), *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((At{"\x01"_su8, Instruction{At{"\x01"_su8, Opcode::Nop}}}), *it++);
  ASSERT_EQ(end, it);
}

TEST(BinaryLazyExprTest, SimpleFunction) {
  TestErrors errors;
  Context context{errors};
  // local.get 0
  // local.get 1
  // i32.add
  auto expr = ReadExpression("\x20\x00\x20\x01\x6a"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((At{"\x20\x00"_su8, Instruction{At{"\x20"_su8, Opcode::LocalGet},
                                            At{"\x00"_su8, Index{0}}}}),
            *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((At{"\x20\x01"_su8, Instruction{At{"\x20"_su8, Opcode::LocalGet},
                                            At{"\x01"_su8, Index{1}}}}),
            *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((At{"\x6a"_su8, Instruction{At{"\x6a"_su8, Opcode::I32Add}}}),
            *it++);
  ASSERT_EQ(end, it);
}

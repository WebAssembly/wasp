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
#include "wasp/binary/read/context.h"

#include "gtest/gtest.h"

#include "test/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(LazyExprTest, Basic) {
  TestErrors errors;
  Context context{errors};
  auto expr = ReadExpression("\x00"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((Instruction{Opcode::Unreachable}), *it++);
  ASSERT_EQ(end, it);
}

TEST(LazyExprTest, Multiple) {
  TestErrors errors;
  Context context{errors};
  auto expr = ReadExpression("\x01\x01"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((Instruction{Opcode::Nop}), *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((Instruction{Opcode::Nop}), *it++);
  ASSERT_EQ(end, it);
}

TEST(LazyExprTest, SimpleFunction) {
  TestErrors errors;
  Context context{errors};
  // local.get 0
  // local.get 1
  // i32.add
  auto expr = ReadExpression("\x20\x00\x20\x01\x6a"_su8, context);
  auto it = expr.begin(), end = expr.end();
  EXPECT_EQ((Instruction{Opcode::LocalGet, Index{0}}), *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((Instruction{Opcode::LocalGet, Index{1}}), *it++);
  ASSERT_NE(end, it);
  EXPECT_EQ((Instruction{Opcode::I32Add}), *it++);
  ASSERT_EQ(end, it);
}

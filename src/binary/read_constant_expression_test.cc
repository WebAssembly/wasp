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

#include "wasp/binary/read/read_constant_expression.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ConstantExpression) {
  // i32.const
  {
    const auto data = MakeSpanU8("\x41\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // i64.const
  {
    const auto data = MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // f32.const
  {
    const auto data = MakeSpanU8("\x43\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // f64.const
  {
    const auto data = MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }

  // get_global
  {
    const auto data = MakeSpanU8("\x23\x00\x0b");
    ExpectRead<ConstantExpression>(ConstantExpression{data}, data);
  }
}

TEST(ReaderTest, ConstantExpression_NoEnd) {
  // i32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x41\x00"));

  // i64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {7, "opcode"}, {7, "Unable to read u8"}},
      MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01"));

  // f32.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {5, "opcode"}, {5, "Unable to read u8"}},
      MakeSpanU8("\x43\x00\x00\x00\x00"));

  // f64.const
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {9, "opcode"}, {9, "Unable to read u8"}},
      MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"));

  // get_global
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {2, "opcode"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x23\x00"));
}

TEST(ReaderTest, ConstantExpression_TooLong) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {3, "Expected end instruction"}},
      MakeSpanU8("\x41\x00\x01\x0b"));
}

TEST(ReaderTest, ConstantExpression_InvalidInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {1, "Unknown opcode: 6"}},
      MakeSpanU8("\x06"));
}

TEST(ReaderTest, ConstantExpression_IllegalInstruction) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"},
       {1, "Illegal instruction in constant expression: unreachable"}},
      MakeSpanU8("\x00"));
}

TEST(ReaderTest, ConstantExpression_PastEnd) {
  ExpectReadFailure<ConstantExpression>(
      {{0, "constant expression"}, {0, "opcode"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));
}

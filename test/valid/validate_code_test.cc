//
// Copyright 2019 WebAssembly Community Group participants
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

#include "gtest/gtest.h"

#include "wasp/base/features.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/begin_code.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate_instruction.h"
#include "wasp/valid/validate_locals.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::valid;

namespace {

// XXX
struct Errors {
  void PushContext(string_view desc) {}
  void PopContext() {}
  void OnError(string_view message) { print("Error: {}\n", message); }
};

}  // namespace

TEST(ValidateCodeTest, BeginCode) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  Errors errors;
  EXPECT_TRUE(BeginCode(context, Features{}, errors));
}

TEST(ValidateCodeTest, BeginCode_CodeIndexOOB) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  context.code_count = 1;
  Errors errors;
  EXPECT_FALSE(BeginCode(context, Features{}, errors));
}

TEST(ValidateCodeTest, BeginCode_TypeIndexOOB) {
  Context context;
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{1});
  Errors errors;
  EXPECT_FALSE(BeginCode(context, Features{}, errors));
}

TEST(ValidateCodeTest, Locals) {
  Context context;
  Errors errors;
  EXPECT_TRUE(
      Validate(Locals{10, ValueType::I32}, context, Features{}, errors));
}

class ValidateInstructionTest : public ::testing::Test {
 protected:
  using I = Instruction;
  using O = Opcode;

  virtual void SetUp() {
    context.types.push_back(TypeEntry{FunctionType{}});
    context.functions.push_back(Function{0});
    EXPECT_TRUE(BeginCode(context, features, errors));
  }

  virtual void TearDown() {
  }

  Context context;
  Features features;
  Errors errors;
};

TEST_F(ValidateInstructionTest, Unreachable) {
  EXPECT_TRUE(Validate(I{O::Unreachable}, context, features, errors));
}

TEST_F(ValidateInstructionTest, Nop) {
  EXPECT_TRUE(Validate(I{O::Nop}, context, features, errors));
}

TEST_F(ValidateInstructionTest, Block) {
  EXPECT_TRUE(
      Validate(I{O::Block, BlockType::Void}, context, features, errors));
  EXPECT_TRUE(Validate(I{O::Block, BlockType::I32}, context, features, errors));
}

TEST_F(ValidateInstructionTest, Loop) {
  EXPECT_TRUE(Validate(I{O::Loop, BlockType::Void}, context, features, errors));
  EXPECT_TRUE(Validate(I{O::Loop, BlockType::I32}, context, features, errors));
}

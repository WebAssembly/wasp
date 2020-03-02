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
#include "test/valid/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/valid/begin_code.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

TEST(ValidateCodeTest, BeginCode) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  EXPECT_TRUE(BeginCode(Location{}, context));
}

TEST(ValidateCodeTest, BeginCode_CodeIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{0});
  context.code_count = 1;
  EXPECT_FALSE(BeginCode(Location{}, context));
}

TEST(ValidateCodeTest, BeginCode_TypeIndexOOB) {
  TestErrors errors;
  Context context{errors};
  context.types.push_back(TypeEntry{FunctionType{}});
  context.functions.push_back(Function{1});
  EXPECT_FALSE(BeginCode(Location{}, context));
}

TEST(ValidateCodeTest, Locals) {
  TestErrors errors;
  Context context{errors};
  EXPECT_TRUE(Validate(Locals{10, ValueType::I32}, context));
}

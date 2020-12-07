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
#include "test/binary/constants.h"
#include "test/valid/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/valid/valid_ctx.h"
#include "wasp/valid/validate.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;

TEST(ValidateCodeTest, BeginCode) {
  TestErrors errors;
  ValidCtx ctx{errors};
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_TRUE(BeginCode(ctx, Location{}));
}

TEST(ValidateCodeTest, BeginCode_CodeIndexOOB) {
  TestErrors errors;
  ValidCtx ctx{errors};
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.functions.push_back(Function{0});
  ctx.code_count = 1;
  EXPECT_FALSE(BeginCode(ctx, Location{}));
}

TEST(ValidateCodeTest, BeginCode_TypeIndexOOB) {
  TestErrors errors;
  ValidCtx ctx{errors};
  ctx.types.push_back(DefinedType{FunctionType{}});
  ctx.functions.push_back(Function{1});
  EXPECT_FALSE(BeginCode(ctx, Location{}));
}

TEST(ValidateCodeTest, BeginCode_NonFunctionType) {
  TestErrors errors;
  ValidCtx ctx{errors};
  ctx.types.push_back(DefinedType{StructType{}});
  ctx.defined_type_count = 1;
  ctx.functions.push_back(Function{0});
  EXPECT_FALSE(BeginCode(ctx, Location{}));
}

TEST(ValidateCodeTest, Locals) {
  TestErrors errors;
  ValidCtx ctx{errors};
  EXPECT_TRUE(Validate(ctx, Locals{10, VT_I32}, RequireDefaultable::Yes));
}

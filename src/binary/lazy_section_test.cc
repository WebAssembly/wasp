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

#include "src/binary/lazy_section.h"

#include "gtest/gtest.h"

#include "src/binary/errors_nop.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(LazySectionTest, Type) {
  TestErrors errors;
  auto sec = ReadTypeSection(
      MakeSpanU8("\x02"                        // Count.
                 "\x60\x00\x00"                // (param) (result)
                 "\x60\x02\x7f\x7f\x01\x7f"),  // (param i32 i32) (result i32)
      errors);
  auto it = sec.sequence.begin(), end = sec.sequence.end();

  EXPECT_EQ(2u, sec.count);

  EXPECT_EQ((TypeEntry{FunctionType{{}, {}}}), *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((TypeEntry{FunctionType{{ValueType::I32, ValueType::I32},
                                    {ValueType::I32}}}),
            *it++);
  ASSERT_EQ(end, it);
}

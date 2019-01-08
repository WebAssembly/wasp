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

#include "wasp/binary/lazy_sequence.h"

#include "gtest/gtest.h"

#include "src/binary/test_utils.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/read/read_s32.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_u8.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(LazySequenceTest, Basic) {
  ErrorsNop errors;
  LazySequence<u32, ErrorsNop> seq{MakeSpanU8("\x01\x80\x02\x00\x80\x80\x01"),
                                   errors};
  auto it = seq.begin();

  EXPECT_EQ(1u, *it++);
  ASSERT_NE(seq.end(), it);

  EXPECT_EQ(256u, *it++);
  ASSERT_NE(seq.end(), it);

  EXPECT_EQ(0u, *it++);
  ASSERT_NE(seq.end(), it);

  EXPECT_EQ(16384u, *it++);
  ASSERT_EQ(seq.end(), it);
}

TEST(LazySequenceTest, Empty) {
  ErrorsNop errors;
  LazySequence<u8, ErrorsNop> seq{MakeSpanU8(""), errors};

  EXPECT_EQ(seq.begin(), seq.end());
}

TEST(LazySequenceTest, Error) {
  TestErrors errors;
  const auto data = MakeSpanU8("\x40\x30\x80");
  LazySequence<s32, TestErrors> seq{data, errors};

  auto it = seq.begin();

  EXPECT_EQ(-64, *it++);
  ASSERT_NE(seq.end(), it);

  EXPECT_EQ(48, *it++);
  ASSERT_EQ(seq.end(), it);

  ExpectError({{2, "s32"}, {3, "Unable to read u8"}}, errors, data);
}

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
#include "test/binary/test_utils.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/read.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(LazySequenceTest, Basic) {
  Features features;
  ErrorsNop errors;
  LazySequence<u32> seq{"\x01\x80\x02\x00\x80\x80\x01"_su8, features, errors};
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
  Features features;
  ErrorsNop errors;
  LazySequence<u8> seq{""_su8, features, errors};

  EXPECT_EQ(seq.begin(), seq.end());
}

TEST(LazySequenceTest, Error) {
  Features features;
  TestErrors errors;
  const auto data = "\x40\x30\x80"_su8;
  LazySequence<s32> seq{data, features, errors};

  auto it = seq.begin();

  EXPECT_EQ(-64, *it++);
  ASSERT_NE(seq.end(), it);

  EXPECT_EQ(48, *it++);
  ASSERT_EQ(seq.end(), it);

  ExpectError({{2, "s32"}, {3, "Unable to read u8"}}, errors, data);
}

TEST(LazySequenceTest, ExpectedCount_Match) {
  Features features;
  TestErrors errors;
  const auto data = "\x00\x01"_su8;
  LazySequence<s32> seq{data, 2, "MySequence", features, errors};

  auto it = seq.begin();
  it++;
  it++;
  ExpectNoErrors(errors);
}

TEST(LazySequenceTest, ExpectedCount_ActualLess) {
  Features features;
  TestErrors errors;
  const auto data = "\x00"_su8;
  LazySequence<s32> seq{data, 2, "MySequence", features, errors};

  auto it = seq.begin();
  it++;
  ExpectError({{1, "Expected MySequence to have count 2, got 1"}}, errors,
              data);
}

TEST(LazySequenceTest, ExpectedCount_ActualMore) {
  Features features;
  TestErrors errors;
  const auto data = "\x00\x01\x02\x03"_su8;
  LazySequence<s32> seq{data, 2, "MySequence", features, errors};

  auto it = seq.begin();
  it++;
  it++;
  it++;
  it++;
  ExpectError({{4, "Expected MySequence to have count 2, got 4"}}, errors,
              data);
}

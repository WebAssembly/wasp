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

#include "wasp/binary/read/read_vector.h"

#include <vector>

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/read/read_u8.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ReadVector_u8) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u8>{'h', 'e', 'l', 'l', 'o'}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVector_u32) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ((std::vector<u32>{5, 128, 206412}), result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVector_FailLength) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {1, "Count extends past end: 2 > 1"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadVector_PastEnd) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05"
      "\x80");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(0u, copy.size());
}

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

#include "wasp/binary/read/read_string.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ReadString) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"hello"}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadString_Leftovers) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01more");
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectNoErrors(errors);
  EXPECT_EQ(string_view{"m"}, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadString_BadLength) {
  {
    Features features;
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("");
    SpanU8 copy = data;
    auto result = ReadString(&copy, features, errors, "test");
    ExpectError({{0, "test"}, {0, "length"}, {0, "Unable to read u8"}}, errors,
                data);
    EXPECT_EQ(nullopt, result);
    EXPECT_EQ(0u, copy.size());
  }

  {
    Features features;
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("\xc0");
    SpanU8 copy = data;
    auto result = ReadString(&copy, features, errors, "test");
    ExpectError({{0, "test"}, {0, "length"}, {1, "Unable to read u8"}}, errors,
                data);
    EXPECT_EQ(nullopt, result);
    EXPECT_EQ(0u, copy.size());
  }
}

TEST(ReaderTest, ReadString_Fail) {
  Features features;
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x06small");
  SpanU8 copy = data;
  auto result = ReadString(&copy, features, errors, "test");
  ExpectError({{0, "test"}, {1, "Length extends past end: 6 > 5"}}, errors,
              data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(5u, copy.size());
}

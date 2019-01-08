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

#include "wasp/binary/read/read_count.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ReadCount) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadCount_PastEnd) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectError({{1, "Count extends past end: 5 > 3"}}, errors, data);
  EXPECT_EQ(nullopt, result);
  EXPECT_EQ(3u, copy.size());
}

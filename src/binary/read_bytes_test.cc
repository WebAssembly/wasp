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

#include "wasp/binary/read/read_bytes.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ReadBytes) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadBytes_Leftovers) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, errors);
  ExpectNoErrors(errors);
  EXPECT_EQ(data.subspan(0, 2), result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadBytes_Fail) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, errors);
  EXPECT_EQ(nullopt, result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

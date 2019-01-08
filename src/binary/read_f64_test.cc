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

#include "wasp/binary/read/read_f64.h"

#include <cmath>

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, F64) {
  ExpectRead<f64>(0.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<f64>(-1.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xbf"));
  ExpectRead<f64>(111111111111111,
                  MakeSpanU8("\xc0\x71\xbc\x93\x84\x43\xd9\x42"));
  ExpectRead<f64>(INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\x7f"));
  ExpectRead<f64>(-INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf8\x7f");
    TestErrors errors;
    auto result = Read<f64>(&data, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, F64_PastEnd) {
  ExpectReadFailure<f64>({{0, "f64"}, {0, "Unable to read 8 bytes"}},
                         MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00"));
}

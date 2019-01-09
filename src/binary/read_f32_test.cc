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

#include "wasp/binary/read/read_f32.h"

#include <cmath>

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, F32) {
  ExpectRead<f32>(0.0f, MakeSpanU8("\x00\x00\x00\x00"));
  ExpectRead<f32>(-1.0f, MakeSpanU8("\x00\x00\x80\xbf"));
  ExpectRead<f32>(1234567.0f, MakeSpanU8("\x38\xb4\x96\x49"));
  ExpectRead<f32>(INFINITY, MakeSpanU8("\x00\x00\x80\x7f"));
  ExpectRead<f32>(-INFINITY, MakeSpanU8("\x00\x00\x80\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\xc0\x7f");
    Features features;
    TestErrors errors;
    auto result = Read<f32>(&data, features, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, F32_PastEnd) {
  ExpectReadFailure<f32>({{0, "f32"}, {0, "Unable to read 4 bytes"}},
                         MakeSpanU8("\x00\x00\x00"));
}

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

#include "wasp/binary/read/read_u32.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, U32) {
  ExpectRead<u32>(32u, MakeSpanU8("\x20"));
  ExpectRead<u32>(448u, MakeSpanU8("\xc0\x03"));
  ExpectRead<u32>(33360u, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<u32>(101718048u, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<u32>(1042036848u, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
}

TEST(ReaderTest, U32_TooLong) {
  ExpectReadFailure<u32>(
      {{0, "u32"},
       {5, "Last byte of u32 must be zero extension: expected 0x2, got 0x12"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\x12"));
}

TEST(ReaderTest, U32_PastEnd) {
  ExpectReadFailure<u32>({{0, "u32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<u32>({{0, "u32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<u32>({{0, "u32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

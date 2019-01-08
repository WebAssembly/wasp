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

#include "wasp/binary/read/read_s32.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, S32) {
  ExpectRead<s32>(32, MakeSpanU8("\x20"));
  ExpectRead<s32>(-16, MakeSpanU8("\x70"));
  ExpectRead<s32>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s32>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s32>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s32>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s32>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s32>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s32>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s32>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
}

TEST(ReaderTest, S32_TooLong) {
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x5 or 0x7d, got 0x15"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0\x15"));
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x3 or 0x7b, got 0x73"}},
                         MakeSpanU8("\xff\xff\xff\xff\x73"));
}

TEST(ReaderTest, S32_PastEnd) {
  ExpectReadFailure<s32>({{0, "s32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s32>({{0, "s32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s32>({{0, "s32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

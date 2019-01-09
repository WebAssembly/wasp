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

#include "wasp/binary/read/read_s64.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, S64) {
  ExpectRead<s64>(32, MakeSpanU8("\x20"));
  ExpectRead<s64>(-16, MakeSpanU8("\x70"));
  ExpectRead<s64>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s64>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s64>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s64>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s64>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s64>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s64>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s64>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
  ExpectRead<s64>(13893120096, MakeSpanU8("\xe0\xe0\xe0\xe0\x33"));
  ExpectRead<s64>(-12413554592, MakeSpanU8("\xe0\xe0\xe0\xe0\x51"));
  ExpectRead<s64>(1533472417872, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"));
  ExpectRead<s64>(-287593715632, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"));
  ExpectRead<s64>(139105536057408, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"));
  ExpectRead<s64>(-124777254608832, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"));
  ExpectRead<s64>(1338117014066474,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"));
  ExpectRead<s64>(-12172681868045014,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"));
  ExpectRead<s64>(1070725794579330814,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"));
  ExpectRead<s64>(-3540960223848057090,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"));
}

TEST(ReaderTest, S64_TooLong) {
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xf0"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xff"}},
      MakeSpanU8("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"));
}

TEST(ReaderTest, S64_PastEnd) {
  ExpectReadFailure<s64>({{0, "s64"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s64>({{0, "s64"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>({{0, "s64"}, {5, "Unable to read u8"}},
                         MakeSpanU8("\xe0\xe0\xe0\xe0\xe0"));
  ExpectReadFailure<s64>({{0, "s64"}, {6, "Unable to read u8"}},
                         MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {7, "Unable to read u8"}},
                         MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {8, "Unable to read u8"}},
                         MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {9, "Unable to read u8"}},
                         MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"));
}

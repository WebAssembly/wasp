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

#include <iterator>
#include <string>
#include <vector>

#include "gtest/gtest.h"

// Write() functions must be declared here before they can be used by
// ExpectWrite (defined in write_test_utils.h below).
#include "wasp/binary/write/write_bytes.h"
#include "wasp/binary/write/write_s32.h"
#include "wasp/binary/write/write_s64.h"
#include "wasp/binary/write/write_string.h"
#include "wasp/binary/write/write_u32.h"
#include "wasp/binary/write/write_u8.h"

#include "src/binary/test_utils.h"
#include "src/binary/write_test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(WriteTest, Bytes) {
  const std::vector<u8> input{{0x12, 0x34, 0x56}};
  std::vector<u8> output;
  WriteBytes(input, std::back_inserter(output), Features{});
  EXPECT_EQ(input, output);
}

TEST(WriteTest, S32) {
  ExpectWrite<s32>(MakeSpanU8("\x20"), 32);
  ExpectWrite<s32>(MakeSpanU8("\x70"), -16);
  ExpectWrite<s32>(MakeSpanU8("\xc0\x03"), 448);
  ExpectWrite<s32>(MakeSpanU8("\xc0\x63"), -3648);
  ExpectWrite<s32>(MakeSpanU8("\xd0\x84\x02"), 33360);
  ExpectWrite<s32>(MakeSpanU8("\xd0\x84\x52"), -753072);
  ExpectWrite<s32>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048);
  ExpectWrite<s32>(MakeSpanU8("\xa0\xb0\xc0\x70"), -32499680);
  ExpectWrite<s32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848);
  ExpectWrite<s32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"), -837011344);
}

TEST(WriteTest, S64) {
  ExpectWrite<s64>(MakeSpanU8("\x20"), 32);
  ExpectWrite<s64>(MakeSpanU8("\x70"), -16);
  ExpectWrite<s64>(MakeSpanU8("\xc0\x03"), 448);
  ExpectWrite<s64>(MakeSpanU8("\xc0\x63"), -3648);
  ExpectWrite<s64>(MakeSpanU8("\xd0\x84\x02"), 33360);
  ExpectWrite<s64>(MakeSpanU8("\xd0\x84\x52"), -753072);
  ExpectWrite<s64>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048);
  ExpectWrite<s64>(MakeSpanU8("\xa0\xb0\xc0\x70"), -32499680);
  ExpectWrite<s64>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848);
  ExpectWrite<s64>(MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"), -837011344);
  ExpectWrite<s64>(MakeSpanU8("\xe0\xe0\xe0\xe0\x33"), 13893120096);
  ExpectWrite<s64>(MakeSpanU8("\xe0\xe0\xe0\xe0\x51"), -12413554592);
  ExpectWrite<s64>(MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"), 1533472417872);
  ExpectWrite<s64>(MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"), -287593715632);
  ExpectWrite<s64>(MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"), 139105536057408);
  ExpectWrite<s64>(MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"),
                   -124777254608832);
  ExpectWrite<s64>(MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"),
                   1338117014066474);
  ExpectWrite<s64>(MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"),
                   -12172681868045014);
  ExpectWrite<s64>(MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"),
                   1070725794579330814);
  ExpectWrite<s64>(MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"),
                   -3540960223848057090);
}

TEST(WriteTest, String) {
  ExpectWrite<string_view>(MakeSpanU8("\x05hello"), "hello");
  ExpectWrite<string_view>(MakeSpanU8("\x02hi"), std::string{"hi"});
}

TEST(WriteTest, U8) {
  ExpectWrite<u8>(MakeSpanU8("\x2a"), 42);
}

TEST(WriteTest, U32) {
  ExpectWrite<u32>(MakeSpanU8("\x20"), 32u);
  ExpectWrite<u32>(MakeSpanU8("\xc0\x03"), 448u);
  ExpectWrite<u32>(MakeSpanU8("\xd0\x84\x02"), 33360u);
  ExpectWrite<u32>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048u);
  ExpectWrite<u32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848u);
}

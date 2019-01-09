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

#include "wasp/binary/read/read_init_immediate.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, InitImmediate) {
  ExpectRead<InitImmediate>(InitImmediate{1, 0}, MakeSpanU8("\x01\x00"));
  ExpectRead<InitImmediate>(InitImmediate{128, 0}, MakeSpanU8("\x80\x01\x00"));
}

TEST(ReaderTest, InitImmediate_BadReserved) {
  ExpectReadFailure<InitImmediate>({{0, "init immediate"},
                                    {1, "reserved"},
                                    {2, "Expected reserved byte 0, got 1"}},
                                   MakeSpanU8("\x00\x01"));
}

TEST(ReaderTest, InitImmediate_PastEnd) {
  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {0, "segment index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<InitImmediate>(
      {{0, "init immediate"}, {1, "reserved"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x01"));
}

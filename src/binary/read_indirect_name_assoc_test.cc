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

#include "wasp/binary/read/read_indirect_name_assoc.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, IndirectNameAssoc) {
  ExpectRead<IndirectNameAssoc>(
      IndirectNameAssoc{100u, {{0u, "zero"}, {1u, "one"}}},
      MakeSpanU8("\x64"          // Index.
                 "\x02"          // Count.
                 "\x00\x04zero"  // 0 "zero"
                 "\x01\x03one"   // 1 "one"
                 ));
}

TEST(ReaderTest, IndirectNameAssoc_PastEnd) {
  ExpectReadFailure<IndirectNameAssoc>(
      {{0, "indirect name assoc"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {1, "count"},
                                        {1, "Unable to read u8"}},
                                       MakeSpanU8("\x00"));

  ExpectReadFailure<IndirectNameAssoc>({{0, "indirect name assoc"},
                                        {1, "name map"},
                                        {2, "Count extends past end: 1 > 0"}},
                                       MakeSpanU8("\x00\x01"));
}

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

#include "wasp/binary/read/read_data_segment.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, DataSegment) {
  ExpectRead<DataSegment>(DataSegment{1, MakeConstantExpression("\x42\x01\x0b"),
                                      MakeSpanU8("wxyz")},
                          MakeSpanU8("\x01\x42\x01\x0b\x04wxyz"));
}

TEST(ReaderTest, DataSegment_PastEnd) {
  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {0, "memory index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<DataSegment>({{0, "data segment"},
                                  {1, "offset"},
                                  {1, "constant expression"},
                                  {1, "opcode"},
                                  {1, "Unable to read u8"}},
                                 MakeSpanU8("\x00"));

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {4, "length"}, {4, "Unable to read u8"}},
      MakeSpanU8("\x00\x41\x00\x0b"));

  ExpectReadFailure<DataSegment>(
      {{0, "data segment"}, {5, "Length extends past end: 2 > 0"}},
      MakeSpanU8("\x00\x41\x00\x0b\x02"));
}

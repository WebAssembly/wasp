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

#include "wasp/binary/read/read_element_segment.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ElementSegment) {
  ExpectRead<ElementSegment>(
      ElementSegment{0, MakeConstantExpression("\x41\x01\x0b"), {1, 2, 3}},
      MakeSpanU8("\x00\x41\x01\x0b\x03\x01\x02\x03"));
}

TEST(ReaderTest, ElementSegment_PastEnd) {
  ExpectReadFailure<ElementSegment>(
      {{0, "element segment"}, {0, "table index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {1, "offset"},
                                     {1, "constant expression"},
                                     {1, "opcode"},
                                     {1, "Unable to read u8"}},
                                    MakeSpanU8("\x00"));

  ExpectReadFailure<ElementSegment>({{0, "element segment"},
                                     {4, "initializers"},
                                     {4, "count"},
                                     {4, "Unable to read u8"}},
                                    MakeSpanU8("\x00\x23\x00\x0b"));
}

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

#include "wasp/binary/read/read_br_table_immediate.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, BrTableImmediate) {
  ExpectRead<BrTableImmediate>(BrTableImmediate{{}, 0}, MakeSpanU8("\x00\x00"));
  ExpectRead<BrTableImmediate>(BrTableImmediate{{1, 2}, 3},
                               MakeSpanU8("\x02\x01\x02\x03"));
}

TEST(ReaderTest, BrTableImmediate_PastEnd) {
  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {0, "targets"}, {0, "count"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {1, "default target"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

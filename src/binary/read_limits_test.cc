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

#include "wasp/binary/read/read_limits.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Limits) {
  ExpectRead<Limits>(Limits{129}, MakeSpanU8("\x00\x81\x01"));
  ExpectRead<Limits>(Limits{2, 1000}, MakeSpanU8("\x01\x02\xe8\x07"));
}

TEST(ReaderTest, Limits_BadFlags) {
  ExpectReadFailure<Limits>({{0, "limits"}, {1, "Invalid flags value: 2"}},
                            MakeSpanU8("\x02\x01"));
}

TEST(ReaderTest, Limits_PastEnd) {
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x01\x00"));
}

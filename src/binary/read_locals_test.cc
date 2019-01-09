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

#include "wasp/binary/read/read_locals.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Locals) {
  ExpectRead<Locals>(Locals{2, ValueType::I32}, MakeSpanU8("\x02\x7f"));
  ExpectRead<Locals>(Locals{320, ValueType::F64}, MakeSpanU8("\xc0\x02\x7c"));
}

TEST(ReaderTest, Locals_PastEnd) {
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {0, "count"}, {0, "Unable to read u8"}}, MakeSpanU8(""));
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {2, "type"}, {2, "value type"}, {2, "Unable to read u8"}},
      MakeSpanU8("\xc0\x02"));
}

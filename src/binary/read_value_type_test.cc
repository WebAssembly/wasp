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

#include "wasp/binary/read/read_value_type.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ValueType) {
  ExpectRead<ValueType>(ValueType::I32, MakeSpanU8("\x7f"));
  ExpectRead<ValueType>(ValueType::I64, MakeSpanU8("\x7e"));
  ExpectRead<ValueType>(ValueType::F32, MakeSpanU8("\x7d"));
  ExpectRead<ValueType>(ValueType::F64, MakeSpanU8("\x7c"));
}

TEST(ReaderTest, ValueType_Unknown) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 16"}}, MakeSpanU8("\x10"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 255"}},
      MakeSpanU8("\xff\x7f"));
}

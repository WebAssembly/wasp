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

#include "wasp/binary/read/read_global.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Global) {
  // i32 global with i64.const constant expression. This will fail validation
  // but still can be successfully parsed.
  ExpectRead<Global>(Global{GlobalType{ValueType::I32, Mutability::Var},
                            MakeConstantExpression("\x42\x00\x0b")},
                     MakeSpanU8("\x7f\x01\x42\x00\x0b"));
}

TEST(ReaderTest, Global_PastEnd) {
  ExpectReadFailure<Global>({{0, "global"},
                             {0, "global type"},
                             {0, "value type"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));

  ExpectReadFailure<Global>({{0, "global"},
                             {2, "constant expression"},
                             {2, "opcode"},
                             {2, "Unable to read u8"}},
                            MakeSpanU8("\x7f\x00"));
}

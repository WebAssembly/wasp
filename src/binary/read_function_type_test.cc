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

#include "wasp/binary/read/read_function_type.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, FunctionType) {
  ExpectRead<FunctionType>(FunctionType{{}, {}}, MakeSpanU8("\x00\x00"));
  ExpectRead<FunctionType>(
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}},
      MakeSpanU8("\x02\x7f\x7e\x01\x7c"));
}

TEST(ReaderTest, FunctionType_PastEnd) {
  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {0, "count"},
                                   {0, "Unable to read u8"}},
                                  MakeSpanU8(""));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {1, "Count extends past end: 1 > 0"}},
                                  MakeSpanU8("\x01"));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {1, "count"},
                                   {1, "Unable to read u8"}},
                                  MakeSpanU8("\x00"));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {2, "Count extends past end: 1 > 0"}},
                                  MakeSpanU8("\x00\x01"));
}

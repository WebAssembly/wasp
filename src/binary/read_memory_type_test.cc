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

#include "wasp/binary/read/read_memory_type.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, MemoryType) {
  ExpectRead<MemoryType>(MemoryType{Limits{1}}, MakeSpanU8("\x00\x01"));
  ExpectRead<MemoryType>(MemoryType{Limits{0, 128}},
                         MakeSpanU8("\x01\x00\x80\x01"));
}

TEST(ReaderTest, MemoryType_PastEnd) {
  ExpectReadFailure<MemoryType>({{0, "memory type"},
                                 {0, "limits"},
                                 {0, "flags"},
                                 {0, "Unable to read u8"}},
                                MakeSpanU8(""));
}

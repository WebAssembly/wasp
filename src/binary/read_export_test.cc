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

#include "wasp/binary/read/read_export.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Export) {
  ExpectRead<Export>(Export{ExternalKind::Function, "hi", 3},
                     MakeSpanU8("\x02hi\x00\x03"));
  ExpectRead<Export>(Export{ExternalKind::Table, "", 1000},
                     MakeSpanU8("\x00\x01\xe8\x07"));
  ExpectRead<Export>(Export{ExternalKind::Memory, "mem", 0},
                     MakeSpanU8("\x03mem\x02\x00"));
  ExpectRead<Export>(Export{ExternalKind::Global, "g", 1},
                     MakeSpanU8("\x01g\x03\x01"));
}

TEST(ReaderTest, Export_PastEnd) {
  ExpectReadFailure<Export>(
      {{0, "export"}, {0, "name"}, {0, "length"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<Export>(
      {{0, "export"}, {1, "external kind"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));

  ExpectReadFailure<Export>(
      {{0, "export"}, {2, "index"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));
}

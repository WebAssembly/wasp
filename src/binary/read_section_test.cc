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

#include "wasp/binary/read/read_section.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Section) {
  ExpectRead<Section>(
      Section{KnownSection{SectionId::Type, MakeSpanU8("\x01\x02\x03")}},
      MakeSpanU8("\x01\x03\x01\x02\x03"));

  ExpectRead<Section>(
      Section{CustomSection{"name", MakeSpanU8("\x04\x05\x06")}},
      MakeSpanU8("\x00\x08\x04name\x04\x05\x06"));
}

TEST(ReaderTest, Section_PastEnd) {
  ExpectReadFailure<Section>(
      {{0, "section"}, {0, "section id"}, {0, "u32"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<Section>(
      {{0, "section"}, {1, "length"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x01"));

  ExpectReadFailure<Section>(
      {{0, "section"}, {2, "Length extends past end: 1 > 0"}},
      MakeSpanU8("\x01\x01"));
}


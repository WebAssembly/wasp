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

#include "wasp/binary/read/read_section_id.h"

#include "gtest/gtest.h"

#include "src/binary/read_test_utils.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, SectionId) {
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x00"));
  ExpectRead<SectionId>(SectionId::Type, MakeSpanU8("\x01"));
  ExpectRead<SectionId>(SectionId::Import, MakeSpanU8("\x02"));
  ExpectRead<SectionId>(SectionId::Function, MakeSpanU8("\x03"));
  ExpectRead<SectionId>(SectionId::Table, MakeSpanU8("\x04"));
  ExpectRead<SectionId>(SectionId::Memory, MakeSpanU8("\x05"));
  ExpectRead<SectionId>(SectionId::Global, MakeSpanU8("\x06"));
  ExpectRead<SectionId>(SectionId::Export, MakeSpanU8("\x07"));
  ExpectRead<SectionId>(SectionId::Start, MakeSpanU8("\x08"));
  ExpectRead<SectionId>(SectionId::Element, MakeSpanU8("\x09"));
  ExpectRead<SectionId>(SectionId::Code, MakeSpanU8("\x0a"));
  ExpectRead<SectionId>(SectionId::Data, MakeSpanU8("\x0b"));

  // Overlong encoding.
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x80\x00"));
}

TEST(ReaderTest, SectionId_Unknown) {
  ExpectReadFailure<SectionId>(
      {{0, "section id"}, {1, "Unknown section id: 12"}}, MakeSpanU8("\x0c"));
}

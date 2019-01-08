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

#include "wasp/binary/read/read_name_subsection_id.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;


TEST(ReaderTest, NameSubsectionId) {
  ExpectRead<NameSubsectionId>(NameSubsectionId::ModuleName,
                               MakeSpanU8("\x00"));
  ExpectRead<NameSubsectionId>(NameSubsectionId::FunctionNames,
                               MakeSpanU8("\x01"));
  ExpectRead<NameSubsectionId>(NameSubsectionId::LocalNames,
                               MakeSpanU8("\x02"));
}

TEST(ReaderTest, NameSubsectionId_Unknown) {
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 3"}},
      MakeSpanU8("\x03"));
  ExpectReadFailure<NameSubsectionId>(
      {{0, "name subsection id"}, {1, "Unknown name subsection id: 255"}},
      MakeSpanU8("\xff"));
}

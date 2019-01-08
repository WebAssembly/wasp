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

#include "wasp/binary/read/read_name_subsection.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, NameSubsection) {
  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::ModuleName, MakeSpanU8("\0")},
      MakeSpanU8("\x00\x01\0"));

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::FunctionNames, MakeSpanU8("\0\0")},
      MakeSpanU8("\x01\x02\0\0"));

  ExpectRead<NameSubsection>(
      NameSubsection{NameSubsectionId::LocalNames, MakeSpanU8("\0\0\0")},
      MakeSpanU8("\x02\x03\0\0\0"));
}

TEST(ReaderTest, NameSubsection_BadSubsectionId) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {1, "Unknown name subsection id: 3"}},
                                    MakeSpanU8("\x03"));
}

TEST(ReaderTest, NameSubsection_PastEnd) {
  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {0, "name subsection id"},
                                     {0, "Unable to read u8"}},
                                    MakeSpanU8(""));

  ExpectReadFailure<NameSubsection>({{0, "name subsection"},
                                     {1, "length"},
                                     {1, "Unable to read u8"}},
                                    MakeSpanU8("\x00"));
}

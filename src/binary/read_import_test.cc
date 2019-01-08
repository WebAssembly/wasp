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

#include "wasp/binary/read/read_import.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Import) {
  ExpectRead<Import>(Import{"a", "func", 11u},
                     MakeSpanU8("\x01\x61\x04\x66unc\x00\x0b"));

  ExpectRead<Import>(
      Import{"b", "table", TableType{Limits{1}, ElementType::Funcref}},
      MakeSpanU8("\x01\x62\x05table\x01\x70\x00\x01"));

  ExpectRead<Import>(Import{"c", "memory", MemoryType{Limits{0, 2}}},
                     MakeSpanU8("\x01\x63\x06memory\x02\x01\x00\x02"));

  ExpectRead<Import>(
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}},
      MakeSpanU8("\x01\x64\x06global\x03\x7f\x00"));
}

TEST(ReaderTest, ImportType_PastEnd) {
  ExpectReadFailure<Import>({{0, "import"},
                             {0, "module name"},
                             {0, "length"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));

  ExpectReadFailure<Import>({{0, "import"},
                             {1, "field name"},
                             {1, "length"},
                             {1, "Unable to read u8"}},
                            MakeSpanU8("\x00"));

  ExpectReadFailure<Import>(
      {{0, "import"}, {2, "external kind"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));

  ExpectReadFailure<Import>(
      {{0, "import"}, {3, "function index"}, {3, "Unable to read u8"}},
      MakeSpanU8("\x00\x00\x00"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "table type"},
                             {3, "element type"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x01"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "memory type"},
                             {3, "limits"},
                             {3, "flags"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x02"));

  ExpectReadFailure<Import>({{0, "import"},
                             {3, "global type"},
                             {3, "value type"},
                             {3, "Unable to read u8"}},
                            MakeSpanU8("\x00\x00\x03"));
}

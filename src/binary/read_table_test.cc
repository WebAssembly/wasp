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

#include "wasp/binary/read/read_table.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, Table) {
  ExpectRead<Table>(Table{TableType{Limits{1}, ElementType::Funcref}},
                    MakeSpanU8("\x70\x00\x01"));
}

TEST(ReaderTest, Table_PastEnd) {
  ExpectReadFailure<Table>({{0, "table"},
                            {0, "table type"},
                            {0, "element type"},
                            {0, "Unable to read u8"}},
                           MakeSpanU8(""));
}

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

#include "wasp/binary/read/read_type_entry.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, TypeEntry) {
  ExpectRead<TypeEntry>(TypeEntry{FunctionType{{}, {ValueType::I32}}},
                        MakeSpanU8("\x60\x00\x01\x7f"));
}

TEST(ReaderTest, TypeEntry_BadForm) {
  ExpectReadFailure<TypeEntry>(
      {{0, "type entry"}, {1, "Unknown type form: 64"}}, MakeSpanU8("\x40"));
}

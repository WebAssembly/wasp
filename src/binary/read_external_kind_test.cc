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

#include "wasp/binary/read/read_external_kind.h"

#include "gtest/gtest.h"

#include "src/binary/reader_test_helpers.h"
#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReaderTest, ExternalKind) {
  ExpectRead<ExternalKind>(ExternalKind::Function, MakeSpanU8("\x00"));
  ExpectRead<ExternalKind>(ExternalKind::Table, MakeSpanU8("\x01"));
  ExpectRead<ExternalKind>(ExternalKind::Memory, MakeSpanU8("\x02"));
  ExpectRead<ExternalKind>(ExternalKind::Global, MakeSpanU8("\x03"));
}

TEST(ReaderTest, ExternalKind_Unknown) {
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 4"}},
      MakeSpanU8("\x04"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 132"}},
      MakeSpanU8("\x84\x00"));
}

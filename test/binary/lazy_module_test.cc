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

#include "wasp/binary/lazy_module.h"

#include "gtest/gtest.h"

#include "test/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(LazyModuleTest, Basic) {
  Features features;
  TestErrors errors;
  auto module = ReadModule(
      MakeSpanU8("\0asm\x01\0\0\0"
                 "\x01\x03\0\0\0"         // Invalid type section.
                 "\x01\x05\0\0\0\0\0"     // Another invalid type section.
                 "\x0a\x01\0"             // Code section.
                 "\x00\x06\x03yup\0\0"),  // Custom section "yup"
      features,
      errors);

  EXPECT_EQ(MakeSpanU8("\0asm"), module.magic);
  EXPECT_EQ(MakeSpanU8("\x01\0\0\0"), module.version);

  auto it = module.sections.begin(), end = module.sections.end();

  EXPECT_EQ((Section{KnownSection{SectionId::Type, MakeSpanU8("\0\0\0")}}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((Section{KnownSection{SectionId::Type, MakeSpanU8("\0\0\0\0\0")}}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((Section{KnownSection{SectionId::Code, MakeSpanU8("\0")}}), *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((Section{CustomSection{"yup", MakeSpanU8("\0\0")}}), *it++);
  ASSERT_EQ(end, it);

  ExpectNoErrors(errors);
}

TEST(LazyModuleTest, BadMagic) {
  TestErrors errors;
  auto data = MakeSpanU8("wasm\x01\0\0\0");
  auto module = ReadModule(data, Features{}, errors);

  ExpectError({{0, "magic"},
               {4, R"(Mismatch: expected "\00\61\73\6d", got "\77\61\73\6d")"}},
              errors, data);
}

TEST(LazyModuleTest, Magic_PastEnd) {
  TestErrors errors;
  auto data = MakeSpanU8("\0as");
  auto module = ReadModule(data, Features{}, errors);

  // TODO(binji): This should produce better errors.
  ExpectErrors({{{0, "magic"}, {0, "Unable to read 4 bytes"}},
                {{0, "version"}, {0, "Unable to read 4 bytes"}}},
               errors, data);
}

TEST(LazyModuleTest, BadVersion) {
  TestErrors errors;
  auto data = MakeSpanU8("\0asm\x02\0\0\0");
  auto module = ReadModule(data, Features{}, errors);

  ExpectError({{4, "version"},
               {8, R"(Mismatch: expected "\01\00\00\00", got "\02\00\00\00")"}},
              errors, data);
}

TEST(LazyModuleTest, Version_PastEnd) {
  TestErrors errors;
  auto data = MakeSpanU8("\0asm\x01");
  auto module = ReadModule(data, Features{}, errors);

  ExpectError({{4, "version"}, {4, "Unable to read 4 bytes"}}, errors, data);
}

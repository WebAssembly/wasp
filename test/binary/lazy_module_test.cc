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
#include "test/test_utils.h"
#include "wasp/base/macros.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

TEST(BinaryLazyModuleTest, Basic) {
  Features features;
  TestErrors errors;
  auto module = ReadModule(
      "\0asm\x01\0\0\0"
      "\x01\x03\0\0\0"            // Invalid type section.
      "\x03\x05\0\0\0\0\0"        // Invalid function section.
      "\x0a\x01\0"                // Code section.
      "\x00\x06\x03yup\0\0"_su8,  // Custom section "yup"
      features, errors);

  EXPECT_EQ("\0asm"_su8, module.magic);
  EXPECT_EQ("\x01\0\0\0"_su8, module.version);

  auto it = module.sections.begin(), end = module.sections.end();

  EXPECT_EQ((Section{At{
                "\x01\03\0\0\0"_su8,
                KnownSection{At{"\x01"_su8, SectionId::Type}, "\0\0\0"_su8}}}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((Section{At{"\x03\x05\0\0\0\0\0"_su8,
                        KnownSection{At{"\x03"_su8, SectionId::Function},
                                     "\0\0\0\0\0"_su8}}}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (Section{At{"\x0a\x01\0"_su8,
                  KnownSection{At{"\x0a"_su8, SectionId::Code}, "\0"_su8}}}),
      *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (Section{At{"\x00\x06\x03yup\0\0"_su8,
                  CustomSection{At{"\x03yup"_su8, "yup"_sv}, "\0\0"_su8}}}),
      *it++);
  ASSERT_EQ(end, it);

  ExpectNoErrors(errors);
}

TEST(BinaryLazyModuleTest, BadMagic) {
  TestErrors errors;
  auto data = "wasm\x01\0\0\0"_su8;
  auto module = ReadModule(data, Features{}, errors);
  WASP_USE(module);

  ExpectError({{0, "magic"},
               {0, R"(Mismatch: expected "\00\61\73\6d", got "\77\61\73\6d")"}},
              errors, data);
}

TEST(BinaryLazyModuleTest, Magic_PastEnd) {
  TestErrors errors;
  auto data = "\0as"_su8;
  auto module = ReadModule(data, Features{}, errors);
  WASP_USE(module);

  // TODO(binji): This should produce better errors.
  ExpectErrors({{{0, "magic"}, {0, "Unable to read 4 bytes"}},
                {{0, "version"}, {0, "Unable to read 4 bytes"}}},
               errors, data);
}

TEST(BinaryLazyModuleTest, BadVersion) {
  TestErrors errors;
  auto data = "\0asm\x02\0\0\0"_su8;
  auto module = ReadModule(data, Features{}, errors);
  WASP_USE(module);

  ExpectError({{4, "version"},
               {4, R"(Mismatch: expected "\01\00\00\00", got "\02\00\00\00")"}},
              errors, data);
}

TEST(BinaryLazyModuleTest, Version_PastEnd) {
  TestErrors errors;
  auto data = "\0asm\x01"_su8;
  auto module = ReadModule(data, Features{}, errors);
  WASP_USE(module);

  ExpectError({{4, "version"}, {4, "Unable to read 4 bytes"}}, errors, data);
}

//
// Copyright 2020 WebAssembly Community Group participants
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

#include "wasp/binary/read.h"

#include "gtest/gtest.h"
#include "test/binary/constants.h"
#include "test/binary/test_utils.h"
#include "test/test_utils.h"
#include "wasp/binary/read/context.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;
using namespace ::wasp::binary::test;

class BinaryReadModuleTest : public ::testing::Test {
 protected:
  void OK(const Module& expected, SpanU8 data) {
    auto actual = ReadModule(data, context);
    ExpectNoErrors(errors);
    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(expected, actual.value());
  }

  void Fail(const ExpectedError& error, SpanU8 data) {
    auto actual = ReadModule(data, context);
    EXPECT_FALSE(actual.has_value());
    ExpectError(error, errors, data);
    errors.Clear();
  }

  TestErrors errors;
  Context context{errors};
};

TEST_F(BinaryReadModuleTest, EmptyModule) {
  OK(
      Module{
          // types
          {},
          // imports
          {},
          // functions
          {},
          // tables
          {},
          // memories
          {},
          // globals
          {},
          // events
          {},
          // exports
          {},
          // start
          {},
          // element_segments
          {},
          // data_count
          {},
          // codes
          {},
          // data_segments
          {},
      },
      "\0asm\x01\0\0\0"_su8);
}

TEST_F(BinaryReadModuleTest, SimpleModule) {
  OK(
      Module{
          // types
          {At{"\x60\x00\x01\x7f"_su8,
              DefinedType{At{"\x00\x01\x7f"_su8,
                             FunctionType{{}, {At{"\x7f"_su8, VT_I32}}}}}}},
          // imports
          {},
          // functions
          {At{"\x00"_su8, Function{At{"\x00"_su8, Index{0}}}}},
          // tables
          {},
          // memories
          {},
          // globals
          {},
          // events
          {},
          // exports
          {},
          // start
          {},
          // element_segments
          {},
          // data_count
          {},
          // codes
          {At{"\x04\x00\x41\x2a\x0b"_su8,
              UnpackedCode{
                  {},
                  UnpackedExpression{
                      {At{"\x41\x2a"_su8,
                          Instruction{At{"\x41"_su8, Opcode::I32Const},
                                      At{"\x2a"_su8, s32{42}}}},
                       At{"\x0b"_su8,
                          Instruction{At{"\x0b"_su8, Opcode::End}}}}}}}},
          // data_segments
          {},
      },
      "\0asm\x01\0\0\0"
      // type: (func (result i32))
      "\x01\x05\x01\x60\x00\x01\x7f"
      // func: (func (type 0))
      "\x03\x02\x01\x00"
      // code: (func (type 0) i32.const 42)
      "\x0a\x06\x01\x04\x00\x41\x2a\x0b"_su8);
}

TEST_F(BinaryReadModuleTest, BadMagic) {
  Fail({{0, "module"},
        {0, "magic"},
        {0,
         "Mismatch: expected \"\\00\\61\\73\\6d\", got \"\\00\\41\\53\\4d\""}},
       "\0ASM\x01\0\0\0"_su8);
}

TEST_F(BinaryReadModuleTest, BadVersion) {
  Fail({{0, "module"},
        {4, "version"},
        {4,
         "Mismatch: expected \"\\01\\00\\00\\00\", got \"\\02\\00\\00\\00\""}},
       "\0asm\x02\0\0\0"_su8);
}

TEST_F(BinaryReadModuleTest, BadTypeSection) {
  Fail({{0, "module"}, {10, "count"}, {10, "Unable to read u8"}},
       "\0asm\x01\0\0\0"
       "\x01\x00"_su8  // Empty type section.
  );
}

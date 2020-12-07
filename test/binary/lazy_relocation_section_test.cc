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

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "wasp/binary/linking_section/sections.h"
#include "wasp/binary/read/read_ctx.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

TEST(BinaryRelocationTest, Basic) {
  TestErrors errors;
  ReadCtx ctx{errors};
  auto sec = ReadRelocationSection(
      "\x01\x03"  // Section index = 1, 3 relocations.
      "\x01\x02\x03"
      "\x04\x05\x06\x07"
      "\x08\x09\x0a\x0b"_su8,
      ctx);

  EXPECT_EQ(Index{1}, sec.section_index);
  EXPECT_EQ(Index{3}, sec.count);

  auto it = sec.entries.begin();
  auto end = sec.entries.end();

  EXPECT_EQ((RelocationEntry{At{"\x01"_su8, RelocationType::TableIndexSLEB},
                             At{"\x02"_su8, u32{2}}, At{"\x03"_su8, Index{3}},
                             nullopt}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((RelocationEntry{At{"\x04"_su8, RelocationType::MemoryAddressSLEB},
                             At{"\x05"_su8, u32{5}}, At{"\x06"_su8, Index{6}},
                             At{"\x07"_su8, s32{7}}}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((RelocationEntry{At{"\x08"_su8, RelocationType::FunctionOffsetI32},
                             At{"\x09"_su8, u32{9}}, At{"\x0a"_su8, Index{10}},
                             At{"\x0b"_su8, s32{11}}}),
            *it++);
  ASSERT_EQ(end, it);

  ExpectNoErrors(errors);
}

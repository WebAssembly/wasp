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

#include "gtest/gtest.h"
#include "test/test_utils.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/read/read_ctx.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;

namespace {

template <typename T>
void ExpectSubsection(const std::vector<T>& expected, LazySection<T>& sec) {
  EXPECT_EQ(expected.size(), sec.count);
  size_t i = 0;
  for (const auto& item : sec.sequence) {
    ASSERT_LT(i, expected.size());
    EXPECT_EQ(expected[i++], item);
  }
  EXPECT_EQ(expected.size(), i);
}

}  // namespace

TEST(BinaryLazyNameSectionTest, NameSection) {
  TestErrors errors;
  ReadCtx ctx{errors};
  auto sec = ReadNameSection(
      "\x00\x02\x01m"                              // Module name = "m"
      "\x01\x03\x02\x01g"                          // Function names: 2 => "g"
      "\x02\x0a\x03\x02\x04\x02g4\x05\x02g5"_su8,  // Local names: function 3
                                                   //   4 => "g4", 5 => "g5"
      ctx);

  auto it = sec.begin();

  EXPECT_EQ((At{"\x00\x02\x01m"_su8,
                NameSubsection{At{"\x00"_su8, NameSubsectionId::ModuleName},
                               "\x01m"_su8}}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((At{"\x01\x03\x02\x01g"_su8,
                NameSubsection{At{"\x01"_su8, NameSubsectionId::FunctionNames},
                               "\x02\x01g"_su8}}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((At{"\x02\x0a\x03\x02\x04\x02g4\x05\x02g5"_su8,
                NameSubsection{At{"\x02"_su8, NameSubsectionId::LocalNames},
                               "\x03\x02\x04\x02g4\x05\x02g5"_su8}}),
            *it++);
  ASSERT_EQ(sec.end(), it);

  ExpectNoErrors(errors);
}

TEST(BinaryLazyNameSectionTest, ModuleNameSubsection) {
  TestErrors errors;
  ReadCtx ctx{errors};
  auto name = ReadModuleNameSubsection("\x04name"_su8, ctx);
  EXPECT_EQ("name", name);
  ExpectNoErrors(errors);
}

TEST(BinaryLazyNameSectionTest, FunctionNamesSubsection) {
  TestErrors errors;
  ReadCtx ctx{errors};
  auto sec = ReadFunctionNamesSubsection(
      "\x02\x03\x05three\x05\x04"
      "five"_su8,
      ctx);

  ExpectSubsection(
      {NameAssoc{At{"\x03"_su8, Index{3}}, At{"\x05three"_su8, "three"_sv}},
       NameAssoc{At{"\x05"_su8, Index{5}}, At{"\x04"
                                              "five"_su8,
                                              "five"_sv}}},
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLazyNameSectionTest, LocalNamesSubsection) {
  TestErrors errors;
  ReadCtx ctx{errors};
  auto sec = ReadLocalNamesSubsection(
      "\x02"
      "\x02\x02\x01\x04ichi\x03\x03san"
      "\x04\x01\x05\x05"
      "cinco"_su8,
      ctx);

  ExpectSubsection(
      {IndirectNameAssoc{
           At{"\x02"_su8, Index{2}},
           {At{"\x01\x04ichi"_su8, NameAssoc{At{"\x01"_su8, Index{1}},
                                             At{"\x04ichi"_su8, "ichi"_sv}}},
            At{"\x03\x03san"_su8, NameAssoc{At{"\x03"_su8, Index{3}},
                                            At{"\x03san"_su8, "san"_sv}}}}},
       IndirectNameAssoc{
           At{"\x04"_su8, Index{4}},
           {At{"\x05\x05"
               "cinco"_su8,
               NameAssoc{At{"\x05"_su8, Index{5}}, At{"\x05"
                                                      "cinco"_su8,
                                                      "cinco"_sv}}}}}},
      sec);
  ExpectNoErrors(errors);
}

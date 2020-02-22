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
#include "test/binary/test_utils.h"
#include "wasp/binary/sections_name.h"
#include "wasp/binary/read/context.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

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

TEST(LazyNameSectionTest, NameSection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadNameSection(
      "\x00\x02\x01m"                              // Module name = "m"
      "\x01\x03\x02\x01g"                          // Function names: 2 => "g"
      "\x02\x0a\x03\x02\x04\x02g4\x05\x02g5"_su8,  // Local names: function 3
                                                   //   4 => "g4", 5 => "g5"
      context);

  auto it = sec.begin();

  EXPECT_EQ((NameSubsection{NameSubsectionId::ModuleName, "\x01m"_su8}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((NameSubsection{NameSubsectionId::FunctionNames,
                            "\x02\x01g"_su8}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((NameSubsection{NameSubsectionId::LocalNames,
                            "\x03\x02\x04\x02g4\x05\x02g5"_su8}),
            *it++);
  ASSERT_EQ(sec.end(), it);

  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, ModuleNameSubsection) {
  TestErrors errors;
  Context context{errors};
  auto name = ReadModuleNameSubsection("\x04name"_su8, context);
  EXPECT_EQ("name", name);
  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, FunctionNamesSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadFunctionNamesSubsection(
      "\x02\x03\x05three\x05\x04"
      "five"_su8,
      context);

  ExpectSubsection(
      {
          NameAssoc{3, "three"_sv},
          NameAssoc{5, "five"_sv},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, LocalNamesSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadLocalNamesSubsection(
      "\x02"
      "\x02\x02\x01\x04ichi\x03\x03san"
      "\x04\x01\x05\x05"
      "cinco"_su8,
      context);

  ExpectSubsection(
      {
          IndirectNameAssoc{2,
                            {NameAssoc{1, "ichi"_sv}, NameAssoc{3, "san"_sv}}},
          IndirectNameAssoc{4, {NameAssoc{5, "cinco"_sv}}},
      },
      sec);
  ExpectNoErrors(errors);
}

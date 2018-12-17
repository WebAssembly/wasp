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

#include "src/binary/lazy_name_section.h"

#include "gtest/gtest.h"

#include "src/binary/test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

namespace {

template <typename T, typename Errors>
void ExpectSubsection(const std::vector<T>& expected,
                      LazySection<T, Errors>& sec) {
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
  auto sec = ReadNameSection(
      MakeSpanU8(
          "\x00\x02\x01m"                           // Module name = "m"
          "\x01\x03\x02\x01g"                       // Function names: 2 => "g"
          "\x02\x0a\x03\x02\x04\x02g4\x05\x02g5"),  // Local names: function 3
                                                    //   4 => "g4", 5 => "g5"
      errors);

  auto it = sec.begin();

  EXPECT_EQ((NameSubsection{NameSubsectionId::ModuleName, MakeSpanU8("\x01m")}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((NameSubsection{NameSubsectionId::FunctionNames,
                            MakeSpanU8("\x02\x01g")}),
            *it++);
  ASSERT_NE(sec.end(), it);

  EXPECT_EQ((NameSubsection{NameSubsectionId::LocalNames,
                            MakeSpanU8("\x03\x02\x04\x02g4\x05\x02g5")}),
            *it++);
  ASSERT_EQ(sec.end(), it);

  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, ModuleNameSubsection) {
  TestErrors errors;
  auto name = ReadModuleNameSubsection(MakeSpanU8("\x04name"), errors);
  EXPECT_EQ("name", name);
  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, FunctionNamesSubsection) {
  TestErrors errors;
  auto sec = ReadFunctionNamesSubsection(MakeSpanU8("\x02\x03\x05three\x05\x04"
                                                    "five"),
                                         errors);

  ExpectSubsection(
      {
          NameAssoc{3, "three"},
          NameAssoc{5, "five"},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LazyNameSectionTest, LocalNamesSubsection) {
  TestErrors errors;
  auto sec =
      ReadLocalNamesSubsection(MakeSpanU8("\x02"
                                          "\x02\x02\x01\x04ichi\x03\x03san"
                                          "\x04\x01\x05\x05"
                                          "cinco"),
                               errors);

  ExpectSubsection(
      {
          IndirectNameAssoc{2, {{1, "ichi"}, {3, "san"}}},
          IndirectNameAssoc{4, {{5, "cinco"}}},
      },
      sec);
  ExpectNoErrors(errors);
}

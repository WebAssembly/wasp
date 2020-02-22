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
#include "test/binary/test_utils.h"
#include "wasp/binary/sections_linking.h"
#include "wasp/binary/types_linking.h"
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

TEST(LinkingSectionTest, LinkingSection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadLinkingSection(
      "\x02"                // Linking version.
      "\x05\x05zzzzz"       // Segment info
      "\x06\x05zzzzz"       // Init functions
      "\x07\x05zzzzz"       // Comdat info
      "\x08\x05zzzzz"_su8,  // Symbol table
      context);

  auto it = sec.subsections.begin();
  auto end = sec.subsections.end();

  EXPECT_EQ((LinkingSubsection{LinkingSubsectionId::SegmentInfo, "zzzzz"_su8}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (LinkingSubsection{LinkingSubsectionId::InitFunctions, "zzzzz"_su8}),
      *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((LinkingSubsection{LinkingSubsectionId::ComdatInfo, "zzzzz"_su8}),
            *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ((LinkingSubsection{LinkingSubsectionId::SymbolTable, "zzzzz"_su8}),
            *it++);
  ASSERT_EQ(end, it);

  ExpectNoErrors(errors);
}

TEST(LinkingSectionTest, SegmentInfoSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadSegmentInfoSubsection(
      "\x03"
      "\x01X\x01\x02"
      "\x01Y\x03\x04"
      "\x01Z\x05\x06"_su8,
      context);

  ExpectSubsection(
      {
          SegmentInfo{"X"_sv, 1, 2},
          SegmentInfo{"Y"_sv, 3, 4},
          SegmentInfo{"Z"_sv, 5, 6},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LinkingSectionTest, InitFunctionsSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadInitFunctionsSubsection(
      "\x02"
      "\x01\x02"
      "\x03\x04"_su8,
      context);

  ExpectSubsection(
      {
          InitFunction{1, 2},
          InitFunction{3, 4},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LinkingSectionTest, ComdatSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadComdatSubsection(
      "\x02"
      "\x01X\0\x01\x03\x04"
      "\x01Y\0\x00"_su8,
      context);

  ExpectSubsection(
      {
          Comdat{"X"_sv, 0, {ComdatSymbol{ComdatSymbolKind::Event, 4}}},
          Comdat{"Y"_sv, 0, {}},
      },
      sec);
  ExpectNoErrors(errors);
}

TEST(LinkingSectionTest, SymbolTableSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadSymbolTableSubsection(
      "\x03"
      "\x00\x40\x00\x03YYY"
      "\x01\x00\x03ZZZ\x00\x00\x00"
      "\x03\x00\x00"_su8,
      context);

  using SI = SymbolInfo;
  using F = SymbolInfo::Flags;

  ExpectSubsection(
      {
          SymbolInfo{F{F::Binding::Global, F::Visibility::Default,
                       F::Undefined::No, F::ExplicitName::Yes},
                     SI::Base{SymbolInfoKind::Function, 0, "YYY"_sv}},

          SymbolInfo{F{F::Binding::Global, F::Visibility::Default,
                       F::Undefined::No, F::ExplicitName::No},
                     SI::Data{"ZZZ"_sv, SI::Data::Defined{0, 0, 0}}},

          SymbolInfo{F{F::Binding::Global, F::Visibility::Default,
                       F::Undefined::No, F::ExplicitName::No},
                     SI::Section{0}},
      },
      sec);
  ExpectNoErrors(errors);
}

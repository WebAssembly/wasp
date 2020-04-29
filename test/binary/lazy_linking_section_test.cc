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
#include "wasp/binary/linking_section/types.h"
#include "wasp/binary/read/context.h"

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

TEST(BinaryLinkingSectionTest, LinkingSection) {
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

  EXPECT_EQ(
      (LinkingSubsection{MakeAt("\x05"_su8, LinkingSubsectionId::SegmentInfo),
                         "zzzzz"_su8}),
      *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (LinkingSubsection{MakeAt("\x06"_su8, LinkingSubsectionId::InitFunctions),
                         "zzzzz"_su8}),
      *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (LinkingSubsection{MakeAt("\x07"_su8, LinkingSubsectionId::ComdatInfo),
                         "zzzzz"_su8}),
      *it++);
  ASSERT_NE(end, it);

  EXPECT_EQ(
      (LinkingSubsection{MakeAt("\x08"_su8, LinkingSubsectionId::SymbolTable),
                         "zzzzz"_su8}),
      *it++);
  ASSERT_EQ(end, it);

  ExpectNoErrors(errors);
}

TEST(BinaryLinkingSectionTest, SegmentInfoSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadSegmentInfoSubsection(
      "\x03"
      "\x01X\x01\x02"
      "\x01Y\x03\x04"
      "\x01Z\x05\x06"_su8,
      context);

  ExpectSubsection(
      {SegmentInfo{MakeAt("\x01X"_su8, "X"_sv), MakeAt("\x01"_su8, u32{1}),
                   MakeAt("\x02"_su8, u32{2})},
       SegmentInfo{MakeAt("\x01Y"_su8, "Y"_sv), MakeAt("\x03"_su8, u32{3}),
                   MakeAt("\x04"_su8, u32{4})},
       SegmentInfo{MakeAt("\x01Z"_su8, "Z"_sv), MakeAt("\x05"_su8, u32{5}),
                   MakeAt("\x06"_su8, u32{6})}},
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLinkingSectionTest, InitFunctionsSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadInitFunctionsSubsection(
      "\x02"
      "\x01\x02"
      "\x03\x04"_su8,
      context);

  ExpectSubsection(
      {InitFunction{MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2})},
       InitFunction{MakeAt("\x03"_su8, u32{3}), MakeAt("\x04"_su8, Index{4})}},
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLinkingSectionTest, ComdatSubsection) {
  TestErrors errors;
  Context context{errors};
  auto sec = ReadComdatSubsection(
      "\x02"
      "\x01X\0\x01\x03\x04"
      "\x01Y\0\x00"_su8,
      context);

  ExpectSubsection(
      {Comdat{MakeAt("\x01X"_su8, "X"_sv),
              MakeAt("\0"_su8, u32{0}),
              {MakeAt("\x03\x04"_su8,
                      ComdatSymbol{MakeAt("\x03"_su8, ComdatSymbolKind::Event),
                                   MakeAt("\x04"_su8, Index{4})})}},
       Comdat{MakeAt("\x01Y"_su8, "Y"_sv), MakeAt("\0"_su8, u32{0}), {}}},
      sec);
  ExpectNoErrors(errors);
}

TEST(BinaryLinkingSectionTest, SymbolTableSubsection) {
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
      {SymbolInfo{
           MakeAt("\x40"_su8, F{MakeAt("\x40"_su8, F::Binding::Global),
                                MakeAt("\x40"_su8, F::Visibility::Default),
                                MakeAt("\x40"_su8, F::Undefined::No),
                                MakeAt("\x40"_su8, F::ExplicitName::Yes)}),
           MakeAt("\x00\x03YYY"_su8,
                  SI::Base{MakeAt("\x00"_su8, SymbolInfoKind::Function),
                           MakeAt("\x00"_su8, Index{0}),
                           MakeAt("\x03YYY"_su8, "YYY"_sv)})},
       SymbolInfo{
           MakeAt("\x00"_su8, F{MakeAt("\x00"_su8, F::Binding::Global),
                                MakeAt("\x00"_su8, F::Visibility::Default),
                                MakeAt("\x00"_su8, F::Undefined::No),
                                MakeAt("\x00"_su8, F::ExplicitName::No)}),
           MakeAt("\x03ZZZ\x00\x00\x00"_su8,
                  SI::Data{MakeAt("\x03ZZZ"_su8, "ZZZ"_sv),
                           SI::Data::Defined{
                               MakeAt("\x00"_su8, Index{0}),
                               MakeAt("\x00"_su8, u32{0}),
                               MakeAt("\x00"_su8, u32{0}),
                           }})},
       SymbolInfo{
           MakeAt("\x00"_su8, F{MakeAt("\x00"_su8, F::Binding::Global),
                                MakeAt("\x00"_su8, F::Visibility::Default),
                                MakeAt("\x00"_su8, F::Undefined::No),
                                MakeAt("\x00"_su8, F::ExplicitName::No)}),
           SI::Section{MakeAt("\x00"_su8, u32{0})}}},
      sec);
  ExpectNoErrors(errors);
}

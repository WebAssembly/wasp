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
#include "test/test_utils.h"
#include "wasp/binary/linking_section/encoding.h"
#include "wasp/binary/linking_section/read.h"
#include "wasp/binary/read/context.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::test;
using namespace ::wasp::binary::test;

// TODO: share code with read_test.cc
class BinaryReadLinkingTest : public ::testing::Test {
 protected:
  template <typename Func, typename T, typename... Args>
  void OK(Func&& func, const T& expected, SpanU8 data, Args&&... args) {
    auto actual = func(&data, context, std::forward<Args>(args)...);
    ExpectNoErrors(errors);
    EXPECT_EQ(0u, data.size());
    EXPECT_NE(nullptr, actual->loc().data());
    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(expected, **actual);
  }

  TestErrors errors;
  Context context{errors};
};

TEST_F(BinaryReadLinkingTest, Comdat) {
  OK(Read<Comdat>,
     Comdat{At{"\x04name"_su8, "name"_sv},
            At{"\x00"_su8, u32{0}},
            {At{"\x00\x02"_su8,
                ComdatSymbol{At{"\x00"_su8, ComdatSymbolKind::Data},
                             At{"\x02"_su8, Index{2}}}},
             At{"\x01\x03"_su8,
                ComdatSymbol{At{"\x01"_su8, ComdatSymbolKind::Function},
                             At{"\x03"_su8, Index{3}}}}}},
     "\x04name\x00\x02\x00\x02\x01\x03"_su8);
}

TEST_F(BinaryReadLinkingTest, ComdatSymbol) {
  OK(Read<ComdatSymbol>,
     ComdatSymbol{At{"\x00"_su8, ComdatSymbolKind::Data},
                  At{"\x00"_su8, Index{0}}},
     "\x00\x00"_su8);
}

TEST_F(BinaryReadLinkingTest, ComdatSymbolKind) {
  OK(Read<ComdatSymbolKind>, ComdatSymbolKind::Data, "\x00"_su8);
  OK(Read<ComdatSymbolKind>, ComdatSymbolKind::Function, "\x01"_su8);
  OK(Read<ComdatSymbolKind>, ComdatSymbolKind::Global, "\x02"_su8);
  OK(Read<ComdatSymbolKind>, ComdatSymbolKind::Event, "\x03"_su8);
}

TEST_F(BinaryReadLinkingTest, InitFunction) {
  OK(Read<InitFunction>,
     InitFunction{At{"\x0d"_su8, u32{13}}, At{"\x0f"_su8, Index{15}}},
     "\x0d\x0f"_su8);
}

TEST_F(BinaryReadLinkingTest, LinkingSubsection) {
  OK(Read<LinkingSubsection>,
     LinkingSubsection{At{"\x05"_su8, LinkingSubsectionId::SegmentInfo},
                       At{"\x03xyz"_su8, "xyz"_su8}},
     "\x05\x03xyz"_su8);
}

TEST_F(BinaryReadLinkingTest, LinkingSubsectionId) {
  OK(Read<LinkingSubsectionId>, LinkingSubsectionId::SegmentInfo, "\x05"_su8);
  OK(Read<LinkingSubsectionId>, LinkingSubsectionId::InitFunctions, "\x06"_su8);
  OK(Read<LinkingSubsectionId>, LinkingSubsectionId::ComdatInfo, "\x07"_su8);
  OK(Read<LinkingSubsectionId>, LinkingSubsectionId::SymbolTable, "\x08"_su8);
}

TEST_F(BinaryReadLinkingTest, RelocationEntry) {
  // Relocation types without addend.
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x00"_su8, RelocationType::FunctionIndexLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x00\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x01"_su8, RelocationType::TableIndexSLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x01\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x02"_su8, RelocationType::TableIndexI32},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x02\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x06"_su8, RelocationType::TypeIndexLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x06\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x07"_su8, RelocationType::GlobalIndexLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x07\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x0a"_su8, RelocationType::EventIndexLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x0a\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x0b"_su8, RelocationType::MemoryAddressRelSLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x0b\x01\x02"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x0c"_su8, RelocationType::TableIndexRelSLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}}, nullopt},
     "\x0c\x01\x02"_su8);

  // Relocation types with addend.
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x03"_su8, RelocationType::MemoryAddressLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}},
                     At{"\x03"_su8, s32{3}}},
     "\x03\x01\x02\x03"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x04"_su8, RelocationType::MemoryAddressSLEB},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}},
                     At{"\x03"_su8, s32{3}}},
     "\x04\x01\x02\x03"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x05"_su8, RelocationType::MemoryAddressI32},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}},
                     At{"\x03"_su8, s32{3}}},
     "\x05\x01\x02\x03"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x08"_su8, RelocationType::FunctionOffsetI32},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}},
                     At{"\x03"_su8, s32{3}}},
     "\x08\x01\x02\x03"_su8);
  OK(Read<RelocationEntry>,
     RelocationEntry{At{"\x09"_su8, RelocationType::SectionOffsetI32},
                     At{"\x01"_su8, u32{1}}, At{"\x02"_su8, Index{2}},
                     At{"\x03"_su8, s32{3}}},
     "\x09\x01\x02\x03"_su8);
}

TEST_F(BinaryReadLinkingTest, RelocationType) {
  OK(Read<RelocationType>, RelocationType::FunctionIndexLEB, "\x00"_su8);
  OK(Read<RelocationType>, RelocationType::TableIndexSLEB, "\x01"_su8);
  OK(Read<RelocationType>, RelocationType::TableIndexI32, "\x02"_su8);
  OK(Read<RelocationType>, RelocationType::MemoryAddressLEB, "\x03"_su8);
  OK(Read<RelocationType>, RelocationType::MemoryAddressSLEB, "\x04"_su8);
  OK(Read<RelocationType>, RelocationType::MemoryAddressI32, "\x05"_su8);
  OK(Read<RelocationType>, RelocationType::TypeIndexLEB, "\x06"_su8);
  OK(Read<RelocationType>, RelocationType::GlobalIndexLEB, "\x07"_su8);
  OK(Read<RelocationType>, RelocationType::FunctionOffsetI32, "\x08"_su8);
  OK(Read<RelocationType>, RelocationType::SectionOffsetI32, "\x09"_su8);
  OK(Read<RelocationType>, RelocationType::EventIndexLEB, "\x0a"_su8);
  OK(Read<RelocationType>, RelocationType::MemoryAddressRelSLEB, "\x0b"_su8);
  OK(Read<RelocationType>, RelocationType::TableIndexRelSLEB, "\x0c"_su8);
}

TEST_F(BinaryReadLinkingTest, ReadSegmentInfo) {
  OK(Read<SegmentInfo>,
     SegmentInfo{At{"\x04name"_su8, "name"_sv}, At{"\x01"_su8, u32{1}},
                 At{"\x02"_su8, u32{2}}},
     "\x04name\x01\x02"_su8);
}

namespace {

// Flags = 0.
const SymbolInfo::Flags zero_flags{
    SymbolInfo::Flags::Binding::Global, SymbolInfo::Flags::Visibility::Default,
    SymbolInfo::Flags::Undefined::No, SymbolInfo::Flags::ExplicitName::No};

// Flags = 0x10.
const SymbolInfo::Flags undefined_flags{
    SymbolInfo::Flags::Binding::Global, SymbolInfo::Flags::Visibility::Default,
    SymbolInfo::Flags::Undefined::Yes, SymbolInfo::Flags::ExplicitName::No};

// Flags = 0x40.
const SymbolInfo::Flags explicit_name_flags{
    SymbolInfo::Flags::Binding::Global, SymbolInfo::Flags::Visibility::Default,
    SymbolInfo::Flags::Undefined::No, SymbolInfo::Flags::ExplicitName::Yes};

}  // namespace

TEST_F(BinaryReadLinkingTest, SymbolInfo_Flags) {
  using SI = SymbolInfo;
  using F = SI::Flags;
  EXPECT_EQ((optional<F>{{F::Binding::Global, F::Visibility::Default,
                          F::Undefined::No, F::ExplicitName::No}}),
            binary::encoding::SymbolInfoFlags::Decode(0x00));

  EXPECT_EQ((optional<F>{{F::Binding::Weak, F::Visibility::Default,
                          F::Undefined::No, F::ExplicitName::No}}),
            binary::encoding::SymbolInfoFlags::Decode(0x01));

  EXPECT_EQ((optional<F>{{F::Binding::Local, F::Visibility::Default,
                          F::Undefined::No, F::ExplicitName::No}}),
            binary::encoding::SymbolInfoFlags::Decode(0x02));

  EXPECT_EQ((optional<F>{{F::Binding::Global, F::Visibility::Hidden,
                          F::Undefined::No, F::ExplicitName::No}}),
            binary::encoding::SymbolInfoFlags::Decode(0x04));

  EXPECT_EQ((optional<F>{{F::Binding::Global, F::Visibility::Default,
                          F::Undefined::Yes, F::ExplicitName::No}}),
            binary::encoding::SymbolInfoFlags::Decode(0x10));

  EXPECT_EQ((optional<F>{{F::Binding::Global, F::Visibility::Default,
                          F::Undefined::No, F::ExplicitName::Yes}}),
            binary::encoding::SymbolInfoFlags::Decode(0x40));
}

TEST_F(BinaryReadLinkingTest, SymbolInfo_Function) {
  using SI = SymbolInfo;
  OK(Read<SI>,
     SI{At{"\x10"_su8, undefined_flags},
        SI::Base{At{"\x00"_su8, SymbolInfoKind::Function},
                 At{"\x00"_su8, Index{0}}, nullopt}},
     "\x00\x10\x00"_su8);

  OK(Read<SI>,
     SI{At{"\x40"_su8, explicit_name_flags},
        SI::Base{At{"\x00"_su8, SymbolInfoKind::Function},
                 At{"\x00"_su8, Index{0}}, At{"\x04name"_su8, "name"_sv}}},
     "\x00\x40\x00\x04name"_su8);
}

TEST_F(BinaryReadLinkingTest, SymbolInfo_Data) {
  using SI = SymbolInfo;
  OK(Read<SI>,
     SI{At{"\x00"_su8, zero_flags},
        SI::Data{
            At{"\x04name"_su8, "name"_sv},
            SI::Data::Defined{At{"\x00"_su8, Index{0}}, At{"\x00"_su8, u32{0}},
                              At{"\x00"_su8, u32{0}}}}},
     "\x01\x00\x04name\x00\x00\x00"_su8);

  OK(Read<SI>,
     SI{At{"\x10"_su8, undefined_flags},
        SI::Data{At{"\x04name"_su8, "name"_sv}, nullopt}},
     "\x01\x10\x04name"_su8);
}

TEST_F(BinaryReadLinkingTest, SymbolInfo_Global) {
  using SI = SymbolInfo;
  OK(Read<SI>,
     SI{At{"\x10"_su8, undefined_flags},
        SI::Base{At{"\x02"_su8, SymbolInfoKind::Global},
                 At{"\x00"_su8, Index{0}}, nullopt}},
     "\x02\x10\x00"_su8);

  OK(Read<SI>,
     SI{At{"\x40"_su8, explicit_name_flags},
        SI::Base{At{"\x02"_su8, SymbolInfoKind::Global},
                 At{"\x00"_su8, Index{0}}, At{"\x04name"_su8, "name"_sv}}},
     "\x02\x40\x00\x04name"_su8);
}

TEST_F(BinaryReadLinkingTest, SymbolInfo_Section) {
  using SI = SymbolInfo;
  OK(Read<SI>,
     SI{At{"\x00"_su8, zero_flags}, SI::Section{At{"\x00"_su8, u32{0}}}},
     "\x03\x00\x00"_su8);
}

TEST_F(BinaryReadLinkingTest, SymbolInfo_Event) {
  using SI = SymbolInfo;
  OK(Read<SI>,
     SI{At{"\x10"_su8, undefined_flags},
        SI::Base{At{"\x04"_su8, SymbolInfoKind::Event},
                 At{"\x00"_su8, Index{0}}, nullopt}},
     "\x04\x10\x00"_su8);

  OK(Read<SI>,
     SI{At{"\x40"_su8, explicit_name_flags},
        SI::Base{At{"\x04"_su8, SymbolInfoKind::Event},
                 At{"\x00"_su8, Index{0}}, At{"\x04name"_su8, "name"_sv}}},
     "\x04\x40\x00\x04name"_su8);
}

TEST_F(BinaryReadLinkingTest, SymbolInfoKind) {
  OK(Read<SymbolInfoKind>, SymbolInfoKind::Function, "\x00"_su8);
  OK(Read<SymbolInfoKind>, SymbolInfoKind::Data, "\x01"_su8);
  OK(Read<SymbolInfoKind>, SymbolInfoKind::Global, "\x02"_su8);
  OK(Read<SymbolInfoKind>, SymbolInfoKind::Section, "\x03"_su8);
  OK(Read<SymbolInfoKind>, SymbolInfoKind::Event, "\x04"_su8);
}

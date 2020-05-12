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
#include "test/binary/read_test_utils.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/linking_section/encoding.h"
#include "wasp/binary/linking_section/read.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(BinaryReadLinkingTest, Comdat) {
  ExpectRead<Comdat>(
      Comdat{
          MakeAt("\x04name"_su8, "name"_sv),
          MakeAt("\x00"_su8, u32{0}),
          {MakeAt("\x00\x02"_su8,
                  ComdatSymbol{MakeAt("\x00"_su8, ComdatSymbolKind::Data),
                               MakeAt("\x02"_su8, Index{2})}),
           MakeAt("\x01\x03"_su8,
                  ComdatSymbol{MakeAt("\x01"_su8, ComdatSymbolKind::Function),
                               MakeAt("\x03"_su8, Index{3})})}},
      "\x04name\x00\x02\x00\x02\x01\x03"_su8);
}

TEST(BinaryReadLinkingTest, ComdatSymbol) {
  ExpectRead<ComdatSymbol>(
      ComdatSymbol{MakeAt("\x00"_su8, ComdatSymbolKind::Data),
                   MakeAt("\x00"_su8, Index{0})},
      "\x00\x00"_su8);
}

TEST(BinaryReadLinkingTest, ComdatSymbolKind) {
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Data, "\x00"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Function, "\x01"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Global, "\x02"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Event, "\x03"_su8);
}

TEST(BinaryReadLinkingTest, InitFunction) {
  ExpectRead<InitFunction>(
      InitFunction{MakeAt("\x0d"_su8, u32{13}), MakeAt("\x0f"_su8, Index{15})},
      "\x0d\x0f"_su8);
}

TEST(BinaryReadLinkingTest, LinkingSubsection) {
  ExpectRead<LinkingSubsection>(
      LinkingSubsection{MakeAt("\x05"_su8, LinkingSubsectionId::SegmentInfo),
                        MakeAt("\x03xyz"_su8, "xyz"_su8)},
      "\x05\x03xyz"_su8);
}

TEST(BinaryReadLinkingTest, LinkingSubsectionId) {
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::SegmentInfo, "\x05"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::InitFunctions,
                                  "\x06"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::ComdatInfo, "\x07"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::SymbolTable, "\x08"_su8);
}

TEST(BinaryReadLinkingTest, RelocationEntry) {
  // Relocation types without addend.
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x00"_su8, RelocationType::FunctionIndexLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x00\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x01"_su8, RelocationType::TableIndexSLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x01\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x02"_su8, RelocationType::TableIndexI32),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x02\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x06"_su8, RelocationType::TypeIndexLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x06\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x07"_su8, RelocationType::GlobalIndexLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x07\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x0a"_su8, RelocationType::EventIndexLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x0a\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x0b"_su8, RelocationType::MemoryAddressRelSLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x0b\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x0c"_su8, RelocationType::TableIndexRelSLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      nullopt},
      "\x0c\x01\x02"_su8);

  // Relocation types with addend.
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x03"_su8, RelocationType::MemoryAddressLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      MakeAt("\x03"_su8, s32{3})},
      "\x03\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x04"_su8, RelocationType::MemoryAddressSLEB),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      MakeAt("\x03"_su8, s32{3})},
      "\x04\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x05"_su8, RelocationType::MemoryAddressI32),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      MakeAt("\x03"_su8, s32{3})},
      "\x05\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x08"_su8, RelocationType::FunctionOffsetI32),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      MakeAt("\x03"_su8, s32{3})},
      "\x08\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{MakeAt("\x09"_su8, RelocationType::SectionOffsetI32),
                      MakeAt("\x01"_su8, u32{1}), MakeAt("\x02"_su8, Index{2}),
                      MakeAt("\x03"_su8, s32{3})},
      "\x09\x01\x02\x03"_su8);
}

TEST(BinaryReadLinkingTest, RelocationType) {
  ExpectRead<RelocationType>(RelocationType::FunctionIndexLEB, "\x00"_su8);
  ExpectRead<RelocationType>(RelocationType::TableIndexSLEB, "\x01"_su8);
  ExpectRead<RelocationType>(RelocationType::TableIndexI32, "\x02"_su8);
  ExpectRead<RelocationType>(RelocationType::MemoryAddressLEB, "\x03"_su8);
  ExpectRead<RelocationType>(RelocationType::MemoryAddressSLEB, "\x04"_su8);
  ExpectRead<RelocationType>(RelocationType::MemoryAddressI32, "\x05"_su8);
  ExpectRead<RelocationType>(RelocationType::TypeIndexLEB, "\x06"_su8);
  ExpectRead<RelocationType>(RelocationType::GlobalIndexLEB, "\x07"_su8);
  ExpectRead<RelocationType>(RelocationType::FunctionOffsetI32, "\x08"_su8);
  ExpectRead<RelocationType>(RelocationType::SectionOffsetI32, "\x09"_su8);
  ExpectRead<RelocationType>(RelocationType::EventIndexLEB, "\x0a"_su8);
  ExpectRead<RelocationType>(RelocationType::MemoryAddressRelSLEB, "\x0b"_su8);
  ExpectRead<RelocationType>(RelocationType::TableIndexRelSLEB, "\x0c"_su8);
}

TEST(BinaryReadLinkingTest, ReadSegmentInfo) {
  ExpectRead<SegmentInfo>(
      SegmentInfo{MakeAt("\x04name"_su8, "name"_sv), MakeAt("\x01"_su8, u32{1}),
                  MakeAt("\x02"_su8, u32{2})},
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

TEST(BinaryReadLinkingTest, SymbolInfo_Flags) {
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

TEST(BinaryReadLinkingTest, SymbolInfo_Function) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{MakeAt("\x10"_su8, undefined_flags),
                    SI::Base{MakeAt("\x00"_su8, SymbolInfoKind::Function),
                             MakeAt("\x00"_su8, Index{0}), nullopt}},
                 "\x00\x10\x00"_su8);

  ExpectRead<SI>(SI{MakeAt("\x40"_su8, explicit_name_flags),
                    SI::Base{MakeAt("\x00"_su8, SymbolInfoKind::Function),
                             MakeAt("\x00"_su8, Index{0}),
                             MakeAt("\x04name"_su8, "name"_sv)}},
                 "\x00\x40\x00\x04name"_su8);
}

TEST(BinaryReadLinkingTest, SymbolInfo_Data) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{MakeAt("\x00"_su8, zero_flags),
                    SI::Data{MakeAt("\x04name"_su8, "name"_sv),
                             SI::Data::Defined{MakeAt("\x00"_su8, Index{0}),
                                               MakeAt("\x00"_su8, u32{0}),
                                               MakeAt("\x00"_su8, u32{0})}}},
                 "\x01\x00\x04name\x00\x00\x00"_su8);

  ExpectRead<SI>(SI{MakeAt("\x10"_su8, undefined_flags),
                    SI::Data{MakeAt("\x04name"_su8, "name"_sv), nullopt}},
                 "\x01\x10\x04name"_su8);
}

TEST(BinaryReadLinkingTest, SymbolInfo_Global) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{MakeAt("\x10"_su8, undefined_flags),
                    SI::Base{MakeAt("\x02"_su8, SymbolInfoKind::Global),
                             MakeAt("\x00"_su8, Index{0}), nullopt}},
                 "\x02\x10\x00"_su8);

  ExpectRead<SI>(SI{MakeAt("\x40"_su8, explicit_name_flags),
                    SI::Base{MakeAt("\x02"_su8, SymbolInfoKind::Global),
                             MakeAt("\x00"_su8, Index{0}),
                             MakeAt("\x04name"_su8, "name"_sv)}},
                 "\x02\x40\x00\x04name"_su8);
}

TEST(BinaryReadLinkingTest, SymbolInfo_Section) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{MakeAt("\x00"_su8, zero_flags),
                    SI::Section{MakeAt("\x00"_su8, u32{0})}},
                 "\x03\x00\x00"_su8);
}

TEST(BinaryReadLinkingTest, SymbolInfo_Event) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{MakeAt("\x10"_su8, undefined_flags),
                    SI::Base{MakeAt("\x04"_su8, SymbolInfoKind::Event),
                             MakeAt("\x00"_su8, Index{0}), nullopt}},
                 "\x04\x10\x00"_su8);

  ExpectRead<SI>(SI{MakeAt("\x40"_su8, explicit_name_flags),
                    SI::Base{MakeAt("\x04"_su8, SymbolInfoKind::Event),
                             MakeAt("\x00"_su8, Index{0}),
                             MakeAt("\x04name"_su8, "name"_sv)}},
                 "\x04\x40\x00\x04name"_su8);
}

TEST(BinaryReadLinkingTest, SymbolInfoKind) {
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Function, "\x00"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Data, "\x01"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Global, "\x02"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Section, "\x03"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Event, "\x04"_su8);
}

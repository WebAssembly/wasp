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

#include "wasp/binary/read_linking.h"

#include "gtest/gtest.h"
#include "test/binary/read_test_utils.h"
#include "test/binary/test_utils.h"
#include "wasp/binary/encoding/symbol_info_flags_encoding.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(ReadLinkingTest, Comdat) {
  ExpectRead<Comdat>(Comdat{"name"_sv,
                            0,
                            {ComdatSymbol{ComdatSymbolKind::Data, 2},
                             ComdatSymbol{ComdatSymbolKind::Function, 3}}},
                     "\x04name\x00"
                     "\x02\x00\x02\x01\x03"_su8);
}

TEST(ReadLinkingTest, ComdatSymbol) {
  ExpectRead<ComdatSymbol>(ComdatSymbol{ComdatSymbolKind::Data, 0},
                           "\x00\x00"_su8);
}

TEST(ReadLinkingTest, ComdatSymbolKind) {
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Data, "\x00"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Function, "\x01"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Global, "\x02"_su8);
  ExpectRead<ComdatSymbolKind>(ComdatSymbolKind::Event, "\x03"_su8);
}

TEST(ReadLinkingTest, InitFunction) {
  ExpectRead<InitFunction>(InitFunction{13, 15}, "\x0d\x0f"_su8);
}

TEST(ReadLinkingTest, LinkingSubsection) {
  ExpectRead<LinkingSubsection>(
      LinkingSubsection{LinkingSubsectionId::SegmentInfo, "xyz"_su8},
      "\x05\x03xyz"_su8);
}

TEST(ReadLinkingTest, LinkingSubsectionId) {
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::SegmentInfo, "\x05"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::InitFunctions,
                                  "\x06"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::ComdatInfo, "\x07"_su8);
  ExpectRead<LinkingSubsectionId>(LinkingSubsectionId::SymbolTable, "\x08"_su8);
}

TEST(ReadLinkingTest, RelocationEntry) {
  // Relocation types without addend.
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::FunctionIndexLEB, 1, 2, nullopt},
      "\x00\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::TableIndexSLEB, 1, 2, nullopt},
      "\x01\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::TableIndexI32, 1, 2, nullopt},
      "\x02\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::TypeIndexLEB, 1, 2, nullopt},
      "\x06\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::GlobalIndexLEB, 1, 2, nullopt},
      "\x07\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::EventIndexLEB, 1, 2, nullopt},
      "\x0a\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::MemoryAddressRelSLEB, 1, 2, nullopt},
      "\x0b\x01\x02"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::TableIndexRelSLEB, 1, 2, nullopt},
      "\x0c\x01\x02"_su8);

  // Relocation types with addend.
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::MemoryAddressLEB, 1, 2, 3},
      "\x03\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::MemoryAddressSLEB, 1, 2, 3},
      "\x04\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::MemoryAddressI32, 1, 2, 3},
      "\x05\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::FunctionOffsetI32, 1, 2, 3},
      "\x08\x01\x02\x03"_su8);
  ExpectRead<RelocationEntry>(
      RelocationEntry{RelocationType::SectionOffsetI32, 1, 2, 3},
      "\x09\x01\x02\x03"_su8);
}

TEST(ReadLinkingTest, RelocationType) {
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

TEST(ReadLinkingTest, ReadSegmentInfo) {
  ExpectRead<SegmentInfo>(SegmentInfo{"name"_sv, 1, 2}, "\x04name\x01\x02"_su8);
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

TEST(ReadLinkingTest, SymbolInfo_Flags) {
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

TEST(ReadLinkingTest, SymbolInfo_Function) {
  using SI = SymbolInfo;
  ExpectRead<SI>(
      SI{undefined_flags, SI::Base{SymbolInfoKind::Function, 0, nullopt}},
      "\x00\x10\x00"_su8);
  ExpectRead<SI>(
      SI{explicit_name_flags, SI::Base{SymbolInfoKind::Function, 0, "name"_sv}},
      "\x00\x40\x00\x04name"_su8);
}

TEST(ReadLinkingTest, SymbolInfo_Data) {
  using SI = SymbolInfo;
  ExpectRead<SI>(
      SI{zero_flags, SI::Data{"name"_sv, SI::Data::Defined{0, 0, 0}}},
      "\x01\x00\x04name\x00\x00\x00"_su8);
  ExpectRead<SI>(SI{undefined_flags, SI::Data{"name"_sv, nullopt}},
                 "\x01\x10\x04name"_su8);
}

TEST(ReadLinkingTest, SymbolInfo_Global) {
  using SI = SymbolInfo;
  ExpectRead<SI>(
      SI{undefined_flags, SI::Base{SymbolInfoKind::Global, 0, nullopt}},
      "\x02\x10\x00"_su8);
  ExpectRead<SI>(
      SI{explicit_name_flags, SI::Base{SymbolInfoKind::Global, 0, "name"_sv}},
      "\x02\x40\x00\x04name"_su8);
}

TEST(ReadLinkingTest, SymbolInfo_Section) {
  using SI = SymbolInfo;
  ExpectRead<SI>(SI{zero_flags, SI::Section{0}}, "\x03\x00\x00"_su8);
}

TEST(ReadLinkingTest, SymbolInfo_Event) {
  using SI = SymbolInfo;
  ExpectRead<SI>(
      SI{undefined_flags, SI::Base{SymbolInfoKind::Event, 0, nullopt}},
      "\x04\x10\x00"_su8);
  ExpectRead<SI>(
      SI{explicit_name_flags, SI::Base{SymbolInfoKind::Event, 0, "name"_sv}},
      "\x04\x40\x00\x04name"_su8);
}

TEST(ReadLinkingTest, SymbolInfoKind) {
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Function, "\x00"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Data, "\x01"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Global, "\x02"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Section, "\x03"_su8);
  ExpectRead<SymbolInfoKind>(SymbolInfoKind::Event, "\x04"_su8);
}

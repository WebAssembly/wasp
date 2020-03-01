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

#include "wasp/binary/linking_section/types.h"

#include "src/base/operator_eq_ne_macros.h"
#include "src/base/std_hash_macros.h"

namespace wasp {
namespace binary {

SymbolInfo::SymbolInfo(At<Flags> flags, const Base& base)
    : flags{flags}, desc{base} {
  assert(base.kind == SymbolInfoKind::Function ||
         base.kind == SymbolInfoKind::Global ||
         base.kind == SymbolInfoKind::Event);
}

SymbolInfo::SymbolInfo(At<Flags> flags, const Data& data)
    : flags{flags}, desc{data} {}

SymbolInfo::SymbolInfo(At<Flags> flags, const Section& section)
    : flags{flags}, desc{section} {}


WASP_OPERATOR_EQ_NE_3(Comdat, name, flags, symbols)
WASP_OPERATOR_EQ_NE_2(ComdatSymbol, kind, index)
WASP_OPERATOR_EQ_NE_2(InitFunction, priority, index)
WASP_OPERATOR_EQ_NE_2(LinkingSubsection, id, data)
WASP_OPERATOR_EQ_NE_4(RelocationEntry, type, offset, index, addend)
WASP_OPERATOR_EQ_NE_3(SegmentInfo, name, align_log2, flags)
WASP_OPERATOR_EQ_NE_2(SymbolInfo, flags, desc)
WASP_OPERATOR_EQ_NE_4(SymbolInfo::Flags,
                      binding,
                      visibility,
                      undefined,
                      explicit_name)
WASP_OPERATOR_EQ_NE_3(SymbolInfo::Base, kind, index, name)
WASP_OPERATOR_EQ_NE_2(SymbolInfo::Data, name, defined)
WASP_OPERATOR_EQ_NE_3(SymbolInfo::Data::Defined, index, offset, size)
WASP_OPERATOR_EQ_NE_1(SymbolInfo::Section, section)


}  // namespace binary
}  // namespace wasp

WASP_STD_HASH_2(::wasp::binary::ComdatSymbol, kind, index)
WASP_STD_HASH_2(::wasp::binary::InitFunction, priority, index)
WASP_STD_HASH_2(::wasp::binary::LinkingSubsection, id, data)
WASP_STD_HASH_4(::wasp::binary::RelocationEntry, type, offset, index, addend)
WASP_STD_HASH_3(::wasp::binary::SegmentInfo, name, align_log2, flags)
WASP_STD_HASH_2(::wasp::binary::SymbolInfo, flags, desc)
WASP_STD_HASH_4(::wasp::binary::SymbolInfo::Flags,
                binding,
                visibility,
                undefined,
                explicit_name)
WASP_STD_HASH_3(::wasp::binary::SymbolInfo::Base, kind, index, name)
WASP_STD_HASH_2(::wasp::binary::SymbolInfo::Data, name, defined)
WASP_STD_HASH_3(::wasp::binary::SymbolInfo::Data::Defined, index, offset, size)
WASP_STD_HASH_1(::wasp::binary::SymbolInfo::Section, section)

namespace std {
size_t hash<::wasp::binary::Comdat>::operator()(
    const ::wasp::binary::Comdat& v) const {
  return ::wasp::HashState::combine(0, v.name, v.flags,
                                    ::wasp::HashContainer(v.symbols));
}
}  // namespace std

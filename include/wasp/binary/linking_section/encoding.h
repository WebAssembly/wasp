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

#ifndef WASP_BINARY_LINKING_SECTION_ENCODING_H
#define WASP_BINARY_LINKING_SECTION_ENCODING_H

#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/linking_section/types.h"

namespace wasp {

class Features;

namespace binary {
namespace encoding {

struct ComdatSymbolKind {
  static u8 Encode(::wasp::binary::ComdatSymbolKind);
  static optional<::wasp::binary::ComdatSymbolKind> Decode(u8);
};

struct LinkingSubsectionId {
  static u8 Encode(::wasp::binary::LinkingSubsectionId);
  static optional<::wasp::binary::LinkingSubsectionId> Decode(u8);
};

struct RelocationType {
  static u8 Encode(::wasp::binary::RelocationType);
  static optional<::wasp::binary::RelocationType> Decode(u8);
};

struct SymbolInfoFlags {
  static constexpr u32 BindingGlobal = 0x00;
  static constexpr u32 BindingWeak = 0x01;
  static constexpr u32 BindingLocal = 0x02;
  static constexpr u32 BindingMask = 0x03;
  static constexpr u32 VisibilityHidden = 0x04;
  static constexpr u32 Undefined = 0x10;
  static constexpr u32 ExplicitName = 0x40;

  static u32 Encode(SymbolInfo::Flags);
  static optional<SymbolInfo::Flags> Decode(u32);
};

struct SymbolInfoKind {
  static u8 Encode(::wasp::binary::SymbolInfoKind);
  static optional<::wasp::binary::SymbolInfoKind> Decode(u8);
};

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_LINKING_SECTION_ENCODING_H

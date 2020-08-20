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

#include "wasp/binary/linking_section/encoding.h"

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/linking_section/types.h"

namespace wasp::binary::encoding {

// static
u8 ComdatSymbolKind::Encode(::wasp::binary::ComdatSymbolKind decoded) {
  return u8(decoded);
}

// static
optional<::wasp::binary::ComdatSymbolKind> ComdatSymbolKind::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::ComdatSymbolKind::Name;
#include "wasp/binary/def/comdat_symbol_kind.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

// static
u8 LinkingSubsectionId::Encode(::wasp::binary::LinkingSubsectionId decoded) {
  return u8(decoded);
}

// static
optional<::wasp::binary::LinkingSubsectionId> LinkingSubsectionId::Decode(
    u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::LinkingSubsectionId::Name;
#include "wasp/binary/def/linking_subsection_id.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

// static
u8 RelocationType::Encode(::wasp::binary::RelocationType decoded) {
  return u8(decoded);
}

// static
optional<::wasp::binary::RelocationType> RelocationType::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::RelocationType::Name;
#include "wasp/binary/def/relocation_type.def"
#undef WASP_V
    default:
      break;
  }
  return nullopt;
}

// static
u32 SymbolInfoFlags::Encode(SymbolInfo::Flags flags) {
  u32 result = 0;
  switch (flags.binding) {
    case SymbolInfo::Flags::Binding::Global:
      break;
    case SymbolInfo::Flags::Binding::Weak:
      result |= BindingWeak;
      break;
    case SymbolInfo::Flags::Binding::Local:
      result |= BindingLocal;
      break;
  }
  if (flags.visibility == SymbolInfo::Flags::Visibility::Hidden) {
    result |= VisibilityHidden;
  }
  if (flags.undefined == SymbolInfo::Flags::Undefined::Yes) {
    result |= Undefined;
  }
  if (flags.explicit_name == SymbolInfo::Flags::ExplicitName::Yes) {
    result |= ExplicitName;
  }
  return result;
}

// static
optional<SymbolInfo::Flags> SymbolInfoFlags::Decode(u32 flags) {
  SymbolInfo::Flags result;
  switch (flags & BindingMask) {
    case BindingGlobal:
      result.binding = SymbolInfo::Flags::Binding::Global;
      break;
    case BindingWeak:
      result.binding = SymbolInfo::Flags::Binding::Weak;
      break;
    case BindingLocal:
      result.binding = SymbolInfo::Flags::Binding::Local;
      break;
    default:
      return nullopt;
  }
  result.visibility = flags & VisibilityHidden
                          ? SymbolInfo::Flags::Visibility::Hidden
                          : SymbolInfo::Flags::Visibility::Default;
  result.undefined = flags & Undefined ? SymbolInfo::Flags::Undefined::Yes
                                       : SymbolInfo::Flags::Undefined::No;
  result.explicit_name = flags & ExplicitName
                             ? SymbolInfo::Flags::ExplicitName::Yes
                             : SymbolInfo::Flags::ExplicitName::No;
  return result;
}

// static
u8 SymbolInfoKind::Encode(::wasp::binary::SymbolInfoKind decoded) {
  return u8(decoded);
}

// static
optional<::wasp::binary::SymbolInfoKind> SymbolInfoKind::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::SymbolInfoKind::Name;
#include "wasp/binary/def/symbol_info_kind.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

}  // namespace wasp::binary::encoding

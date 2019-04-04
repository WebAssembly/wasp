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

#ifndef WASP_BINARY_SYMBOL_INFO_FLAGS_ENCODING_H
#define WASP_BINARY_SYMBOL_INFO_FLAGS_ENCODING_H

#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/symbol_info.h"

namespace wasp {
namespace binary {
namespace encoding {

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

// static
inline u32 SymbolInfoFlags::Encode(SymbolInfo::Flags flags) {
  u32 result = 0;
  switch (flags.binding) {
    case SymbolInfo::Flags::Binding::Global: break;
    case SymbolInfo::Flags::Binding::Weak:   result |= BindingWeak;
    case SymbolInfo::Flags::Binding::Local:  result |= BindingLocal;
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
inline optional<SymbolInfo::Flags> SymbolInfoFlags::Decode(u32 flags) {
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

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SYMBOL_INFO_TYPE_ENCODING_H

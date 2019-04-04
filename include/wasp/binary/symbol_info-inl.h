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

#include "wasp/binary/symbol_info.h"

namespace wasp {
namespace binary {

inline SymbolInfoKind SymbolInfo::kind() const {
  switch (desc.index()) {
    case 0:
      return base().kind;
    case 1:
      return SymbolInfoKind::Data;
    case 2:
      return SymbolInfoKind::Section;
    default:
      WASP_UNREACHABLE;
  }
}

inline bool SymbolInfo::is_base() const {
  return desc.index() == 0;
}

inline bool SymbolInfo::is_data() const {
  return desc.index() == 1;
}

inline bool SymbolInfo::is_section() const {
  return desc.index() == 2;
}

inline SymbolInfo::Base& SymbolInfo::base() {
  return get<Base>(desc);
}

inline const SymbolInfo::Base& SymbolInfo::base() const {
  return get<Base>(desc);
}

inline SymbolInfo::Data& SymbolInfo::data() {
  return get<Data>(desc);
}

inline const SymbolInfo::Data& SymbolInfo::data() const {
  return get<Data>(desc);
}

inline SymbolInfo::Section& SymbolInfo::section() {
  return get<Section>(desc);
}

inline const SymbolInfo::Section& SymbolInfo::section() const {
  return get<Section>(desc);
}

}  // namespace binary
}  // namespace wasp

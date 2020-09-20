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

#include <cassert>

#include "wasp/base/hash.h"
#include "wasp/base/macros.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

namespace wasp::binary {

SymbolInfo::SymbolInfo(At<Flags> flags, const Base& base)
    : flags{flags}, desc{base} {
  assert(base.kind.value() == SymbolInfoKind::Function ||
         base.kind.value() == SymbolInfoKind::Global ||
         base.kind.value() == SymbolInfoKind::Event);
}

SymbolInfo::SymbolInfo(At<Flags> flags, const Data& data)
    : flags{flags}, desc{data} {}

SymbolInfo::SymbolInfo(At<Flags> flags, const Section& section)
    : flags{flags}, desc{section} {}

SymbolInfoKind SymbolInfo::kind() const {
  switch (desc.index()) {
    case 0:
      return *base().kind;
    case 1:
      return SymbolInfoKind::Data;
    case 2:
      return SymbolInfoKind::Section;
    default:
      WASP_UNREACHABLE();
  }
}

bool SymbolInfo::is_base() const {
  return desc.index() == 0;
}

bool SymbolInfo::is_data() const {
  return desc.index() == 1;
}

bool SymbolInfo::is_section() const {
  return desc.index() == 2;
}

SymbolInfo::Base& SymbolInfo::base() {
  return get<Base>(desc);
}

const SymbolInfo::Base& SymbolInfo::base() const {
  return get<Base>(desc);
}

SymbolInfo::Data& SymbolInfo::data() {
  return get<Data>(desc);
}

const SymbolInfo::Data& SymbolInfo::data() const {
  return get<Data>(desc);
}

SymbolInfo::Section& SymbolInfo::section() {
  return get<Section>(desc);
}

const SymbolInfo::Section& SymbolInfo::section() const {
  return get<Section>(desc);
}

optional<string_view> SymbolInfo::name() const {
  if (is_base()) {
    return base().name.value();
  } else if (is_data()) {
    return data().name.value();
  } else {
    return nullopt;
  }
}

WASP_BINARY_LINKING_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_BINARY_LINKING_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

}  // namespace wasp::binary

WASP_BINARY_LINKING_STRUCTS(WASP_STD_HASH_VARGS)
WASP_BINARY_LINKING_CONTAINERS(WASP_STD_HASH_CONTAINER)

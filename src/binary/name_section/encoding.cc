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

#include "wasp/binary/name_section/encoding.h"

#include "wasp/base/features.h"
#include "wasp/base/macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/name_section/types.h"

namespace wasp::binary::encoding {

// static
u8 NameSubsectionId::Encode(::wasp::binary::NameSubsectionId decoded) {
  return u8(decoded);
}

// static
optional<::wasp::binary::NameSubsectionId> NameSubsectionId::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case val:                    \
    return ::wasp::binary::NameSubsectionId::Name;
#include "wasp/binary/def/name_subsection_id.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

}  // namespace wasp::binary::encoding

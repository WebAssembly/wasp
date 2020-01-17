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

#ifndef WASP_BINARY_LINKING_SUBSECTION_ID_ENCODING_H
#define WASP_BINARY_LINKING_SUBSECTION_ID_ENCODING_H

#include "wasp/base/macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/types_linking.h"

namespace wasp {
namespace binary {
namespace encoding {

struct LinkingSubsectionId {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
#include "wasp/binary/linking_subsection_id.def"
#undef WASP_V

  static u8 Encode(::wasp::binary::LinkingSubsectionId);
  static optional<::wasp::binary::LinkingSubsectionId> Decode(u8);
};

// static
inline u8 LinkingSubsectionId::Encode(
    ::wasp::binary::LinkingSubsectionId decoded) {
  switch (decoded) {
#define WASP_V(val, Name, str)                    \
  case ::wasp::binary::LinkingSubsectionId::Name: \
    return val;
#include "wasp/binary/linking_subsection_id.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
}

// static
inline optional<::wasp::binary::LinkingSubsectionId>
LinkingSubsectionId::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case Name:                   \
    return ::wasp::binary::LinkingSubsectionId::Name;
#include "wasp/binary/linking_subsection_id.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_LINKING_SUBSECTION_ID_ENCODING_H

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

#ifndef WASP_BINARY_NAME_SECTION_ENCODING_H
#define WASP_BINARY_NAME_SECTION_ENCODING_H

#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/binary/name_section/types.h"

namespace wasp {

class Features;

namespace binary {
namespace encoding {

struct NameSubsectionId {
  static u8 Encode(::wasp::binary::NameSubsectionId);
  static optional<::wasp::binary::NameSubsectionId> Decode(u8);
};

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_NAME_SECTION_ENCODING_H

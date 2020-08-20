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

#include "wasp/binary/name_section/formatters.h"

#include <iostream>

#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace wasp::binary {

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::NameSubsectionId& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)                 \
  case ::wasp::binary::NameSubsectionId::Name: \
    result = str;                              \
    break;
#include "wasp/binary/def/name_subsection_id.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::NameAssoc& self) {
  return os << self.index << " \"" << self.name << "\"";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::IndirectNameAssoc& self) {
  return os << self.index << " " << self.name_map;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::NameSubsection& self) {
  return os << self.id << " " << self.data;
}

}  // namespace wasp::binary

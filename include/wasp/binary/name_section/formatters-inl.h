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

#include "wasp/binary/linking_section/formatters.h"

#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::NameSubsectionId>::format(
    const ::wasp::binary::NameSubsectionId& self,
    Ctx& ctx) {
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
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::NameAssoc>::format(
    const ::wasp::binary::NameAssoc& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} \"{}\"", self.index, self.name);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::IndirectNameAssoc>::format(
    const ::wasp::binary::IndirectNameAssoc& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.index, self.name_map);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::NameSubsection>::format(
    const ::wasp::binary::NameSubsection& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.id, self.data);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

}  // namespace fmt

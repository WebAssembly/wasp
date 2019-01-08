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

#ifndef WASP_BINARY_READ_READ_SECTION_H_
#define WASP_BINARY_READ_READ_SECTION_H_

#include "wasp/binary/section.h"

#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_bytes.h"
#include "wasp/binary/read/read_length.h"
#include "wasp/binary/read/read_section_id.h"
#include "wasp/binary/read/read_string.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Section> Read(SpanU8* data, Errors& errors, Tag<Section>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "section"};
  WASP_TRY_READ(id, Read<SectionId>(data, errors));
  WASP_TRY_READ(length, ReadLength(data, errors));
  auto bytes = *ReadBytes(data, length, errors);

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name, ReadString(&bytes, errors, "custom section name"));
    return Section{CustomSection{name, bytes}};
  } else {
    return Section{KnownSection{id, bytes}};
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_SECTION_H_

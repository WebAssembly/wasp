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

#ifndef WASP_BINARY_READ_READ_NAME_SUBSECTION_ID_H_
#define WASP_BINARY_READ_READ_NAME_SUBSECTION_ID_H_

#include "wasp/binary/name_subsection_id.h"

#include "wasp/binary/encoding/name_subsection_id_encoding.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_u8.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<NameSubsectionId> Read(SpanU8* data,
                                Errors& errors,
                                Tag<NameSubsectionId>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "name subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, NameSubsectionId, "name subsection id");
  return decoded;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_NAME_SUBSECTION_ID_H_

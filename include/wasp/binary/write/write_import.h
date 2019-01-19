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

#ifndef WASP_BINARY_WRITE_WRITE_IMPORT_H_
#define WASP_BINARY_WRITE_WRITE_IMPORT_H_

#include "wasp/binary/import.h"
#include "wasp/binary/write/write_external_kind.h"
#include "wasp/binary/write/write_global_type.h"
#include "wasp/binary/write/write_index.h"
#include "wasp/binary/write/write_memory_type.h"
#include "wasp/binary/write/write_string.h"
#include "wasp/binary/write/write_table_type.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(const Import& value, Iterator out) {
  out = Write(value.module, out);
  out = Write(value.name, out);
  out = Write(value.kind(), out);
  switch (value.kind()) {
    case ExternalKind::Function:
      return WriteIndex(value.index(), out);

    case ExternalKind::Table:
      return Write(value.table_type(), out);

    case ExternalKind::Memory:
      return Write(value.memory_type(), out);

    case ExternalKind::Global:
      return Write(value.global_type(), out);

    default:
      WASP_UNREACHABLE();
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_IMPORT_H_

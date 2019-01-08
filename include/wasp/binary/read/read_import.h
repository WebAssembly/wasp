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

#ifndef WASP_BINARY_READ_READ_IMPORT_H_
#define WASP_BINARY_READ_READ_IMPORT_H_

#include "wasp/binary/import.h"

#include "wasp/base/macros.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_external_kind.h"
#include "wasp/binary/read/read_global_type.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_memory_type.h"
#include "wasp/binary/read/read_string.h"
#include "wasp/binary/read/read_table_type.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<Import> Read(SpanU8* data, Errors& errors, Tag<Import>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "import"};
  WASP_TRY_READ(module, ReadString(data, errors, "module name"));
  WASP_TRY_READ(name, ReadString(data, errors, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, errors));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index, ReadIndex(data, errors, "function index"));
      return Import{module, name, type_index};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, errors));
      return Import{module, name, table_type};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, errors));
      return Import{module, name, memory_type};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, errors));
      return Import{module, name, global_type};
    }
  }
  WASP_UNREACHABLE();
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_IMPORT_H_

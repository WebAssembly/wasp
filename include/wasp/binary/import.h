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

#ifndef WASP_BINARY_IMPORT_H_
#define WASP_BINARY_IMPORT_H_

#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/binary/external_kind.h"
#include "wasp/binary/global_type.h"
#include "wasp/binary/memory_type.h"
#include "wasp/binary/table_type.h"

namespace wasp {
namespace binary {

struct Import {
  ExternalKind kind() const;
  bool is_function() const;
  bool is_table() const;
  bool is_memory() const;
  bool is_global() const;

  Index& index();
  const Index& index() const;
  TableType& table_type();
  const TableType& table_type() const;
  MemoryType& memory_type();
  const MemoryType& memory_type() const;
  GlobalType& global_type();
  const GlobalType& global_type() const;

  string_view module;
  string_view name;
  variant<Index, TableType, MemoryType, GlobalType> desc;
};

bool operator==(const Import&, const Import&);
bool operator!=(const Import&, const Import&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/import-inl.h"

#endif // WASP_BINARY_IMPORT_H_

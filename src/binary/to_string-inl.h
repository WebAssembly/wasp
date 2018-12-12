//
// Copyright 2018 WebAssembly Community Group participants
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

#include "src/binary/to_string.h"

#include "src/base/format.h"
#include "src/base/macros.h"
#include "src/base/to_string.h"

namespace wasp {
namespace binary {

using wasp::ToString;

template <typename Traits>
std::string ToString(const KnownSection<Traits>& self) {
  return format("{{id {}, contents {}}}", self.id, ToString(self.data));
}

template <typename Traits>
std::string ToString(const CustomSection<Traits>& self) {
  return format("{{name \"{}\", contents {}}}", self.name.to_string(),
                ToString(self.data));
}

template <typename Traits>
std::string ToString(const Section<Traits>& self) {
  if (self.is_known()) {
    return ToString(self.known());
  } else if (self.is_custom()) {
    return ToString(self.custom());
  } else {
    WASP_UNREACHABLE();
  }
}

template <typename Traits>
std::string ToString(const Import<Traits>& self) {
  std::string result =
      format("{{module \"{}\", name \"{}\", desc {}", self.module.to_string(),
             self.name.to_string(), ToString(self.kind()));

  if (holds_alternative<Index>(self.desc)) {
    result += format(" {}}}", get<Index>(self.desc));
  } else if (holds_alternative<TableType>(self.desc)) {
    result += format(" {}}}", ToString(get<TableType>(self.desc)));
  } else if (holds_alternative<MemoryType>(self.desc)) {
    result += format(" {}}}", ToString(get<MemoryType>(self.desc)));
  } else if (holds_alternative<GlobalType>(self.desc)) {
    result += format(" {}}}", ToString(get<GlobalType>(self.desc)));
  }

  return result;
}

template <typename Traits>
std::string ToString(const Export<Traits>& self) {
  return format("{{name \"{}\", desc {} {}}}", self.name.to_string(),
                ToString(self.kind), self.index);
}

template <typename Traits>
std::string ToString(const Expr<Traits>& self) {
  return ToString(self.data);
}

template <typename Traits>
std::string ToString(const Global<Traits>& self) {
  return format("{{type {}, init {}}}", ToString(self.global_type),
                ToString(self.init_expr));
}

template <typename Traits>
std::string ToString(const ElementSegment<Traits>& self) {
  return format("{{table {}, offset {}, init {}}}", self.table_index,
                ToString(self.offset), ToString(self.init));
}

template <typename Traits>
std::string ToString(const DataSegment<Traits>& self) {
  return format("{{memory {}, offset {}, init {}}}", self.memory_index,
                ToString(self.offset), ToString(self.init));
}

template <typename Traits>
std::string ToString(const Code<Traits>& self) {
  return format("{{locals {}, body {}}}", ToString(self.local_decls),
                ToString(self.body));
}

}  // namespace binary
}  // namespace wasp

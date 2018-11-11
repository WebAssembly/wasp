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

#include "absl/strings/str_format.h"

#include "src/base/to_string.h"

namespace wasp {
namespace binary {

using wasp::ToString;

template <typename Traits>
std::string ToString(const CustomSection<Traits>& self) {
  std::string result = "{after_id ";
  if (self.after_id) {
    absl::StrAppendFormat(&result, "%u", *self.after_id);
  } else {
    absl::StrAppendFormat(&result, "<none>");
  }

  absl::StrAppendFormat(&result, ", name \"%s\", contents %s}", self.name,
                        ToString(self.data));
  return result;
}

template <typename Traits>
std::string ToString(const Import<Traits>& self) {
  std::string result =
      absl::StrFormat("{module \"%s\", name \"%s\", desc %s", self.module,
                      self.name, ToString(self.kind()));

  if (absl::holds_alternative<Index>(self.desc)) {
    absl::StrAppendFormat(&result, " %d}", absl::get<Index>(self.desc));
  } else if (absl::holds_alternative<TableType>(self.desc)) {
    absl::StrAppendFormat(&result, " %s}",
                          ToString(absl::get<TableType>(self.desc)));
  } else if (absl::holds_alternative<MemoryType>(self.desc)) {
    absl::StrAppendFormat(&result, " %s}",
                          ToString(absl::get<MemoryType>(self.desc)));
  } else if (absl::holds_alternative<GlobalType>(self.desc)) {
    absl::StrAppendFormat(&result, " %s}",
                          ToString(absl::get<GlobalType>(self.desc)));
  }

  return result;
}

template <typename Traits>
std::string ToString(const Export<Traits>& self) {
  return absl::StrFormat("{name \"%s\", desc %s %u}", self.name,
                         ToString(self.kind), self.index);
}

template <typename Traits>
std::string ToString(const Expr<Traits>& self) {
  return ToString(self.data);
}

template <typename Traits>
std::string ToString(const Global<Traits>& self) {
  return absl::StrFormat("{type %s, init %s}", ToString(self.global_type),
                         ToString(self.init_expr));
}

template <typename Traits>
std::string ToString(const ElementSegment<Traits>& self) {
  return absl::StrFormat("{table %u, offset %s, init %s}", self.table_index,
                         ToString(self.offset), ToString(self.init));
}

template <typename Traits>
std::string ToString(const DataSegment<Traits>& self) {
  return absl::StrFormat("{memory %u, offset %s, init %s}", self.memory_index,
                         ToString(self.offset), ToString(self.init));
}

template <typename Traits>
std::string ToString(const Code<Traits>& self) {
  return absl::StrFormat("{locals %s, body %s}", ToString(self.local_decls),
                         ToString(self.body));
}

}  // namespace binary
}  // namespace wasp

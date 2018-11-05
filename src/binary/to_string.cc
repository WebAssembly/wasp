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

#include "src/base/to_string.h"

namespace wasp {
namespace binary {

using ::wasp::ToString;

std::string ToString(ValType self) {
  switch (self) {
    case ValType::I32: return "i32";
    case ValType::I64: return "i64";
    case ValType::F32: return "f32";
    case ValType::F64: return "f64";
    case ValType::Anyfunc: return "anyfunc";
    case ValType::Func: return "func";
    case ValType::Void: return "void";
  }
}

std::string ToString(ExternalKind self) {
  switch (self) {
    case ExternalKind::Func: return "func";
    case ExternalKind::Table: return "table";
    case ExternalKind::Memory: return "memory";
    case ExternalKind::Global: return "global";
  }
}

std::string ToString(Mutability self) {
  switch (self) {
    case Mutability::Const: return "const";
    case Mutability::Var: return "var";
  }
}

std::string ToString(const MemArg& self) {
  return absl::StrFormat("{align %u, offset %u}", self.align_log2, self.offset);
}

std::string ToString(const Limits& self) {
  if (self.max) {
    return absl::StrFormat("{min %u, max %u}", self.min, *self.max);
  } else {
    return absl::StrFormat("{min %u}", self.min);
  }
}

std::string ToString(const LocalDecl& self) {
  return absl::StrFormat("%s ** %u", ToString(self.type), self.count);
}

std::string ToString(const Section& self) {
  return absl::StrFormat("{id %u, contents (%zu bytes)}", self.id,
                         self.data.size());
}

std::string ToString(const CustomSection& self) {
  if (self.after_id) {
    return absl::StrFormat("{after_id %u, name %s, contents (%zu bytes)}",
                           *self.after_id, self.name, self.data.size());
  } else {
    return absl::StrFormat("{after_id <none>, name %s, contents (%zu bytes)}",
                           self.name, self.data.size());
  }
}

std::string ToString(const FuncType& self) {
  return absl::StrFormat("%s -> %s", ToString(self.param_types),
                         ToString(self.result_types));
}

std::string ToString(const TableType& self) {
  return absl::StrFormat("%s %s", ToString(self.limits),
                         ToString(self.elemtype));
}

std::string ToString(const MemoryType& self) {
  return absl::StrFormat("%s", ToString(self.limits));
}

std::string ToString(const GlobalType& self) {
  return absl::StrFormat("%s %s", ToString(self.mut), ToString(self.valtype));
}

std::string ToString(const Import& self) {
  return absl::StrFormat("module \"%s\", name \"%s\", desc %s", self.module,
                         self.name, ToString(self.kind));
}

std::string ToString(const FuncImport& self) {
  return absl::StrFormat("{%s %d}", ToString(static_cast<const Import&>(self)),
                         self.type_index);
}

std::string ToString(const TableImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.table_type));
}

std::string ToString(const MemoryImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.memory_type));
}

std::string ToString(const GlobalImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.global_type));
}

std::string ToString(const Export& self) {
  return absl::StrFormat("{name \"%s\", desc %s %u}", self.name,
                         ToString(self.kind), self.index);
}

std::string ToString(const Expr& self) {
  return absl::StrJoin(self.instrs, "", [](std::string* out, u8 x) {
    return absl::StrAppendFormat(out, "\\%02x", x);
  });
}

std::string ToString(const Global& self) {
  return absl::StrFormat("{type %s, init %s}", ToString(self.global_type),
                         ToString(self.init_expr));
}

std::string ToString(const ElementSegment& self) {
  return absl::StrFormat("{table %u, offset %s, init %s}", self.table_index,
                         ToString(self.offset), ToString(self.init));
}

std::string ToString(const DataSegment& self) {
  return absl::StrFormat("{table %u, offset %s, init %s}", self.memory_index,
                         ToString(self.offset), ToString(self.init));
}

}  // namespace binary
}  // namespace wasp

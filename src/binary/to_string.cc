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

#include "src/base/macros.h"
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
    default: WASP_UNREACHABLE();
  }
}

std::string ToString(ExternalKind self) {
  switch (self) {
    case ExternalKind::Func: return "func";
    case ExternalKind::Table: return "table";
    case ExternalKind::Memory: return "memory";
    case ExternalKind::Global: return "global";
    default: WASP_UNREACHABLE();
  }
}

std::string ToString(Mutability self) {
  switch (self) {
    case Mutability::Const: return "const";
    case Mutability::Var: return "var";
    default: WASP_UNREACHABLE();
  }
}

std::string ToString(const MemArg& self) {
  return format("{{align {}, offset {}}}", self.align_log2, self.offset);
}

std::string ToString(const Limits& self) {
  if (self.max) {
    return format("{{min {}, max {}}}", self.min, *self.max);
  } else {
    return format("{{min {}}}", self.min);
  }
}

std::string ToString(const LocalDecl& self) {
  return format("{} ** {}", ToString(self.type), self.count);
}

std::string ToString(const FuncType& self) {
  return format("{} -> {}", ToString(self.param_types),
                ToString(self.result_types));
}

std::string ToString(const TypeEntry& self) {
  return format("{} {}", ToString(self.form), ToString(self.type));
}

std::string ToString(const TableType& self) {
  return format("{} {}", ToString(self.limits), ToString(self.elemtype));
}

std::string ToString(const MemoryType& self) {
  return format("{}", ToString(self.limits));
}

std::string ToString(const GlobalType& self) {
  return format("{} {}", ToString(self.mut), ToString(self.valtype));
}

std::string ToString(const Opcode& opcode) {
  if (opcode.prefix) {
    return format("{:02x} {:08x}", *opcode.prefix, opcode.code);
  } else {
    return format("{:02x}", opcode.code);
  }
}

std::string ToString(const CallIndirectImmediate& imm) {
  return format("{} {}", imm.index, imm.reserved);
}

std::string ToString(const BrTableImmediate& imm) {
  return format("{} {}", ToString(imm.targets), imm.default_target);
}

std::string ToString(const Instr& instr) {
  std::string result = ToString(instr.opcode);

  if (holds_alternative<EmptyImmediate>(instr.immediate)) {
    // Nothing.
  } else if (holds_alternative<ValType>(instr.immediate)) {
    result += format(" {}", ToString(get<ValType>(instr.immediate)));
  } else if (holds_alternative<Index>(instr.immediate)) {
    result += format(" {}", get<Index>(instr.immediate));
  } else if (holds_alternative<CallIndirectImmediate>(instr.immediate)) {
    result +=
        format(" {}", ToString(get<CallIndirectImmediate>(instr.immediate)));
  } else if (holds_alternative<BrTableImmediate>(instr.immediate)) {
    result += format(" {}", ToString(get<BrTableImmediate>(instr.immediate)));
  } else if (holds_alternative<u8>(instr.immediate)) {
    result += format(" {}", get<u8>(instr.immediate));
  } else if (holds_alternative<MemArg>(instr.immediate)) {
    result += format(" {}", ToString(get<MemArg>(instr.immediate)));
  } else if (holds_alternative<s32>(instr.immediate)) {
    result += format(" {}", get<s32>(instr.immediate));
  } else if (holds_alternative<s64>(instr.immediate)) {
    result += format(" {}", get<s64>(instr.immediate));
  } else if (holds_alternative<f32>(instr.immediate)) {
    result += format(" {:f}", get<f32>(instr.immediate));
  } else if (holds_alternative<f64>(instr.immediate)) {
    result += format(" {:f}", get<f64>(instr.immediate));
  }

  return result;
}

std::string ToString(const Func& self) {
  return format("{{type {}}}", self.type_index);
}

std::string ToString(const Table& self) {
  return format("{{type {}}}", ToString(self.table_type));
}

std::string ToString(const Memory& self) {
  return format("{{type {}}}", ToString(self.memory_type));
}

std::string ToString(const Start& self) {
  return format("{{func {}}}", self.func_index);
}

}  // namespace binary
}  // namespace wasp

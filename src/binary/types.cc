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

#include "src/binary/types.h"

namespace wasp {
namespace binary {

bool operator==(const MemArg& lhs, const MemArg& rhs) {
  return lhs.align_log2 == rhs.align_log2 && lhs.offset == rhs.offset;
}

bool operator!=(const MemArg& lhs, const MemArg& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Limits& lhs, const Limits& rhs) {
  return lhs.min == rhs.min && lhs.max == rhs.max;
}

bool operator!=(const Limits& lhs, const Limits& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Locals& lhs, const Locals& rhs) {
  return lhs.count == rhs.count && lhs.type == rhs.type;
}

bool operator!=(const Locals& lhs, const Locals& rhs) {
  return !(lhs == rhs);
}

bool operator==(const FunctionType& lhs, const FunctionType& rhs) {
  return lhs.param_types == rhs.param_types &&
         lhs.result_types == rhs.result_types;
}

bool operator!=(const FunctionType& lhs, const FunctionType& rhs) {
  return !(lhs == rhs);
}

bool operator==(const TypeEntry& lhs, const TypeEntry& rhs) {
  return lhs.type == rhs.type;
}

bool operator!=(const TypeEntry& lhs, const TypeEntry& rhs) {
  return !(lhs == rhs);
}

bool operator==(const TableType& lhs, const TableType& rhs) {
  return lhs.limits == rhs.limits && lhs.elemtype == rhs.elemtype;
}

bool operator!=(const TableType& lhs, const TableType& rhs) {
  return !(lhs == rhs);
}

bool operator==(const MemoryType& lhs, const MemoryType& rhs) {
  return lhs.limits == rhs.limits;
}

bool operator!=(const MemoryType& lhs, const MemoryType& rhs) {
  return !(lhs == rhs);
}

bool operator==(const GlobalType& lhs, const GlobalType& rhs) {
  return lhs.valtype == rhs.valtype && lhs.mut == rhs.mut;
}

bool operator!=(const GlobalType& lhs, const GlobalType& rhs) {
  return !(lhs == rhs);
}

bool operator==(const EmptyImmediate& lhs, const EmptyImmediate& rhs) {
  return true;
}

bool operator!=(const EmptyImmediate& lhs, const EmptyImmediate& rhs) {
  return false;
}

bool operator==(const CallIndirectImmediate& lhs,
                const CallIndirectImmediate& rhs) {
  return lhs.index == rhs.index && lhs.reserved == rhs.reserved;
}

bool operator!=(const CallIndirectImmediate& lhs,
                const CallIndirectImmediate& rhs) {
  return !(lhs == rhs);
}

bool operator==(const BrTableImmediate& lhs, const BrTableImmediate& rhs) {
  return lhs.targets == rhs.targets && lhs.default_target == rhs.default_target;
}

bool operator!=(const BrTableImmediate& lhs, const BrTableImmediate& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Instruction& lhs, const Instruction& rhs) {
  return lhs.opcode == rhs.opcode && lhs.immediate == rhs.immediate;
}

bool operator!=(const Instruction& lhs, const Instruction& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Function& lhs, const Function& rhs) {
  return lhs.type_index == rhs.type_index;
}

bool operator!=(const Function& lhs, const Function& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Table& lhs, const Table& rhs) {
  return lhs.table_type == rhs.table_type;
}

bool operator!=(const Table& lhs, const Table& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Memory& lhs, const Memory& rhs) {
  return lhs.memory_type == rhs.memory_type;
}

bool operator!=(const Memory& lhs, const Memory& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Start& lhs, const Start& rhs) {
  return lhs.func_index == rhs.func_index;
}

bool operator!=(const Start& lhs, const Start& rhs) {
  return !(lhs == rhs);
}

bool operator==(const KnownSection& lhs, const KnownSection& rhs) {
  return lhs.id == rhs.id && lhs.data == rhs.data;
}

bool operator!=(const KnownSection& lhs, const KnownSection& rhs) {
  return !(lhs == rhs);
}

bool operator==(const CustomSection& lhs, const CustomSection& rhs) {
  return lhs.name == rhs.name && lhs.data == rhs.data;
}

bool operator!=(const CustomSection& lhs, const CustomSection& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Section& lhs, const Section& rhs) {
  return lhs.contents == rhs.contents;
}

bool operator!=(const Section& lhs, const Section& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Import& lhs, const Import& rhs) {
  return lhs.module == rhs.module && lhs.name == rhs.name &&
         lhs.desc == rhs.desc;
}

bool operator!=(const Import& lhs, const Import& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Expression& lhs, const Expression& rhs) {
  return lhs.data == rhs.data;
}

bool operator!=(const Expression& lhs, const Expression& rhs) {
  return !(lhs == rhs);
}

bool operator==(const ConstantExpression& lhs, const ConstantExpression& rhs) {
  return lhs.data == rhs.data;
}

bool operator!=(const ConstantExpression& lhs, const ConstantExpression& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Global& lhs, const Global& rhs) {
  return lhs.global_type == rhs.global_type && lhs.init == rhs.init;
}

bool operator!=(const Global& lhs, const Global& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Export& lhs, const Export& rhs) {
  return lhs.kind == rhs.kind && lhs.name == rhs.name && lhs.index == rhs.index;
}

bool operator!=(const Export& lhs, const Export& rhs) {
  return !(lhs == rhs);
}

bool operator==(const ElementSegment& lhs, const ElementSegment& rhs) {
  return lhs.table_index == rhs.table_index && lhs.offset == rhs.offset &&
         lhs.init == rhs.init;
}

bool operator!=(const ElementSegment& lhs, const ElementSegment& rhs) {
  return !(lhs == rhs);
}

bool operator==(const Code& lhs, const Code& rhs) {
  return lhs.locals == rhs.locals && lhs.body == rhs.body;
}

bool operator!=(const Code& lhs, const Code& rhs) {
  return !(lhs == rhs);
}

bool operator==(const DataSegment& lhs, const DataSegment& rhs) {
  return lhs.memory_index == rhs.memory_index && lhs.offset == rhs.offset &&
         lhs.init == rhs.init;
}

bool operator!=(const DataSegment& lhs, const DataSegment& rhs) {
  return !(lhs == rhs);
}

}  // namespace binary
}  // namespace wasp

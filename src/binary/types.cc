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

}  // namespace binary
}  // namespace wasp

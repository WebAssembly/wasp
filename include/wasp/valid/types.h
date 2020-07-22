//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_VALID_TYPES_H_
#define WASP_VALID_TYPES_H_

#include <vector>

#include "wasp/base/macros.h"
#include "wasp/base/span.h"
#include "wasp/base/types.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace valid {

struct Any {};

struct StackType {
  explicit StackType();
  explicit StackType(binary::ValueType);
  explicit StackType(Any);

  static StackType I32();
  static StackType I64();
  static StackType F32();
  static StackType F64();
  static StackType V128();
  static StackType Funcref();
  static StackType Externref();
  static StackType Exnref();

  bool is_value_type() const;
  bool is_any() const;

  auto value_type() -> binary::ValueType&;
  auto value_type() const -> const binary::ValueType&;

  variant<binary::ValueType, Any> type;
};

using StackTypeList = std::vector<StackType>;
using StackTypeSpan = span<const StackType>;

binary::ValueType ToValueType(binary::ReferenceType);
binary::ValueType ToValueType(binary::HeapType);
StackType ToStackType(binary::ValueType);
StackType ToStackType(binary::ReferenceType);
StackType ToStackType(binary::HeapType);
StackTypeList ToStackTypeList(const binary::ValueTypeList&);
bool IsReferenceTypeOrAny(StackType);
binary::ReferenceType Canonicalize(binary::ReferenceType);

#define WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_V) \
  WASP_V(valid::Any, 0)            \
  WASP_V(valid::StackType, 1, type)

#define WASP_VALID_CONTAINERS(WASP_V) \
  WASP_V(valid::StackTypeList)        \
  WASP_V(valid::StackTypeSpan)

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_VALID_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_PRINT_TO)


}  // namespace valid
}  // namespace wasp

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_STD_HASH)
WASP_VALID_CONTAINERS(WASP_DECLARE_STD_HASH)

#endif // WASP_VALID_TYPES_H_

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

#include "wasp/valid/types.h"

#include <cassert>

#include "wasp/base/hash.h"
#include "wasp/base/macros.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/std_hash_macros.h"

namespace wasp {
namespace valid {

StackType::StackType() : type{Any{}} {}

StackType::StackType(binary::ValueType type) : type{type} {}

StackType::StackType(Any type) : type{type} {}

// static
StackType StackType::I32() {
  return StackType{binary::ValueType::I32()};
}

// static
StackType StackType::I64() {
  return StackType{binary::ValueType::I64()};
}

// static
StackType StackType::F32() {
  return StackType{binary::ValueType::F32()};
}

// static
StackType StackType::F64() {
  return StackType{binary::ValueType::F64()};
}

// static
StackType StackType::V128() {
  return StackType{binary::ValueType::V128()};
}

// static
StackType StackType::Funcref() {
  return StackType{binary::ValueType::Funcref()};
}

// static
StackType StackType::Externref() {
  return StackType{binary::ValueType::Externref()};
}

// static
StackType StackType::Exnref() {
  return StackType{binary::ValueType::Exnref()};
}

bool StackType::is_value_type() const {
  return holds_alternative<binary::ValueType>(type);
}

bool StackType::is_any() const {
  return holds_alternative<Any>(type);
}

auto StackType::value_type() -> binary::ValueType& {
  return get<binary::ValueType>(type);
}

auto StackType::value_type() const -> const binary::ValueType& {
  return get<binary::ValueType>(type);
}

binary::ValueType ToValueType(binary::ReferenceType type) {
  return binary::ValueType{type};
}

binary::ValueType ToValueType(binary::HeapType type) {
  return binary::ValueType{
      binary::ReferenceType{binary::RefType{type, Null::Yes}}};
}

StackType ToStackType(binary::ValueType type) {
  return StackType(type);
}

StackType ToStackType(binary::ReferenceType type) {
  return ToStackType(ToValueType(type));
}

StackType ToStackType(binary::HeapType type) {
  return ToStackType(ToValueType(type));
}

StackTypeList ToStackTypeList(const binary::ValueTypeList& value_types) {
  StackTypeList result;
  for (auto value_type : value_types) {
    result.push_back(StackType{*value_type});
  }
  return result;
}

bool IsReferenceTypeOrAny(StackType type) {
  return type.is_any() ||
         (type.is_value_type() && type.value_type().is_reference_type());
}

binary::ReferenceType Canonicalize(binary::ReferenceType type) {
  if (type.is_reference_kind()) {
    switch (type.reference_kind()) {
#define WASP_V(val, Name, str, ...) \
  case ReferenceKind::Name##ref:    \
    return binary::ReferenceType{   \
        binary::RefType{binary::HeapType{HeapKind::Name}, Null::Yes}};
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/heap_kind.def"
#undef WASP_V
#undef WASP_FEATURE_V

      default:
        WASP_UNREACHABLE();
    }
  } else {
    return type;
  }
}

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_OPERATOR_EQ_NE_VARGS)
WASP_VALID_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

}  // namespace valid
}  // namespace wasp

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_STD_HASH_VARGS)
WASP_VALID_CONTAINERS(WASP_STD_HASH_CONTAINER)

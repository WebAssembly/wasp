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

namespace wasp::valid {

StackType::StackType() : type{Any{}} {}

StackType::StackType(binary::ValueType type) : type{type} {}

StackType::StackType(Any type) : type{type} {}

// static
StackType StackType::I32() {
  return StackType{binary::ValueType::I32_NoLocation()};
}

// static
StackType StackType::I64() {
  return StackType{binary::ValueType::I64_NoLocation()};
}

// static
StackType StackType::F32() {
  return StackType{binary::ValueType::F32_NoLocation()};
}

// static
StackType StackType::F64() {
  return StackType{binary::ValueType::F64_NoLocation()};
}

// static
StackType StackType::V128() {
  return StackType{binary::ValueType::V128_NoLocation()};
}

// static
StackType StackType::Funcref() {
  return StackType{binary::ValueType::Funcref_NoLocation()};
}

// static
StackType StackType::Externref() {
  return StackType{binary::ValueType::Externref_NoLocation()};
}

// static
StackType StackType::Anyref() {
  return StackType{binary::ValueType::Anyref_NoLocation()};
}

// static
StackType StackType::Eqref() {
  return StackType{binary::ValueType::Eqref_NoLocation()};
}

// static
StackType StackType::I31ref() {
  return StackType{binary::ValueType::I31ref_NoLocation()};
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

auto ToValueType(binary::StorageType type) -> binary::ValueType {
  return type.is_packed_type() ? binary::ValueType::I32_NoLocation()
                               : type.value_type().value();
}

auto ToValueType(binary::ReferenceType type) -> binary::ValueType {
  return binary::ValueType{type};
}

auto ToValueType(binary::RefType type) -> binary::ValueType {
  return binary::ValueType{binary::ReferenceType{type}};
}

auto ToValueType(binary::HeapType type) -> binary::ValueType {
  return binary::ValueType{
      binary::ReferenceType{binary::RefType{type, Null::Yes}}};
}

auto ToStackType(binary::StorageType type) -> StackType {
  return StackType(ToValueType(type));
}

auto ToStackType(binary::ValueType type) -> StackType {
  return StackType(type);
}

auto ToStackType(binary::ReferenceType type) -> StackType {
  return ToStackType(ToValueType(type));
}

auto ToStackType(binary::RefType type) -> StackType {
  return ToStackType(ToValueType(type));
}

auto ToStackType(binary::HeapType type) -> StackType {
  return ToStackType(ToValueType(type));
}

auto ToStackTypeList(const binary::ValueTypeList& value_types)
    -> StackTypeList {
  StackTypeList result;
  for (auto value_type : value_types) {
    result.push_back(StackType{*value_type});
  }
  return result;
}

auto ToStackTypeList(const binary::LocalsList& locals_list) -> StackTypeList {
  StackTypeList result;
  for (auto& locals : locals_list) {
    for (size_t i = 0; i < locals->count; ++i) {
      result.push_back(StackType{*locals->type});
    }
  }
  return result;
}

bool IsReferenceTypeOrAny(StackType type) {
  return type.is_any() ||
         (type.is_value_type() && type.value_type().is_reference_type());
}

bool IsRttOrAny(StackType type) {
  return type.is_any() || (type.is_value_type() && type.value_type().is_rtt());
}

auto Canonicalize(binary::ReferenceType type) -> binary::ReferenceType {
  if (type.is_reference_kind()) {
    switch (type.reference_kind()) {
      case ReferenceKind::Funcref:
        return binary::ReferenceType{
            binary::RefType{binary::HeapType{HeapKind::Func}, Null::Yes}};

      case ReferenceKind::Externref:
        return binary::ReferenceType{
            binary::RefType{binary::HeapType{HeapKind::Extern}, Null::Yes}};

      case ReferenceKind::Anyref:
        return binary::ReferenceType{
            binary::RefType{binary::HeapType{HeapKind::Any}, Null::Yes}};

      case ReferenceKind::Eqref:
        return binary::ReferenceType{
            binary::RefType{binary::HeapType{HeapKind::Eq}, Null::Yes}};

      case ReferenceKind::I31ref:
        // NOTE: i31ref is equivalent to (ref i31), not (ref null i31).
        return binary::ReferenceType{
            binary::RefType{binary::HeapType{HeapKind::I31}, Null::No}};

      default:
        WASP_UNREACHABLE();
    }
  } else {
    return type;
  }
}

bool IsDefaultableType(binary::RefType type) {
  return type.null == Null::Yes;
}

bool IsDefaultableType(binary::ReferenceType type) {
  return type.is_reference_kind() || IsDefaultableType(type.ref());
}

bool IsDefaultableType(binary::ValueType type) {
  return type.is_numeric_type() || IsDefaultableType(type.reference_type());
}

bool IsDefaultableType(binary::StorageType type) {
  return type.is_packed_type() || IsDefaultableType(type.value_type());
}

bool IsNullableType(binary::ValueType type) {
  // All reference types are considered "nullable" for the purpose of
  // type-checking, even non-nullable reference types. This is because nullable
  // types are a supertype of non-nullable types.
  return type.is_reference_type();
}

bool IsNullableType(StackType type) {
  return type.is_any() ||
         (type.is_value_type() && IsNullableType(type.value_type()));
}

auto AsNonNullableType(binary::RefType type) -> binary::RefType {
  return binary::RefType{type.heap_type, Null::No};
}

auto AsNonNullableType(binary::ReferenceType type) -> binary::ReferenceType {
  type = Canonicalize(type);
  assert(type.is_ref());
  return binary::ReferenceType{AsNonNullableType(type.ref())};
}

auto AsNonNullableType(binary::ValueType type) -> binary::ValueType {
  assert(IsNullableType(type));
  return binary::ValueType{AsNonNullableType(type.reference_type())};
}

auto AsNonNullableType(StackType type) -> StackType {
  assert(IsNullableType(type));
  if (type.is_any()) {
    return type;
  } else {
    return StackType{AsNonNullableType(type.value_type())};
  }
}

WASP_VALID_STRUCTS_CUSTOM_FORMAT(WASP_OPERATOR_EQ_NE_VARGS)
WASP_VALID_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

}  // namespace wasp::valid

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

#include "wasp/valid/match.h"

#include <cassert>

#include "wasp/valid/context.h"

namespace wasp::valid {

auto CanonicalizeToRefType(const binary::ReferenceType& type)
    -> binary::RefType {
  binary::ReferenceType canon = Canonicalize(type);
  assert(canon.is_ref());
  return canon.ref();
}

/// IsSame ///

bool IsSame(Context& context,
            const binary::HeapType& expected,
            const binary::HeapType& actual) {
  if (expected.is_heap_kind() && actual.is_heap_kind()) {
    return expected.heap_kind().value() == actual.heap_kind().value();
  } else if (expected.is_index() && actual.is_index()) {
    Index expected_index = expected.index().value();
    Index actual_index = actual.index().value();
    if (expected_index == actual_index) {
      return true;
    }

    auto is_same_opt = context.same_types.Get(expected_index, actual_index);
    if (is_same_opt) {
      return *is_same_opt;
    }

    // Assume that they are the same and check that everything still is valid.
    context.same_types.Assume(expected_index, actual_index);
    bool is_same = IsSame(context, context.types[expected_index],
                          context.types[actual_index]);
    context.same_types.Resolve(expected_index, actual_index, is_same);
    return is_same;
  }
  return false;
}

bool IsSame(Context& context,
            const binary::RefType& expected,
            const binary::RefType& actual) {
  return IsSame(context, expected.heap_type, actual.heap_type) &&
         expected.null == actual.null;
}

bool IsSame(Context& context,
            const binary::ReferenceType& expected,
            const binary::ReferenceType& actual) {
  // Canonicalize in case one of the types is "ref null X" and the other is
  // "Xref".
  return IsSame(context, CanonicalizeToRefType(expected),
                CanonicalizeToRefType(actual));
}

bool IsSame(Context& context,
            const binary::Rtt& expected,
            const binary::Rtt& actual) {
  return expected.depth.value() == actual.depth.value() &&
         IsSame(context, expected.type, actual.type);
}

bool IsSame(Context& context,
            const binary::ValueType& expected,
            const binary::ValueType& actual) {
  if (expected.is_numeric_type() && actual.is_numeric_type()) {
    return expected.numeric_type().value() == actual.numeric_type().value();
  } else if (expected.is_reference_type() && actual.is_reference_type()) {
    return IsSame(context, expected.reference_type(), actual.reference_type());
  } else if (expected.is_rtt() && actual.is_rtt()) {
    return IsSame(context, expected.rtt(), actual.rtt());
  }
  return false;
}

bool IsSame(Context& context,
            const binary::ValueTypeList& expected,
            const binary::ValueTypeList& actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsSame(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsSame(Context& context,
            const binary::FunctionType& expected,
            const binary::FunctionType& actual) {
  return IsSame(context, expected.param_types, actual.param_types) &&
         IsSame(context, expected.result_types, actual.result_types);
}

bool IsSame(Context& context,
            const StackType& expected,
            const StackType& actual) {
  // One of the types is "any" (i.e. universal supertype or subtype), or the
  // value types are the same.
  return expected.is_any() || actual.is_any() ||
         IsSame(context, expected.value_type(), actual.value_type());
}

bool IsSame(Context& context, StackTypeSpan expected, StackTypeSpan actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsSame(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsSame(Context& context,
            const binary::StorageType& expected,
            const binary::StorageType& actual) {
  if (expected.is_value_type() && actual.is_value_type()) {
    return IsSame(context, expected.value_type(), actual.value_type());
  } else if (expected.is_packed_type() && actual.is_packed_type()) {
    return expected.packed_type().value() == actual.packed_type().value();
  }
  return false;
}

bool IsSame(Context& context,
            const binary::FieldType& expected,
            const binary::FieldType& actual) {
  return IsSame(context, expected.type, actual.type) &&
         expected.mut.value() == actual.mut.value();
}

bool IsSame(Context& context,
            const binary::FieldTypeList& expected,
            const binary::FieldTypeList& actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsSame(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsSame(Context& context,
            const binary::StructType& expected,
            const binary::StructType& actual) {
  return IsSame(context, expected.fields, actual.fields);
}

bool IsSame(Context& context,
            const binary::ArrayType& expected,
            const binary::ArrayType& actual) {
  return IsSame(context, expected.field, actual.field);
}

bool IsSame(Context& context,
            const binary::DefinedType& expected,
            const binary::DefinedType& actual) {
  if (expected.is_function_type() && actual.is_function_type()) {
    return IsSame(context, expected.function_type(), actual.function_type());
  } else if (expected.is_struct_type() && actual.is_struct_type()) {
    return IsSame(context, expected.struct_type(), actual.struct_type());
  } else if (expected.is_array_type() && actual.is_array_type()) {
    return IsSame(context, expected.array_type(), actual.array_type());
  }
  return false;
}

/// IsMatch ///

bool IsMatch(Context& context,
             const binary::HeapType& expected,
             const binary::HeapType& actual) {
  // `any` is a supertype of all types.
  if (expected.is_heap_kind(HeapKind::Any)) {
    return true;
  }

  // `func` is a supertype of all function types.
  if (expected.is_heap_kind(HeapKind::Func) && actual.is_index() &&
      context.IsFunctionType(actual.index())) {
    return true;
  }

  // `eq` is a supertype of all i31, or any non-function defined type.
  if (expected.is_heap_kind(HeapKind::Eq)) {
    if (actual.is_heap_kind(HeapKind::I31)) {
      return true;
    } else if (actual.is_index() && (context.IsStructType(actual.index()) ||
                                     context.IsArrayType(actual.index()))) {
      return true;
    }
  }

  if (expected.is_index() && actual.is_index()) {
    Index expected_index = expected.index().value();
    Index actual_index = actual.index().value();
    if (expected_index == actual_index) {
      return true;
    }

    // Check whether heap types match, but make sure to handle recursive
    // structures. This is the same logic used in IsSame(HeapType, HeapType).

    auto is_match_opt = context.match_types.Get(expected_index, actual_index);
    if (is_match_opt) {
      return *is_match_opt;
    }

    // Assume that they match and check that everything still is valid.
    context.match_types.Assume(expected_index, actual_index);
    bool is_match = IsMatch(context, context.types[expected_index],
                            context.types[actual_index]);
    context.match_types.Resolve(expected_index, actual_index, is_match);
    return is_match;
  }

  return IsSame(context, expected, actual);
}

bool IsMatch(Context& context,
             const binary::RefType& expected,
             const binary::RefType& actual) {
  // "ref null 0" is a supertype of "ref 0"
  if (expected.null == Null::No && actual.null == Null::Yes) {
    return false;
  }
  return IsMatch(context, expected.heap_type.value(), actual.heap_type);
}

bool IsMatch(Context& context,
             const binary::ReferenceType& expected,
             const binary::ReferenceType& actual) {
  // Canonicalize in case one of the types is "ref null X" and the other is
  // "Xref".
  return IsMatch(context, CanonicalizeToRefType(expected),
                 CanonicalizeToRefType(actual));
}

bool IsMatch(Context& context,
             const binary::Rtt& expected,
             const binary::Rtt& actual) {
  // Rtt's don't have any subtyping, aside from rtt <: any
  return IsSame(context, expected, actual);
}

bool IsMatch(Context& context,
             const binary::ValueType& expected,
             const binary::ValueType& actual) {
  if (expected.is_numeric_type() && actual.is_numeric_type()) {
    return expected.numeric_type().value() == actual.numeric_type().value();
  } else if (expected.is_reference_type() && actual.is_reference_type()) {
    return IsMatch(context, expected.reference_type(), actual.reference_type());
  } else if (expected.is_rtt() && actual.is_rtt()) {
    return IsMatch(context, expected.rtt(), actual.rtt());
  }
  return false;
}

bool IsMatch(Context& context,
             const binary::ValueTypeList& expected,
             const binary::ValueTypeList& actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsMatch(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsMatch(Context& context,
             const StackType& expected,
             const StackType& actual) {
  // One of the types is "any" (i.e. universal supertype or subtype), or the
  // value types match.
  return expected.is_any() || actual.is_any() ||
         IsMatch(context, expected.value_type(), actual.value_type());
}

bool IsMatch(Context& context, StackTypeSpan expected, StackTypeSpan actual) {
  if (expected.size() != actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsMatch(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsMatch(Context& context,
             const binary::StorageType& expected,
             const binary::StorageType& actual) {
  if (expected.is_value_type() && actual.is_value_type()) {
    return IsMatch(context, expected.value_type(), actual.value_type());
  } else if (expected.is_packed_type() && actual.is_packed_type()) {
    return expected.packed_type().value() == actual.packed_type().value();
  }
  return false;
}

bool IsMatch(Context& context,
             const binary::FieldType& expected,
             const binary::FieldType& actual) {
  // Only const fields are covariant.
  if (expected.mut == Mutability::Const && actual.mut == Mutability::Const) {
    return IsMatch(context, expected.type, actual.type);
  }
  return IsSame(context, expected, actual);
}

bool IsMatch(Context& context,
             const binary::FieldTypeList& expected,
             const binary::FieldTypeList& actual) {
  // A longer list of fields is a subtype of a shorter list, as long as the
  // types match.
  if (expected.size() > actual.size()) {
    return false;
  }

  for (auto eiter = expected.begin(), lend = expected.end(),
            aiter = actual.begin();
       eiter != lend; ++eiter, ++aiter) {
    if (!IsMatch(context, *eiter, *aiter)) {
      return false;
    }
  }
  return true;
}

bool IsMatch(Context& context,
             const binary::FunctionType& expected,
             const binary::FunctionType& actual) {
  // Function types have no subtyping, so just check if they are equivalent.
  return IsSame(context, expected, actual);
}

bool IsMatch(Context& context,
             const binary::StructType& expected,
             const binary::StructType& actual) {
  return IsMatch(context, expected.fields, actual.fields);
}

bool IsMatch(Context& context,
             const binary::ArrayType& expected,
             const binary::ArrayType& actual) {
  return IsMatch(context, expected.field, actual.field);
}

bool IsMatch(Context& context,
             const binary::DefinedType& expected,
             const binary::DefinedType& actual) {
  if (expected.is_function_type() && actual.is_function_type()) {
    return IsSame(context, expected.function_type(), actual.function_type());
  } else if (expected.is_struct_type() && actual.is_struct_type()) {
    return IsMatch(context, expected.struct_type(), actual.struct_type());
  } else if (expected.is_array_type() && actual.is_array_type()) {
    return IsMatch(context, expected.array_type(), actual.array_type());
  }
  return false;
}

}  // namespace wasp::valid

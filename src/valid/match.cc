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

    auto is_equ_opt =
        context.equivalent_types.Get(expected_index, actual_index);
    if (is_equ_opt) {
      return *is_equ_opt;
    }

    // Assume that they are the same and check that everything still is valid.
    context.equivalent_types.Assume(expected_index, actual_index);
    bool is_same = IsSame(context, context.types[expected_index],
                          context.types[actual_index]);
    context.equivalent_types.Resolve(expected_index, actual_index, is_same);
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
            const binary::ValueType& expected,
            const binary::ValueType& actual) {
  if (expected.is_numeric_type() && actual.is_numeric_type()) {
    return expected.numeric_type().value() == actual.numeric_type().value();
  } else if (expected.is_reference_type() && actual.is_reference_type()) {
    return IsSame(context, expected.reference_type(), actual.reference_type());
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
            const binary::DefinedType& expected,
            const binary::DefinedType& actual) {
  return IsSame(context, expected.type, actual.type);
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

bool IsMatch(Context& context,
             const binary::HeapType& expected,
             const binary::HeapType& actual) {
  // "func" is a supertype of all function types.
  if (expected.is_heap_kind() && expected.heap_kind() == HeapKind::Func &&
      actual.is_index()) {
    return true;
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
             const binary::ValueType& expected,
             const binary::ValueType& actual) {
  if (expected.is_numeric_type() && actual.is_numeric_type()) {
    return expected.numeric_type().value() == actual.numeric_type().value();
  } else if (expected.is_reference_type() && actual.is_reference_type()) {
    return IsMatch(context, expected.reference_type(), actual.reference_type());
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

}  // namespace wasp::valid

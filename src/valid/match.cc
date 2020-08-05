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

namespace wasp {
namespace valid {

bool IsMatch(Context& context,
             const binary::HeapType& expected,
             const binary::HeapType& actual) {
  // "func" is a supertype of all function types.
  if (expected.is_heap_kind() && expected.heap_kind() == HeapKind::Func &&
      actual.is_index()) {
    return true;
  }

  if (expected.is_heap_kind() && actual.is_heap_kind()) {
    return expected.heap_kind().value() == actual.heap_kind().value();
  } else if (expected.is_index() && actual.is_index()) {
    return expected.index().value() == actual.index().value();
  }
  return false;
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
  // Canoicalize in case one of the types is "ref null X" and the other is
  // "Xref".
  binary::ReferenceType expected_canon = Canonicalize(expected);
  binary::ReferenceType actual_canon = Canonicalize(actual);

  assert(expected_canon.is_ref() && actual_canon.is_ref());
  return IsMatch(context, expected_canon.ref(), actual_canon.ref());
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
    StackType etype = *eiter;
    StackType atype = *aiter;
    if (!IsMatch(context, etype, atype)) {
      return false;
    }
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

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

#ifndef WASP_VALID_MATCH_H_
#define WASP_VALID_MATCH_H_

#include "wasp/base/types.h"
#include "wasp/valid/types.h"

namespace wasp {
namespace valid {

struct Context;

bool IsSame(Context&,
            const binary::HeapType& expected,
            const binary::HeapType& actual);
bool IsSame(Context&,
            const binary::RefType& expected,
            const binary::RefType& actual);
bool IsSame(Context&,
            const binary::ReferenceType& expected,
            const binary::ReferenceType& actual);
bool IsSame(Context&,
            const binary::ValueType& expected,
            const binary::ValueType& actual);
bool IsSame(Context&,
            const binary::ValueTypeList& expected,
            const binary::ValueTypeList& actual);
bool IsSame(Context& context,
            const binary::TypeEntry& expected,
            const binary::TypeEntry& actual);
bool IsSame(Context&, const StackType& expected, const StackType& actual);
bool IsSame(Context&, StackTypeSpan expected, StackTypeSpan actual);

bool IsMatch(Context&,
             const binary::HeapType& expected,
             const binary::HeapType& actual);
bool IsMatch(Context&,
             const binary::RefType& expected,
             const binary::RefType& actual);
bool IsMatch(Context&,
             const binary::ReferenceType& expected,
             const binary::ReferenceType& actual);
bool IsMatch(Context&,
             const binary::ValueType& expected,
             const binary::ValueType& actual);
bool IsMatch(Context&, const StackType& expected, const StackType& actual);
bool IsMatch(Context&, StackTypeSpan expected, StackTypeSpan actual);

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_MATCH_H_

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

namespace wasp::valid {

struct ValidCtx;

bool IsSame(ValidCtx&,
            const binary::HeapType& expected,
            const binary::HeapType& actual);
bool IsSame(ValidCtx&,
            const binary::RefType& expected,
            const binary::RefType& actual);
bool IsSame(ValidCtx&,
            const binary::ReferenceType& expected,
            const binary::ReferenceType& actual);
bool IsSame(ValidCtx&, const binary::Rtt& expected, const binary::Rtt& actual);
bool IsSame(ValidCtx&,
            const binary::ValueType& expected,
            const binary::ValueType& actual);
bool IsSame(ValidCtx&,
            const binary::ValueTypeList& expected,
            const binary::ValueTypeList& actual);
bool IsSame(ValidCtx&, const StackType& expected, const StackType& actual);
bool IsSame(ValidCtx&, StackTypeSpan expected, StackTypeSpan actual);
bool IsSame(ValidCtx& ctx,
            const binary::StorageType& expected,
            const binary::StorageType& actual);
bool IsSame(ValidCtx& ctx,
            const binary::FieldType& expected,
            const binary::FieldType& actual);
bool IsSame(ValidCtx& ctx,
            const binary::FieldTypeList& expected,
            const binary::FieldTypeList& actual);
bool IsSame(ValidCtx& ctx,
            const binary::FunctionType& expected,
            const binary::FunctionType& actual);
bool IsSame(ValidCtx& ctx,
            const binary::StructType& expected,
            const binary::StructType& actual);
bool IsSame(ValidCtx& ctx,
            const binary::ArrayType& expected,
            const binary::ArrayType& actual);
bool IsSame(ValidCtx& ctx,
            const binary::DefinedType& expected,
            const binary::DefinedType& actual);

bool IsMatch(ValidCtx&,
             const binary::HeapType& expected,
             const binary::HeapType& actual);
bool IsMatch(ValidCtx&,
             const binary::RefType& expected,
             const binary::RefType& actual);
bool IsMatch(ValidCtx&,
             const binary::ReferenceType& expected,
             const binary::ReferenceType& actual);
bool IsMatch(ValidCtx&, const binary::Rtt& expected, const binary::Rtt& actual);
bool IsMatch(ValidCtx&,
             const binary::ValueType& expected,
             const binary::ValueType& actual);
bool IsMatch(ValidCtx&,
             const binary::ValueTypeList& expected,
             const binary::ValueTypeList& actual);
bool IsMatch(ValidCtx&, const StackType& expected, const StackType& actual);
bool IsMatch(ValidCtx&, StackTypeSpan expected, StackTypeSpan actual);
bool IsMatch(ValidCtx& ctx,
             const binary::StorageType& expected,
             const binary::StorageType& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::FieldType& expected,
             const binary::FieldType& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::FieldTypeList& expected,
             const binary::FieldTypeList& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::FunctionType& expected,
             const binary::FunctionType& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::StructType& expected,
             const binary::StructType& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::ArrayType& expected,
             const binary::ArrayType& actual);
bool IsMatch(ValidCtx& ctx,
             const binary::DefinedType& expected,
             const binary::DefinedType& actual);

}  // namespace wasp::valid

#endif  // WASP_VALID_MATCH_H_

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

#ifndef WASP_VALID_VALIDATE_H_
#define WASP_VALID_VALIDATE_H_

#include "wasp/base/types.h"
#include "wasp/valid/types.h"

namespace wasp::valid {

enum class RequireDefaultable {
  No,
  Yes,
};

struct ValidCtx;

bool BeginTypeSection(ValidCtx&, Index type_count);
bool EndTypeSection(ValidCtx&);
bool BeginCode(ValidCtx&, Location loc);

bool CheckDefaultable(ValidCtx&,
                      const At<binary::ReferenceType>&,
                      string_view desc);
bool CheckDefaultable(ValidCtx&,
                      const At<binary::ValueType>&,
                      string_view desc);
bool CheckDefaultable(ValidCtx&,
                      const At<binary::StorageType>&,
                      string_view desc);

bool Validate(ValidCtx&, const At<binary::ArrayType>&);
bool Validate(ValidCtx&, const At<binary::BlockType>&);
bool Validate(ValidCtx&, const At<binary::DataSegment>&);
bool Validate(ValidCtx&,
              const At<binary::ConstantExpression>&,
              binary::ValueType expected_type,
              Index max_global_index);
bool Validate(ValidCtx&, const At<binary::DataCount>&);
bool Validate(ValidCtx&, const At<binary::DataSegment>&);
bool Validate(ValidCtx&, const At<binary::DefinedType>&);
bool Validate(ValidCtx&,
              const At<binary::ElementExpression>&,
              binary::ReferenceType);
bool Validate(ValidCtx&, const At<binary::ElementSegment>&);
bool Validate(ValidCtx&, const At<binary::Export>&);
bool Validate(ValidCtx&, const At<binary::Event>&);
bool Validate(ValidCtx&, const At<binary::EventType>&);
bool Validate(ValidCtx&, const At<binary::FieldType>&);
bool Validate(ValidCtx&, const binary::FieldTypeList&);
bool Validate(ValidCtx&, const At<binary::Function>&);
bool Validate(ValidCtx&, const At<binary::FunctionType>&);
bool Validate(ValidCtx&, const At<binary::Global>&);
bool Validate(ValidCtx&, const At<binary::GlobalType>&);
bool Validate(ValidCtx&, const At<binary::HeapType>&);
bool Validate(ValidCtx&, const At<binary::Import>&);
bool ValidateIndex(ValidCtx&,
                   const At<Index>& index,
                   Index max,
                   string_view desc);
bool ValidateTypeIndex(ValidCtx&, const At<Index>& index);
bool ValidateFunctionIndex(ValidCtx&, const At<Index>& index);
bool ValidateMemoryIndex(ValidCtx&, const At<Index>& index);
bool ValidateTableIndex(ValidCtx&, const At<Index>& index);
bool ValidateGlobalIndex(ValidCtx&, const At<Index>& index);
bool ValidateEventIndex(ValidCtx&, const At<Index>& index);
bool Validate(ValidCtx&, const At<binary::Instruction>&);
bool Validate(ValidCtx&, const At<Limits>&, Index max);
bool Validate(ValidCtx&, const At<binary::Locals>&, RequireDefaultable);
bool Validate(ValidCtx&, const At<binary::LocalsList>&, RequireDefaultable);
bool Validate(ValidCtx&, const At<binary::Memory>&);
bool Validate(ValidCtx&, const At<MemoryType>&);
bool Validate(ValidCtx&, const At<binary::ReferenceType>&);
bool Validate(ValidCtx&, const At<binary::RefType>&);
bool Validate(ValidCtx&,
              binary::ReferenceType expected,
              const At<binary::ReferenceType>& actual);
bool Validate(ValidCtx&, const At<binary::Rtt>&);
bool Validate(ValidCtx&, const At<binary::Start>& value);
bool Validate(ValidCtx&, const At<binary::StorageType>& value);
bool Validate(ValidCtx&, const At<binary::StructType>& value);
bool Validate(ValidCtx&, const At<binary::Table>&);
bool Validate(ValidCtx&, const At<binary::TableType>&);
bool Validate(ValidCtx&, const At<binary::ValueType>&);
bool Validate(ValidCtx&,
              binary::ValueType expected,
              const At<binary::ValueType>& actual);
bool Validate(ValidCtx&, const binary::ValueTypeList&);
bool Validate(ValidCtx&, const At<binary::UnpackedCode>&);
bool Validate(ValidCtx&, const At<binary::UnpackedExpression>&);

bool Validate(ValidCtx&, const binary::Module&);

}  // namespace wasp::valid

#endif  // WASP_VALID_VALIDATE_H_

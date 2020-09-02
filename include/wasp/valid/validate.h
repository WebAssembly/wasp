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

struct Context;

bool BeginTypeSection(Context&, Index type_count);
bool BeginCode(Context&, Location loc);

bool CheckDefaultable(Context&,
                      const At<binary::ReferenceType>&,
                      string_view desc);
bool CheckDefaultable(Context&, const At<binary::ValueType>&, string_view desc);

bool Validate(Context&, const At<binary::ArrayType>&);
bool Validate(Context&, const At<binary::BlockType>&);
bool Validate(Context&, const At<binary::DataSegment>&);
bool Validate(Context&,
              const At<binary::ConstantExpression>&,
              binary::ValueType expected_type,
              Index max_global_index);
bool Validate(Context&, const At<binary::DataCount>&);
bool Validate(Context&, const At<binary::DataSegment>&);
bool Validate(Context&, const At<binary::DefinedType>&);
bool Validate(Context&,
              const At<binary::ElementExpression>&,
              binary::ReferenceType);
bool Validate(Context&, const At<binary::ElementSegment>&);
bool Validate(Context&, const At<binary::Export>&);
bool Validate(Context&, const At<binary::Event>&);
bool Validate(Context&, const At<binary::EventType>&);
bool Validate(Context&, const At<binary::FieldType>&);
bool Validate(Context&, const binary::FieldTypeList&);
bool Validate(Context&, const At<binary::Function>&);
bool Validate(Context&, const At<binary::FunctionType>&);
bool Validate(Context&, const At<binary::Global>&);
bool Validate(Context&, const At<binary::GlobalType>&);
bool Validate(Context&, const At<binary::HeapType>&);
bool Validate(Context&, const At<binary::Import>&);
bool ValidateIndex(Context&,
                   const At<Index>& index,
                   Index max,
                   string_view desc);
bool Validate(Context&, const At<binary::Instruction>&);
bool Validate(Context&, const At<Limits>&, Index max);
bool Validate(Context&, const At<binary::Locals>&, RequireDefaultable);
bool Validate(Context&, const At<binary::LocalsList>&, RequireDefaultable);
bool Validate(Context&, const At<binary::Memory>&);
bool Validate(Context&, const At<MemoryType>&);
bool Validate(Context&, const At<binary::ReferenceType>&);
bool Validate(Context&, const At<binary::RefType>&);
bool Validate(Context&,
              binary::ReferenceType expected,
              const At<binary::ReferenceType>& actual);
bool Validate(Context&, const At<binary::Rtt>&);
bool Validate(Context&, const At<binary::Start>& value);
bool Validate(Context&, const At<binary::StorageType>& value);
bool Validate(Context&, const At<binary::StructType>& value);
bool Validate(Context&, const At<binary::Table>&);
bool Validate(Context&, const At<binary::TableType>&);
bool Validate(Context&, const At<binary::ValueType>&);
bool Validate(Context&,
              binary::ValueType expected,
              const At<binary::ValueType>& actual);
bool Validate(Context&, const binary::ValueTypeList&);
bool Validate(Context&, const At<binary::UnpackedCode>&);
bool Validate(Context&, const At<binary::UnpackedExpression>&);

bool Validate(Context&, const binary::Module&);

}  // namespace wasp::valid

#endif  // WASP_VALID_VALIDATE_H_

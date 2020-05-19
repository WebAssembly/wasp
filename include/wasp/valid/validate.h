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
#include "wasp/binary/types.h"

namespace wasp {

class Errors;

namespace valid {

enum class ConstantExpressionKind {
  GlobalInit,
  Other,
};

struct Context;

bool Validate(Context&, const At<binary::Code>&, Errors& read_errors);
bool Validate(Context&, const At<binary::Code>&);
bool Validate(Context&, const At<binary::DataSegment>&);
bool Validate(Context&,
              const At<binary::ConstantExpression>&,
              ConstantExpressionKind kind,
              ValueType expected_type,
              Index max_global_index);
bool Validate(Context&, const At<binary::DataCount>&);
bool Validate(Context&, const At<binary::DataSegment>&);
bool Validate(Context&, const At<binary::ElementExpression>&, ReferenceType);
bool Validate(Context&, const At<binary::ElementSegment>&);
bool Validate(Context&,
              const At<ReferenceType>& actual,
              ReferenceType expected);
bool Validate(Context&, const At<binary::Export>&);
bool Validate(Context&, const At<binary::Event>&);
bool Validate(Context&, const At<binary::EventType>&);
bool Validate(Context&, const At<binary::Function>&);
bool Validate(Context&, const At<binary::FunctionType>&);
bool Validate(Context&, const At<binary::Global>&);
bool Validate(Context&, const At<GlobalType>&);
bool Validate(Context&, const At<binary::Import>&);
bool ValidateIndex(Context&,
                   const At<Index>& index,
                   Index max,
                   string_view desc);
bool Validate(Context&, const At<binary::Instruction>&);
bool Validate(Context&, const At<Limits>&, Index max);
bool Validate(Context&, const At<binary::Locals>&);
bool Validate(Context&, const At<binary::Memory>&);
bool Validate(Context&, const At<MemoryType>&);
bool Validate(Context&, const At<binary::Start>& value);
bool Validate(Context&, const At<binary::Table>&);
bool Validate(Context&, const At<TableType>&);
bool Validate(Context&, const At<binary::TypeEntry>&);
bool Validate(Context&, const At<ValueType>& actual, ValueType expected);
bool EndModule(Context&);
bool Validate(Context&, const binary::Module&);

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_H_

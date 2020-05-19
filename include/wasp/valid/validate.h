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

bool Validate(const At<binary::Code>&, Context&, Errors& read_errors);
bool Validate(const At<binary::Code>&, Context&);

bool Validate(const At<binary::DataSegment>&, Context&);

bool Validate(const At<binary::ConstantExpression>&,
              ConstantExpressionKind kind,
              ValueType expected_type,
              Index max_global_index,
              Context&);

bool Validate(const At<binary::DataCount>&, Context&);

bool Validate(const At<binary::DataSegment>&, Context&);

bool Validate(const At<binary::ElementExpression>&, ReferenceType, Context&);

bool Validate(const At<binary::ElementSegment>&, Context&);

bool Validate(const At<ReferenceType>& actual,
              ReferenceType expected,
              Context&);

bool Validate(const At<binary::Export>&, Context&);

bool Validate(const At<binary::Event>&, Context&);

bool Validate(const At<binary::EventType>&, Context&);

bool Validate(const At<binary::Function>&, Context&);

bool Validate(const At<binary::FunctionType>&, Context&);

bool Validate(const At<binary::Global>&, Context&);

bool Validate(const At<GlobalType>&, Context&);

bool Validate(const At<binary::Import>&, Context&);

bool ValidateIndex(const At<Index>& index,
                   Index max,
                   string_view desc,
                   Context&);

bool Validate(const At<binary::Instruction>&, Context&);

bool Validate(const At<Limits>&, Index max, Context&);

bool Validate(const At<binary::Locals>&, Context&);

bool Validate(const At<binary::Memory>&, Context&);

bool Validate(const At<MemoryType>&, Context&);

bool Validate(const At<binary::Start>& value, Context& context);

bool Validate(const At<binary::Table>&, Context&);

bool Validate(const At<TableType>&, Context&);

bool Validate(const At<binary::TypeEntry>&, Context&);

bool Validate(const At<ValueType>& actual, ValueType expected, Context&);

bool EndModule(Context&);

bool Validate(const binary::Module&, Context&);

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_H_

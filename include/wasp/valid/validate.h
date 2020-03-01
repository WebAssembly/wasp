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

namespace valid {

struct Context;
class Errors;

bool Validate(const binary::DataSegment&, Context&);

bool Validate(const binary::ConstantExpression&,
              binary::ValueType expected_type,
              Index max_global_index,
              Context&);

bool Validate(const binary::DataCount&, Context&);

bool Validate(const binary::DataSegment&, Context&);

bool Validate(const binary::ElementExpression&, binary::ElementType, Context&);

bool Validate(const binary::ElementSegment&, Context&);

bool Validate(binary::ElementType actual,
              binary::ElementType expected,
              Context&);

bool Validate(const binary::Export&, Context&);

bool Validate(const binary::Event&, Context&);

bool Validate(const binary::EventType&, Context&);

bool Validate(const binary::Function&, Context&);

bool Validate(const binary::FunctionType&, Context&);

bool Validate(const binary::Global&, Context&);

bool Validate(const binary::GlobalType&, Context&);

bool Validate(const binary::Import&, Context&);

bool ValidateIndex(Index index, Index max, string_view desc, Context&);

bool Validate(const binary::Instruction&, Context&);

bool Validate(const binary::Limits&, Index max, Context&);

bool Validate(const binary::Locals&, Context&);

bool Validate(const binary::Memory&, Context&);

bool Validate(const binary::MemoryType&, Context&);

bool Validate(const binary::Start& value, Context& context);

bool Validate(const binary::Table&, Context&);

bool Validate(const binary::TableType&, Context&);

bool Validate(const binary::TypeEntry&, Context&);

bool Validate(binary::ValueType actual, binary::ValueType expected, Context&);

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_H_

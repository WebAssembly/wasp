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

class Features;

namespace valid {

struct Context;
class Errors;

bool Validate(const binary::DataSegment&, Context&, const Features&, Errors&);

bool Validate(const binary::ConstantExpression&,
              binary::ValueType expected_type,
              Index max_global_index,
              Context&,
              const Features&,
              Errors&);

bool Validate(const binary::DataCount&, Context&, const Features&, Errors&);

bool Validate(const binary::DataSegment&, Context&, const Features&, Errors&);

bool Validate(const binary::ElementExpression&,
              binary::ElementType,
              Context&,
              const Features&,
              Errors&);

bool Validate(const binary::ElementSegment&,
              Context&,
              const Features&,
              Errors&);

bool Validate(binary::ElementType actual,
              binary::ElementType expected,
              Context&,
              const Features&,
              Errors&);

bool Validate(const binary::Export&, Context&, const Features&, Errors&);

bool Validate(const binary::Function&, Context&, const Features&, Errors&);

bool Validate(const binary::FunctionType&, Context&, const Features&, Errors&);

bool Validate(const binary::Global&, Context&, const Features&, Errors&);

bool Validate(const binary::GlobalType&, Context&, const Features&, Errors&);

bool Validate(const binary::Import&, Context&, const Features&, Errors&);

bool ValidateIndex(Index index, Index max, string_view desc, Errors&);

bool Validate(const binary::Instruction&, Context&, const Features&, Errors&);

bool Validate(const binary::Limits&,
              Index max,
              Context&,
              const Features&,
              Errors&);

bool Validate(const binary::Locals&, Context&, const Features&, Errors&);

bool Validate(const binary::Memory&, Context&, const Features&, Errors&);

bool Validate(const binary::MemoryType&, Context&, const Features&, Errors&);

bool Validate(const binary::Section&, Context&, const Features&, Errors&);

bool Validate(const binary::Start& value,
              Context& context,
              const Features& features,
              Errors& errors);

bool Validate(const binary::Table&, Context&, const Features&, Errors&);

bool Validate(const binary::TableType&, Context&, const Features&, Errors&);

bool Validate(const binary::TypeEntry&, Context&, const Features&, Errors&);

bool Validate(binary::ValueType actual,
              binary::ValueType expected,
              Context&,
              const Features&,
              Errors&);

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_H_

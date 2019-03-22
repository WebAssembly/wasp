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

#ifndef WASP_VALID_VALIDATE_INSTRUCTION_H_
#define WASP_VALID_VALIDATE_INSTRUCTION_H_

#include <cassert>

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/macros.h"
#include "wasp/binary/instruction.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"

namespace wasp {
namespace valid {

void MarkUnreachable(Context&);
void PushLabel(Context&, LabelType, binary::BlockType);

template <typename Errors>
bool Validate(const binary::Instruction& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, "instruction"};
  switch (value.opcode) {
    case binary::Opcode::Unreachable:
      MarkUnreachable(context);
      return true;

    case binary::Opcode::Nop:
      return true;

    case binary::Opcode::Block:
      PushLabel(context, LabelType::Block, value.block_type_immediate());
      return true;

    case binary::Opcode::Loop:
      PushLabel(context, LabelType::Loop, value.block_type_immediate());
      return true;

    default:
      WASP_UNREACHABLE();
      return false;
  }
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_INSTRUCTION_H_

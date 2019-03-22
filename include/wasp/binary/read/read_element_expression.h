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

#ifndef WASP_BINARY_READ_READ_ELEMENT_EXPRESSION_H_
#define WASP_BINARY_READ_READ_ELEMENT_EXPRESSION_H_

#include "wasp/base/features.h"
#include "wasp/binary/element_expression.h"
#include "wasp/binary/errors.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_instruction.h"

namespace wasp {
namespace binary {

inline optional<ElementExpression> Read(SpanU8* data,
                                        const Features& features,
                                        Errors& errors,
                                        Tag<ElementExpression>) {
  ErrorsContextGuard guard{errors, *data, "element expression"};
  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types and
  // function references proposals, but their encoding is still used by the
  // bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  new_features.enable_function_references();
  WASP_TRY_READ(instr, Read<Instruction>(data, new_features, errors));
  switch (instr.opcode) {
    case Opcode::RefNull:
    case Opcode::RefFunc:
      // OK.
      break;

    default:
      errors.OnError(
          *data,
          format("Illegal instruction in element expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, features, errors));
  if (end.opcode != Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ElementExpression{instr};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_ELEMENT_EXPRESSION_H_

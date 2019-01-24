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

#ifndef WASP_BINARY_READ_READ_CONSTANT_EXPRESSION_H_
#define WASP_BINARY_READ_READ_CONSTANT_EXPRESSION_H_

#include "wasp/base/features.h"
#include "wasp/binary/constant_expression.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_instruction.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<ConstantExpression> Read(SpanU8* data,
                                  const Features& features,
                                  Errors& errors,
                                  Tag<ConstantExpression>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "constant expression"};
  WASP_TRY_READ(instr, Read<Instruction>(data, features, errors));
  switch (instr.opcode) {
    case Opcode::I32Const:
    case Opcode::I64Const:
    case Opcode::F32Const:
    case Opcode::F64Const:
    case Opcode::GlobalGet:
      // OK.
      break;

    default:
      errors.OnError(
          *data,
          format("Illegal instruction in constant expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, features, errors));
  if (end.opcode != Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ConstantExpression{instr};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_CONSTANT_EXPRESSION_H_

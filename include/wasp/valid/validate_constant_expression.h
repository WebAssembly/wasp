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

#ifndef WASP_VALID_VALIDATE_CONSTANT_EXPRESSION_H_
#define WASP_VALID_VALIDATE_CONSTANT_EXPRESSION_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/binary/constant_expression.h"
#include "wasp/binary/value_type.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_index.h"
#include "wasp/valid/validate_value_type.h"

namespace wasp {
namespace valid {

inline bool Validate(const binary::ConstantExpression& value,
                     binary::ValueType expected_type,
                     Index max_global_index,
                     Context& context,
                     const Features& features,
                     Errors& errors) {
  ErrorsContextGuard guard{errors, "constant_expression"};
  bool valid = true;
  binary::ValueType actual_type;
  switch (value.instruction.opcode) {
    case binary::Opcode::I32Const:
      actual_type = binary::ValueType::I32;
      break;

    case binary::Opcode::I64Const:
      actual_type = binary::ValueType::I64;
      break;

    case binary::Opcode::F32Const:
      actual_type = binary::ValueType::F32;
      break;

    case binary::Opcode::F64Const:
      actual_type = binary::ValueType::F64;
      break;

    case binary::Opcode::GlobalGet: {
      auto index = value.instruction.index_immediate();
      if (!ValidateIndex(index, max_global_index, "global index", errors)) {
        return false;
      }

      const auto& global = context.globals[index];
      actual_type = global.valtype;

      if (context.globals[index].mut == binary::Mutability::Var) {
        errors.OnError("A constant expression cannot contain a mutable global");
        valid = false;
      }
      break;
    }

    default:
      errors.OnError(format("Invalid instruction in constant expression: {}",
                            value.instruction));
      return false;
  }

  valid &= Validate(actual_type, expected_type, context, features, errors);
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_CONSTANT_EXPRESSION_H_

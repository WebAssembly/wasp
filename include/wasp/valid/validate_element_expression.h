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

#ifndef WASP_VALID_VALIDATE_ELEMENT_EXPRESSION_H_
#define WASP_VALID_VALIDATE_ELEMENT_EXPRESSION_H_

#include <cassert>

#include "wasp/base/features.h"
#include "wasp/binary/element_expression.h"
#include "wasp/binary/element_type.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_index.h"

namespace wasp {
namespace valid {

template <typename Errors>
bool Validate(const binary::ElementExpression& value,
              binary::ElementType element_type,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, "element expression"};
  bool valid = true;
  binary::ElementType actual_type;
  switch (value.instruction.opcode) {
    case binary::Opcode::RefNull:
      actual_type = binary::ElementType::Funcref;
      break;

    case binary::Opcode::RefFunc: {
      actual_type = binary::ElementType::Funcref;
      if (!ValidateIndex(value.instruction.index_immediate(),
                         context.functions.size(), "function index", errors)) {
        valid = false;
      }
      break;
    }

    default:
      errors.OnError(format("Invalid instruction in element expression: {}",
                            value.instruction));
      return false;
  }

  valid &= Validate(actual_type, element_type, context, features, errors);
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_ELEMENT_EXPRESSION_H_

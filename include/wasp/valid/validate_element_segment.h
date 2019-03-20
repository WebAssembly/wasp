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

#ifndef WASP_VALID_VALIDATE_ELEMENT_SEGMENT_H_
#define WASP_VALID_VALIDATE_ELEMENT_SEGMENT_H_

#include "wasp/base/features.h"
#include "wasp/binary/element_segment.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"
#include "wasp/valid/validate_constant_expression.h"
#include "wasp/valid/validate_element_expression.h"
#include "wasp/valid/validate_index.h"

namespace wasp {
namespace valid {

template <typename Errors>
bool Validate(const binary::ElementSegment& value,
              Context& context,
              const Features& features,
              Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, "element segment"};
  bool valid = true;
  if (value.is_active()) {
    const auto& active = value.active();
    valid &= ValidateIndex(active.table_index, context.tables.size(),
                           "table index", errors);
    valid &= Validate(active.offset, binary::ValueType::I32,
                      context.globals.size(), context, features, errors);
    for (auto func_index : active.init) {
      valid &= ValidateIndex(func_index, context.functions.size(),
                             "function index", errors);
    }
  } else {
    const auto& passive = value.passive();
    for (const auto& element_expr : passive.init) {
      valid &= Validate(element_expr, passive.element_type, context, features,
                        errors);
    }
  }
  return valid;
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_ELEMENT_SEGMENT_H_

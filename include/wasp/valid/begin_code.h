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

#ifndef WASP_VALID_BEGIN_CODE_H_
#define WASP_VALID_BEGIN_CODE_H_

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/types.h"
#include "wasp/binary/errors.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors_context_guard.h"

namespace wasp {
namespace valid {

inline bool BeginCode(Location loc, Context& context) {
  Index func_index = context.imported_function_count + context.code_count;
  if (func_index >= context.functions.size()) {
    context.errors->OnError(
        loc, format("Unexpected code index {}, function count is {}",
                    func_index, context.functions.size()));
    return false;
  }
  context.code_count++;
  const binary::Function& function = context.functions[func_index];
  context.type_stack.clear();
  context.label_stack.clear();
  context.locals_partial_sum.clear();
  context.locals.clear();
  // Don't validate the index, should have already been validated at this point.
  if (function.type_index < context.types.size()) {
    const binary::TypeEntry& type_entry = context.types[function.type_index];
    context.AppendLocals(type_entry.type->param_types);
    context.label_stack.push_back(
        Label{LabelType::Function, ToStackTypes(type_entry.type->param_types),
              ToStackTypes(type_entry.type->result_types), 0});
    return true;
  } else {
    // Not valid, but try to continue anyway.
    context.label_stack.push_back(Label{LabelType::Function, {}, {}, 0});
    return false;
  }
}

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_BEGIN_CODE_H_

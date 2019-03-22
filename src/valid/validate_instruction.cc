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

#include "wasp/valid/validate_instruction.h"

#include "wasp/binary/block_type.h"

namespace wasp {
namespace valid {

void MarkUnreachable(Context& context) {
  assert(!context.label_stack.empty());
  context.label_stack.back().unreachable = true;
}

void PushLabel(Context& context,
               LabelType label_type,
               binary::BlockType block_type) {
  binary::ValueTypes param_types;
  binary::ValueTypes result_types;
  switch (block_type) {
    case binary::BlockType::Void:
      break;

#define WASP_V(val, Name, str)                       \
  case binary::BlockType::Name:                      \
    result_types.push_back(binary::ValueType::Name); \
    break;
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V

    default:
      // TODO multi-value returns
      break;
  }
  context.label_stack.emplace_back(label_type, param_types, result_types,
                                   context.type_stack.size());
}

}  // namespace valid
}  // namespace wasp

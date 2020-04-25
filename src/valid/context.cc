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

#include "wasp/valid/context.h"

#include <algorithm>
#include <limits>

namespace wasp {
namespace valid {

Label::Label(LabelType label_type,
             StackTypeSpan param_types,
             StackTypeSpan result_types,
             Index type_stack_limit)
    : label_type{label_type},
      param_types{param_types.begin(), param_types.end()},
      result_types{result_types.begin(), result_types.end()},
      type_stack_limit{type_stack_limit},
      unreachable{false} {}

Context::Context(Errors& errors) : errors{&errors} {}

Context::Context(const Features& features, Errors& errors)
    : features{features}, errors{&errors} {}

Context::Context(const Context& other, Errors& errors) {
  *this = other;
  this->errors = &errors;
}

void Context::Reset() {
  *this = Context{features, *errors};
}

Index Context::GetLocalCount() const {
  return locals_partial_sum.empty() ? 0 : locals_partial_sum.back();
}

optional<ValueType> Context::GetLocalType(Index index) const {
  const auto iter = std::upper_bound(locals_partial_sum.begin(),
                                     locals_partial_sum.end(), index);
  if (iter == locals_partial_sum.end()) {
    return nullopt;
  }
  return locals[iter - locals_partial_sum.begin()];
}

bool Context::AppendLocals(Index count, ValueType value_type) {
  if (count > 0) {
    const Index max = std::numeric_limits<Index>::max();
    const auto old_count = GetLocalCount();
    if (old_count >= max - count) {
      return false;
    }

    locals_partial_sum.emplace_back(old_count + count);
    locals.emplace_back(value_type);
  }
  return true;
}

bool Context::AppendLocals(const ValueTypes& value_types) {
  if (value_types.empty()) {
    return true;
  }

  size_t last_index = 0;
  ValueType last_type = value_types[0];
  for (size_t i = 1; i < value_types.size(); ++i) {
    if (value_types[i] != last_type) {
      if (!AppendLocals(i - last_index, last_type)) {
        return false;
      }
      last_index = i;
      last_type = value_types[i];
    }
  }
  return AppendLocals(value_types.size() - last_index, last_type);
}

}  // namespace valid
}  // namespace wasp

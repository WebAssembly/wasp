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

#include <cassert>

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

bool Context::IsStackPolymorphic() const {
  assert(!label_stack.empty());
  return label_stack.back().unreachable;
}

}  // namespace valid
}  // namespace wasp

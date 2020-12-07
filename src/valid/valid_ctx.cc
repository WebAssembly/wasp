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

#include "wasp/valid/valid_ctx.h"

#include <cassert>

namespace wasp::valid {

Label::Label(LabelType label_type,
             StackTypeSpan param_types,
             StackTypeSpan result_types,
             Index type_stack_limit)
    : label_type{label_type},
      param_types{param_types.begin(), param_types.end()},
      result_types{result_types.begin(), result_types.end()},
      type_stack_limit{type_stack_limit},
      unreachable{false} {}

ValidCtx::ValidCtx(Errors& errors) : errors{&errors} {}

ValidCtx::ValidCtx(const Features& features, Errors& errors)
    : features{features}, errors{&errors} {}

ValidCtx::ValidCtx(const ValidCtx& other, Errors& errors) {
  *this = other;
  this->errors = &errors;
}

void ValidCtx::Reset() {
  *this = ValidCtx{features, *errors};
}

bool ValidCtx::IsStackPolymorphic() const {
  assert(!label_stack.empty());
  return label_stack.back().unreachable;
}

bool ValidCtx::IsFunctionType(Index index) const {
  return index < types.size() && types[index].is_function_type();
}

bool ValidCtx::IsStructType(Index index) const {
  return index < types.size() && types[index].is_struct_type();
}

bool ValidCtx::IsArrayType(Index index) const {
  return index < types.size() && types[index].is_array_type();
}

void TypeRelationSet::Reset(Index size) {
  disjoint_set_.Reset(size);
  assume_.clear();
}

auto TypeRelationSet::Get(Index expected, Index actual) -> optional<bool> {
  MaybeSwapIndexes(expected, actual);
  if (!(disjoint_set_.IsValid(expected) && disjoint_set_.IsValid(actual))) {
    // If the indexes are invalid, then there's no point in checking whether
    // they're equal.
    return false;
  }

  if (disjoint_set_.IsSameSet(expected, actual)) {
    return true;
  }

  auto iter = assume_.find({expected, actual});
  if (iter == assume_.end()) {
    return nullopt;
  }

  return iter->second;
}

void TypeRelationSet::Assume(Index expected, Index actual) {
  MaybeSwapIndexes(expected, actual);
  assume_.insert({{expected, actual}, true});
}

void TypeRelationSet::Resolve(Index expected, Index actual, bool is_same) {
  MaybeSwapIndexes(expected, actual);
  auto iter = assume_.find({expected, actual});
  assert(iter != assume_.end());
  if (is_same) {
    disjoint_set_.MergeSets(expected, actual);
    assume_.erase(iter);
  } else {
    iter->second = false;
  }
}

void TypeRelationSet::MaybeSwapIndexes(Index& lhs, Index& rhs) {
  if (lhs > rhs) {
    std::swap(lhs, rhs);
  }
}

}  // namespace wasp::valid

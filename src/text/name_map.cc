//
// Copyright 2020 WebAssembly Community Group participants
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

#include "wasp/text/read/name_map.h"

#include <cassert>
#include "wasp/base/macros.h"

namespace wasp::text {

NameMap::NameMap() : stack_{0} {}

void NameMap::Reset() {
  names_.clear();
  stack_ = {0};
}

void NameMap::NewUnbound() {
  names_.push_back(nullopt);
}

bool NameMap::NewBound(BindVar var) {
  if (HasSinceLastPush(var)) {
    return false;
  }
  names_.push_back(var);
  return true;
}

void NameMap::Push() {
  stack_.push_back(names_.size());
}

void NameMap::Pop() {
  assert(stack_.size() > 1);
  names_.resize(stack_.back());
  stack_.pop_back();
}

bool NameMap::Has(BindVar var) const {
  return FindInRange(0, names_.size(), var).has_value();
}

bool NameMap::HasSinceLastPush(BindVar var) const {
  return FindInRange(stack_.back(), names_.size(), var).has_value();
}

optional<size_t> NameMap::FindInRange(size_t begin,
                                      size_t end,
                                      BindVar var) const {
  for (size_t i = begin; i < end; ++i) {
    auto&& opt_name = names_[i];
    if (opt_name && opt_name == var) {
      return i;
    }
  }
  return nullopt;
}

optional<Index> NameMap::Get(BindVar var) const {
  Index offset = 0;
  size_t end = names_.size();
  for (auto iter = stack_.rbegin(); iter != stack_.rend(); ++iter) {
    size_t begin = *iter;
    auto found = FindInRange(begin, end, var);
    if (found) {
      return offset + *found - begin;
    }
    offset += static_cast<Index>(end - begin);
    end = begin;
  }
  return nullopt;
}

auto NameMap::Size() const -> Index {
  return static_cast<Index>(names_.size());
}

}  // namespace wasp::text

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

#include "wasp/valid/local_map.h"

#include <algorithm>
#include <limits>

namespace wasp {
namespace valid {

LocalMap::LocalMap() {}

void LocalMap::Reset() {
  partial_sum_.clear();
  types_.clear();
}

auto LocalMap::GetCount() const -> Index {
  return partial_sum_.empty() ? 0 : partial_sum_.back();
}

auto LocalMap::GetType(Index index) const -> optional<binary::ValueType>{
  const auto iter =
      std::upper_bound(partial_sum_.begin(), partial_sum_.end(), index);
  if (iter == partial_sum_.end()) {
    return nullopt;
  }
  return types_[iter - partial_sum_.begin()];
}

bool LocalMap::Append(Index count, binary::ValueType value_type) {
  if (count > 0) {
    const Index max = std::numeric_limits<Index>::max();
    const auto old_count = GetCount();
    if (old_count > max - count) {
      return false;
    }

    partial_sum_.emplace_back(old_count + count);
    types_.emplace_back(value_type);
  }
  return true;
}

bool LocalMap::Append(const binary::ValueTypeList& value_types) {
  if (value_types.empty()) {
    return true;
  }
  if (GetCount() > std::numeric_limits<Index>::max() - value_types.size()) {
    return false;
  }

  for (auto value_type : value_types) {
    if (!partial_sum_.empty() && types_.back() == value_type) {
      partial_sum_.back()++;
    } else {
      partial_sum_.push_back(GetCount() + 1);
      types_.push_back(value_type);
    }
  }
  return true;
}

}  // namespace valid
}  // namespace wasp

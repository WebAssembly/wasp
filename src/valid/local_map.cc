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
  types_.clear();
}

auto LocalMap::GetCount() const -> Index {
  return types_.empty() ? 0 : types_.back().second;
}

auto LocalMap::GetType(Index index) const -> optional<binary::ValueType>{
  struct Compare {
    bool operator()(const Pair& lhs, Index rhs) { return lhs.second < rhs; }
    bool operator()(Index lhs, const Pair& rhs) { return lhs < rhs.second; }
  };

  const auto iter =
      std::upper_bound(types_.begin(), types_.end(), index, Compare{});
  if (iter == types_.end()) {
    return nullopt;
  }
  return iter->first;
}

bool LocalMap::Append(Index count, binary::ValueType value_type) {
  if (count == 0) {
    return true;
  }
  if (!CanAppend(count)) {
    return false;
  }

  types_.emplace_back(value_type, GetCount() + count);
  return true;
}

bool LocalMap::Append(const binary::ValueTypeList& value_types) {
  if (value_types.empty()) {
    return true;
  }
  if (!CanAppend(value_types.size())) {
    return false;
  }

  for (auto value_type : value_types) {
    if (!types_.empty() && types_.back().first == value_type) {
      types_.back().second++;
    } else {
      types_.emplace_back(value_type, GetCount() + 1);
    }
  }
  return true;
}

bool LocalMap::CanAppend(Index count) const {
  return GetCount() <= std::numeric_limits<Index>::max() - count;
}

}  // namespace valid
}  // namespace wasp

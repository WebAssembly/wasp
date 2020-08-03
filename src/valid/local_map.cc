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
#include <cassert>
#include <limits>

namespace wasp {
namespace valid {

LocalMap::LocalMap() {
  Reset();
}

void LocalMap::Reset() {
  pairs_.clear();
  let_stack_.push_back(0);
}

auto LocalMap::GetCount() const -> Index {
  return pairs_.empty() ? 0 : pairs_.back().second;
}

auto LocalMap::GetType(Index index) const -> optional<binary::ValueType>{
  struct Compare {
    bool operator()(const Pair& lhs, Index rhs) { return lhs.second < rhs; }
    bool operator()(Index lhs, const Pair& rhs) { return lhs < rhs.second; }
  };

  const auto iter =
      std::upper_bound(pairs_.begin(), pairs_.end(), index, Compare{});
  if (iter == pairs_.end()) {
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

  // Locals may be "appended" to the middle of the list, if we are currently in
  // a let block, e.g.
  //
  // If the function begins with the following locals:
  //
  //   {i32, f32, f32}
  //
  // Then a new let block will add locals to the beginning of this list. For
  // example, appending an i64 yields:
  //
  //   {i64,  i32, f32, f32}
  //
  // Subsequent appends for this let block go after these variables, but before
  // the function variables. So appending an f64 yields:
  //
  //   {i64, f64,  i32, f32, f32}

  assert(!let_stack_.empty());
  Index insert_at = let_stack_.back();

  if (insert_at > 0) {
    // There's a previous value, see if we can combine this value type.
    auto& prev_pair = pairs_[insert_at - 1];
    if (prev_pair.first == value_type) {
      prev_pair.second += count;
    } else {
      pairs_.emplace(pairs_.begin() + insert_at,
                     Pair{value_type, prev_pair.second + count});
      let_stack_.back()++;
    }
  } else {
    // Inserting at the beginning, so we know that the previous count is 0.
    pairs_.emplace(pairs_.begin(), Pair{value_type, count});
    let_stack_.back()++;
  }

  AdjustPartialSums(pairs_.begin() + let_stack_.back(), count);
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
    bool ok = Append(1, value_type);
    assert(ok);  // Checked above, so it should always be true.
  }
  return true;
}

bool LocalMap::CanAppend(Index count) const {
  return GetCount() <= std::numeric_limits<Index>::max() - count;
}

void LocalMap::AdjustPartialSums(Pairs::iterator first, Index count) {
  for (auto iter = first; iter != pairs_.end(); ++iter) {
    // Wrap-around is OK here, since the adjustment may be positive or negative.
    iter->second += count;
  }
}

void LocalMap::Push() {
  let_stack_.push_back(0);
}

void LocalMap::Pop() {
  assert(!let_stack_.empty());
  Index pair_count = let_stack_.back();
  let_stack_.pop_back();

  if (pair_count > 0) {
    Index var_count = pairs_[pair_count - 1].second;

    // Erase all pairs corresponding to this let block.
    assert(pair_count < pairs_.size());
    pairs_.erase(pairs_.begin(), pairs_.begin() + pair_count);

    // Adjust the partial sums to remove the number of variables from this let
    // block.
    assert(!pairs_.empty());
    AdjustPartialSums(pairs_.begin(), -var_count);
  }
}

}  // namespace valid
}  // namespace wasp

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

#ifndef WASP_BASE_HASH_H_
#define WASP_BASE_HASH_H_

#include <iterator>

#include "parallel_hashmap/phmap.h"
#include "parallel_hashmap/phmap_utils.h"

namespace wasp {

using phmap::flat_hash_set;
using phmap::flat_hash_map;
using phmap::node_hash_set;
using phmap::node_hash_map;

using phmap::HashState;

template <typename T1, typename T2>
size_t HashRange(T1 begin, T2 end) {
  size_t state = 0;
  for (auto it = begin; it != end; ++it) {
    state = HashState::combine(state, *it);
  }
  return state;
}

template <typename C>
size_t HashContainer(const C& c) {
  return HashRange(std::begin(c), std::end(c));
}

}  // namespace wasp

#endif  // WASP_BASE_HASH_H_

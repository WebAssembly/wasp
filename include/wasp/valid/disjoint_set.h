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

#ifndef WASP_VALID_DISJOINT_SET_H
#define WASP_VALID_DISJOINT_SET_H

#include <vector>

#include "wasp/base/types.h"

namespace wasp::valid {

// See union-find algorithm described here:
// https://en.wikipedia.org/wiki/Disjoint-set_data_structure
class DisjointSet {
 public:
  void Reset(Index size);
  bool IsValid(Index) const;

  auto Find(Index) -> Index;  // Find root item for this set.
  bool IsSameSet(Index, Index);

  void MergeSets(Index, Index);  // Merge disjoint sets.

 private:
  struct Node {
    Index parent;
    int size;
  };

  auto Get(Index) const -> const Node&;
  auto Get(Index) -> Node&;

  std::vector<Node> nodes_;
};

}  // namespace wasp::valid

#endif  // WASP_VALID_DISJOINT_SET_H

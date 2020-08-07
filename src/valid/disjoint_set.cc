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

#include "wasp/valid/disjoint_set.h"

#include <cassert>

namespace wasp {
namespace valid {

void DisjointSet::Reset(Index size) {
  nodes_.clear();
  nodes_.reserve(size);
  for (Index i = 0; i < size; ++i) {
    nodes_.push_back(Node{i, 1});
  }
}

auto DisjointSet::Find(Index x) -> Index {
  // Use path-splitting technique described here:
  // https://en.wikipedia.org/wiki/Disjoint-set_data_structure
  while (Get(x).parent != x) {
    Index next = Get(x).parent;
    Get(x).parent = Get(next).parent;
    x = next;
  }
  return x;
}

bool DisjointSet::IsSameSet(Index x, Index y) {
  return Find(x) == Find(y);
}

void DisjointSet::MergeSets(Index x, Index y) {
  Index xroot = Find(x);
  Index yroot = Find(y);

  if (xroot == yroot) {
    return;
  }

  auto& xroot_node = Get(xroot);
  auto& yroot_node = Get(yroot);

  if (xroot_node.size < yroot_node.size) {
    std::swap(xroot_node.size, yroot_node.size);
  }

  // Merge yroot into xroot.
  yroot_node.parent = xroot;
  xroot_node.size += yroot_node.size;
}

auto DisjointSet::Get(Index x) const -> const Node& {
  assert(x  < nodes_.size());
  return nodes_[x];
}

auto DisjointSet::Get(Index x) -> Node& {
  assert(x  < nodes_.size());
  return nodes_[x];
}

}  // namespace valid
}  // namespace wasp

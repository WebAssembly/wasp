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

#include "wasp/base/hash.h"

#include "gtest/gtest.h"

using namespace ::wasp;

namespace {

struct S {
  friend bool operator==(const S& lhs, const S& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }

  // Documentation says to use friend function named `hash_value`, but this
  // only seems to work with a static function.
  static size_t hash_value(const S& s) {
    return HashState::combine(0, s.x, s.y);
  }

  int x, y;
};

}  // namespace

TEST(HashTest, flat_hash_map) {
  flat_hash_map<int, int> map;
  map.emplace(1, 2);
  map.emplace(1, 3);
  map.emplace(2, 4);
  ASSERT_EQ(2u, map.size());
  EXPECT_EQ(2, map[1]);
  EXPECT_EQ(4, map[2]);
}

TEST(HashTest, flat_hash_set) {
  flat_hash_set<int> set;
  set.emplace(1);
  set.emplace(1);
  set.emplace(2);
  ASSERT_EQ(2u, set.size());
  EXPECT_EQ(1, set.count(1));
  EXPECT_EQ(1, set.count(2));
  EXPECT_EQ(0, set.count(0));
}

TEST(HashTest, node_hash_map) {
  node_hash_map<int, int> map;
  map.emplace(1, 2);
  map.emplace(1, 3);
  map.emplace(2, 4);
  ASSERT_EQ(2u, map.size());
  EXPECT_EQ(2, map[1]);
  EXPECT_EQ(4, map[2]);
}

TEST(HashTest, node_hash_set) {
  node_hash_set<int> set;
  set.emplace(1);
  set.emplace(1);
  set.emplace(2);
  ASSERT_EQ(2u, set.size());
  EXPECT_EQ(1, set.count(1));
  EXPECT_EQ(1, set.count(2));
  EXPECT_EQ(0, set.count(0));
}

TEST(HashTest, UserDefined) {
  flat_hash_map<S, int> map;
  map[S{0, 0}]++;
  map[S{0, 0}]++;
  map[S{1, 1}]++;

  EXPECT_EQ(2u, map.size());
  EXPECT_EQ(2, map[(S{0, 0})]);
  EXPECT_EQ(1, map[(S{1, 1})]);
}

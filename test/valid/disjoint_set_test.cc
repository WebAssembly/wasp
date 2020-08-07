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

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::valid;

TEST(ValidDisjointSetTest, Basic) {
  DisjointSet set;
  set.Reset(5);

  set.MergeSets(0, 3);
  set.MergeSets(1, 3);
  set.MergeSets(2, 4);

  EXPECT_TRUE(set.IsSameSet(0, 1));
  EXPECT_TRUE(set.IsSameSet(0, 3));
  EXPECT_TRUE(set.IsSameSet(1, 0));
  EXPECT_TRUE(set.IsSameSet(1, 3));
  EXPECT_TRUE(set.IsSameSet(3, 0));
  EXPECT_TRUE(set.IsSameSet(3, 1));
  EXPECT_TRUE(set.IsSameSet(2, 4));
  EXPECT_TRUE(set.IsSameSet(4, 2));

  EXPECT_FALSE(set.IsSameSet(0, 2));
  EXPECT_FALSE(set.IsSameSet(0, 4));
  EXPECT_FALSE(set.IsSameSet(1, 2));
  EXPECT_FALSE(set.IsSameSet(1, 4));
  EXPECT_FALSE(set.IsSameSet(2, 0));
  EXPECT_FALSE(set.IsSameSet(2, 1));
  EXPECT_FALSE(set.IsSameSet(2, 3));
  EXPECT_FALSE(set.IsSameSet(3, 2));
  EXPECT_FALSE(set.IsSameSet(3, 4));
  EXPECT_FALSE(set.IsSameSet(4, 0));
  EXPECT_FALSE(set.IsSameSet(4, 1));
  EXPECT_FALSE(set.IsSameSet(4, 3));
}

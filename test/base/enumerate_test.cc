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

#include "wasp/base/enumerate.h"

#include <map>
#include <vector>

#include "gtest/gtest.h"

using namespace ::wasp;

namespace {

struct MoveOnly {
  MoveOnly(int value): value{value} {}

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  MoveOnly(const MoveOnly&& other) : value{other.value} {}
  MoveOnly& operator=(const MoveOnly&& rhs) {
    value = rhs.value;
    return *this;
  }

  int value;
};

}  // namespace

TEST(EnumerateTest, Vector) {
  std::vector<int> v{1, 2, 3, 4, 5};

  for (auto pair : enumerate(v)) {
    EXPECT_EQ(pair.index + 1, pair.value);
  }
}

TEST(EnumerateTest, MoveOnly_Lvalue) {
  std::vector<MoveOnly> v;
  v.emplace_back(0);
  v.emplace_back(1);
  v.emplace_back(2);

  for (auto pair : enumerate(v)) {
    EXPECT_EQ(pair.index, pair.value.value);
  }
}

TEST(EnumerateTest, MoveOnly_Rvalue) {
  auto make_v = []() {
    std::vector<MoveOnly> v;
    v.emplace_back(0);
    v.emplace_back(1);
    v.emplace_back(2);
    return v;
  };

  for (auto pair : enumerate(make_v())) {
    EXPECT_EQ(pair.index, pair.value.value);
  }
}

TEST(EnumerateTest, Start) {
  std::vector<int> v{10, 11, 12, 13};

  for (auto pair : enumerate(v, 10)) {
    EXPECT_EQ(pair.index, pair.value);
  }
}

TEST(EnumerateTest, Map) {
  std::map<int, std::string> m{{1, "one"}, {10, "ten"}, {100, "hundred"}};

  auto seq = enumerate(m);
  auto i = seq.begin();

  ASSERT_NE(seq.end(), i);
  EXPECT_EQ(0, (*i).index);
  EXPECT_EQ(1, (*i).value.first);
  EXPECT_EQ("one", (*i).value.second);
  ++i;

  ASSERT_NE(seq.end(), i);
  EXPECT_EQ(1, (*i).index);
  EXPECT_EQ(10, (*i).value.first);
  EXPECT_EQ("ten", (*i).value.second);
  ++i;

  ASSERT_NE(seq.end(), i);
  EXPECT_EQ(2, (*i).index);
  EXPECT_EQ(100, (*i).value.first);
  EXPECT_EQ("hundred", (*i).value.second);
  ++i;

  ASSERT_EQ(seq.end(), i);
}

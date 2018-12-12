//
// Copyright 2018 WebAssembly Community Group participants
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

#include "src/base/to_string.h"

#include "gtest/gtest.h"

using namespace ::wasp;

namespace {

struct Point {
  Point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

std::string ToString(const Point& p) {
  return format("{{x:{}, y:{}}}", p.x, p.y);
}

}

TEST(ToStringTest, U32) {
  EXPECT_EQ("100", ToString(u32{100u}));
}

TEST(ToStringTest, SpanU8) {
  EXPECT_EQ(R"("")", ToString(SpanU8{}));

  const u8 buffer[] = "Hello, World!";
  EXPECT_EQ(R"("\48\65\6c")", ToString(SpanU8{buffer, 3}));
}

TEST(ToStringTest, VectorU32) {
  EXPECT_EQ(R"([])", ToString(std::vector<u32>{}));
  EXPECT_EQ(R"([1 2 3])", ToString(std::vector<u32>{{1u, 2u, 3u}}));
}

TEST(ToStringTest, VectorPoint) {
  EXPECT_EQ(R"([])", ToString(std::vector<Point>{}));
  EXPECT_EQ(R"([{x:1, y:1} {x:2, y:3}])",
            ToString(std::vector<Point>{{Point{1, 1}, Point{2, 3}}}));
}

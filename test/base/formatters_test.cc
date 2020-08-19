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

#include "wasp/base/formatters.h"

#include <iostream>

#include "gtest/gtest.h"
#include "wasp/base/variant.h"

#include "wasp/base/concat.h"

using namespace ::wasp;

namespace {

struct Point {
  Point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << "{x:" << p.x << ", y:" << p.y << "}";
}

}  // namespace

namespace wasp {

WASP_DEFINE_VARIANT_NAME(Point, "Point")

}  // namespace wasp

TEST(FormattersTest, U32) {
  EXPECT_EQ("100", concat(u32{100u}));
}

TEST(FormattersTest, SpanU8) {
  EXPECT_EQ(R"("")", concat(SpanU8{}));

  const u8 buffer[] = "Hello, World!";
  EXPECT_EQ(R"("\48\65\6c")", concat(SpanU8{buffer, 3}));
}

TEST(FormattersTest, SpanPoint) {
  using PointSpan = span<const Point>;
  const Point points[] = {Point{1, 1}, Point{2, 3}, Point{0, 0}};

  EXPECT_EQ(R"([])", concat(PointSpan{}));
  EXPECT_EQ(R"([{x:1, y:1} {x:2, y:3}])", concat(PointSpan{points, 2}));
}

TEST(FormattersTest, VectorU32) {
  EXPECT_EQ(R"([])", concat(std::vector<u32>{}));
  EXPECT_EQ(R"([1 2 3])", concat(std::vector<u32>{{1u, 2u, 3u}}));
}

TEST(FormattersTest, VectorPoint) {
  EXPECT_EQ(R"([])", concat(std::vector<Point>{}));
  EXPECT_EQ(R"([{x:1, y:1} {x:2, y:3}])",
            concat(std::vector<Point>{{Point{1, 1}, Point{2, 3}}}));
}

TEST(FormattersTest, V128) {
  EXPECT_EQ(R"(0x1 0x0 0x2 0x0)", concat(v128{u64{1}, u64{2}}));
}

TEST(FormattersTest, Optional) {
  using OptU32 = optional<u32>;

  EXPECT_EQ(R"(none)", concat(OptU32{}));
  EXPECT_EQ(R"(1)", concat(OptU32{1}));
}

TEST(FormattersTest, Variant) {
  using MyVariant = variant<u32, Point>;

  EXPECT_EQ(R"(u32 123)", concat(MyVariant{u32{123}}));
  EXPECT_EQ(R"(Point {x:1, y:2})", concat(MyVariant{Point{1, 2}}));
}

TEST(FormattersTest, MemoryType) {
  EXPECT_EQ(R"({min 1, max 2})", concat(MemoryType{Limits{1, 2}}));
}

TEST(FormattersTest, ShuffleImmediate) {
  EXPECT_EQ(R"([0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15])",
            concat(ShuffleImmediate{
                {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}}));
}

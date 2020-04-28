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

#include "wasp/base/variant.h"

#include "gtest/gtest.h"

using namespace ::wasp;

namespace {

struct Point {
  Point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

}  // namespace

namespace fmt {

template <>
struct formatter<Point> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) { return ctx.begin(); }

  template <typename Ctx>
  typename Ctx::iterator format(const Point& p, Ctx& ctx) {
    return format_to(ctx.out(), "{{x:{}, y:{}}}", p.x, p.y);
  }
};

}  // namespace fmt

namespace wasp {

WASP_DEFINE_VARIANT_NAME(Point, "Point")

}  // namespace wasp

TEST(FormattersTest, U32) {
  EXPECT_EQ("100", format("{}", u32{100u}));
}

TEST(FormattersTest, SpanU8) {
  EXPECT_EQ(R"("")", format("{}", SpanU8{}));

  const u8 buffer[] = "Hello, World!";
  EXPECT_EQ(R"("\48\65\6c")", format("{}", SpanU8{buffer, 3}));

  EXPECT_EQ(R"(  "\48"  )", format("{:^9s}", SpanU8{buffer, 1}));
}

TEST(FormattersTest, SpanPoint) {
  using PointSpan = span<const Point>;
  const Point points[] = {Point{1, 1}, Point{2, 3}, Point{0, 0}};

  EXPECT_EQ(R"([])", format("{}", PointSpan{}));
  EXPECT_EQ(R"([{x:1, y:1} {x:2, y:3}])", format("{}", PointSpan{points, 2}));
  EXPECT_EQ(R"(  [{x:0, y:0}]  )", format("{:^16s}", PointSpan{points + 2, 1}));
}

TEST(FormattersTest, VectorU32) {
  EXPECT_EQ(R"([])", format("{}", std::vector<u32>{}));
  EXPECT_EQ(R"([1 2 3])", format("{}", std::vector<u32>{{1u, 2u, 3u}}));
  EXPECT_EQ(R"(   [0 0])", format("{:>8s}", std::vector<u32>{{0u, 0u}}));
}

TEST(FormattersTest, VectorPoint) {
  EXPECT_EQ(R"([])", format("{}", std::vector<Point>{}));
  EXPECT_EQ(R"([{x:1, y:1} {x:2, y:3}])",
            format("{}", std::vector<Point>{{Point{1, 1}, Point{2, 3}}}));
  EXPECT_EQ(R"(  [{x:0, y:0}]  )",
            format("{:^16s}", std::vector<Point>{{Point{0, 0}}}));
}

TEST(FormattersTest, V128) {
  EXPECT_EQ(R"(0x1 0x0 0x2 0x0)", format("{}", v128{u64{1}, u64{2}}));
  EXPECT_EQ(R"(  0x0 0x0 0x0 0x0)", format("{:>17}", v128{}));
}

TEST(FormattersTest, Optional) {
  using OptU32 = optional<u32>;

  EXPECT_EQ(R"(none)", format("{}", OptU32{}));
  EXPECT_EQ(R"(1)", format("{}", OptU32{1}));
}

TEST(FormattersTest, Variant) {
  using MyVariant = variant<u32, Point>;

  EXPECT_EQ(R"(u32 123)", format("{}", MyVariant{u32{123}}));
  EXPECT_EQ(R"(Point {x:1, y:2})", format("{}", MyVariant{Point{1, 2}}));
}

TEST(FormattersTest, ShuffleImmediate) {
  EXPECT_EQ(R"([0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15])",
            format("{}", ShuffleImmediate{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                           12, 13, 14, 15}}));
  EXPECT_EQ(R"(  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0])",
            format("{:>35s}", ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0}}));
}

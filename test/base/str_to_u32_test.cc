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

#include "wasp/base/str_to_u32.h"

#include "gtest/gtest.h"

using namespace ::wasp;

TEST(StrToU32Test, Basic) {
  EXPECT_EQ(42u, StrToU32("42"));
  EXPECT_EQ(31415u, StrToU32("31415"));
  EXPECT_EQ(123456789u, StrToU32("123456789"));
  EXPECT_EQ(987654321u, StrToU32("987654321"));
  EXPECT_EQ(4294967295u, StrToU32("4294967295"));
}

TEST(StrToU32Test, NoOctal) {
  EXPECT_EQ(100u, StrToU32("0100"));
}

TEST(StrToU32Test, PowersOf10) {
  EXPECT_EQ(0u, StrToU32("0"));
  EXPECT_EQ(10u, StrToU32("10"));
  EXPECT_EQ(100u, StrToU32("100"));
  EXPECT_EQ(1000u, StrToU32("1000"));
  EXPECT_EQ(10000u, StrToU32("10000"));
  EXPECT_EQ(100000u, StrToU32("100000"));
  EXPECT_EQ(1000000u, StrToU32("1000000"));
  EXPECT_EQ(10000000u, StrToU32("10000000"));
  EXPECT_EQ(100000000u, StrToU32("100000000"));
  EXPECT_EQ(1000000000u, StrToU32("1000000000"));
}

TEST(StrToU32Test, Overflow) {
  EXPECT_FALSE(StrToU32("4294967296").has_value());
  EXPECT_FALSE(StrToU32("10000000000").has_value());
}

TEST(StrToU32Test, BadCharacters) {
  EXPECT_FALSE(StrToU32("one hundred").has_value());
  EXPECT_FALSE(StrToU32("100a").has_value());
  EXPECT_FALSE(StrToU32("  100").has_value());
  EXPECT_FALSE(StrToU32("0x100").has_value());
  EXPECT_FALSE(StrToU32("-1").has_value());
}

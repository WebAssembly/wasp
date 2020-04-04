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

#include "wasp/text/formatters.h"

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::text;

TEST(FormattersTest, TokenType) {
  EXPECT_EQ(R"(Binary)", format("{}", TokenType::Binary));
  EXPECT_EQ(R"(  End)", format("{:>5s}", TokenType::End));
}

TEST(FormattersTest, Sign) {
  EXPECT_EQ(R"(None)", format("{}", Sign::None));
  EXPECT_EQ(R"(  Plus)", format("{:>6s}", Sign::Plus));
}

TEST(FormattersTest, LiteralKind) {
  EXPECT_EQ(R"(Normal)", format("{}", LiteralKind::Normal));
  EXPECT_EQ(R"(  Nan)", format("{:>5s}", LiteralKind::Nan));
}

TEST(FormattersTest, Base) {
  EXPECT_EQ(R"(Decimal)", format("{}", Base::Decimal));
  EXPECT_EQ(R"(  Hex)", format("{:>5s}", Base::Hex));
}

TEST(FormattersTest, HasUnderscores) {
  EXPECT_EQ(R"(No)", format("{}", HasUnderscores::No));
  EXPECT_EQ(R"(  Yes)", format("{:>5s}", HasUnderscores::Yes));
}

TEST(FormattersTest, LiteralInfo) {
  EXPECT_EQ(R"({sign None, kind NanPayload, base Hex, underscores No})",
            format("{}", LiteralInfo{Sign::None, LiteralKind::NanPayload,
                                     Base::Hex, HasUnderscores::No}));
  EXPECT_EQ(R"(  {sign Plus, kind Normal, base Decimal, underscores Yes})",
            format("{:>57s}", LiteralInfo{Sign::Plus, LiteralKind::Normal,
                                         Base::Decimal, HasUnderscores::Yes}));
}

TEST(FormattersTest, Token) {
  EXPECT_EQ(R"({loc "\28", type Lpar})",
            format("{}", Token{"("_su8, TokenType::Lpar}));

  EXPECT_EQ(R"({loc "\69\33\32\2e\61\64\64", type PlainInstr, opcode i32.add})",
            format("{}", Token{"i32.add"_su8, TokenType::PlainInstr,
                               Opcode::I32Add}));

  EXPECT_EQ(
      R"({loc "\69\33\32", type ValueType, value_type i32})",
      format("{}", Token{"i32"_su8, TokenType::ValueType, ValueType::I32}));

  EXPECT_EQ(
      R"({loc "\31\32\33", type Nat, literal_info {sign None, kind Normal, base Decimal, underscores No}})",
      format("{}", Token{"123"_su8, TokenType::Nat,
                         LiteralInfo::Nat(HasUnderscores::No)}));
}

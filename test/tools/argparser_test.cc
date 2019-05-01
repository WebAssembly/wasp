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

#include <string>
#include <vector>

#include "src/tools/argparser.h"

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::tools;

TEST(ArgParserTest, ShortFlag) {
  bool flag = false;
  ArgParser parser;
  parser.Add('f', [&]() { flag = true; });

  std::vector<string_view> args{{"-f"}};
  parser.Parse(args);
  EXPECT_EQ(true, flag);
}

TEST(ArgParserTest, LongFlag) {
  bool flag = false;
  ArgParser parser;
  parser.Add("--flag", [&]() { flag = true; });

  std::vector<string_view> args{{"--flag"}};
  parser.Parse(args);
  EXPECT_EQ(true, flag);
}

TEST(ArgParserTest, BothFlag) {
  int count = 0;
  ArgParser parser;
  parser.Add('f', "--flag", [&]() { ++count; });

  std::vector<string_view> args{{"-f", "--flag", "-f", "--flag"}};
  parser.Parse(args);
  EXPECT_EQ(4, count);
}

TEST(ArgParserTest, ShortFlagCombined) {
  int count = 0;
  ArgParser parser;
  parser.Add('a', [&]() { count += 1; });
  parser.Add('b', [&]() { count += 2; });

  std::vector<string_view> args{{"-aa", "-abb"}};
  parser.Parse(args);
  EXPECT_EQ(7, count);
}

TEST(ArgParserTest, UnknownFlag) {
  ArgParser parser;
  std::vector<string_view> args{{"-f", "-gh"}};
  parser.Parse(args);
}

TEST(ArgParserTest, ShortParam) {
  string_view param;
  ArgParser parser;
  parser.Add('p', [&](string_view arg) { param = arg; });

  std::vector<string_view> args{{"-p", "hello"}};
  parser.Parse(args);
  EXPECT_EQ("hello", param);
}

TEST(ArgParserTest, LongParam) {
  string_view param;
  ArgParser parser;
  parser.Add("--param", [&](string_view arg) { param = arg; });

  std::vector<string_view> args{{"--param", "hello"}};
  parser.Parse(args);
  EXPECT_EQ("hello", param);
}

TEST(ArgParserTest, BothParam) {
  std::string param;
  ArgParser parser;
  parser.Add('p', "--param",
             [&](string_view arg) { param += arg.to_string(); });

  std::vector<string_view> args{{"-p", "hello", "--param", "world"}};
  parser.Parse(args);
  EXPECT_EQ("helloworld", param);
}

TEST(ArgParserTest, MissingParam) {
  string_view param;
  ArgParser parser;
  parser.Add('p', [&](string_view arg) { param = arg; });

  std::vector<string_view> args{{"-p"}};
  parser.Parse(args);
  EXPECT_EQ(string_view{}, param);
}

TEST(ArgParserTest, FlagCombinedAfterShortParam) {
  string_view param;
  bool has_x = false;

  ArgParser parser;
  parser.Add('p', [&](string_view arg) { param = arg; });
  parser.Add('x', [&]() { has_x = true; });

  std::vector<string_view> args{{"-px", "stuff"}};
  parser.Parse(args);
  EXPECT_EQ("stuff", param);
  EXPECT_FALSE(has_x);
}

TEST(ArgParserTest, Bare) {
  std::vector<string_view> bare;

  ArgParser parser;
  parser.Add([&](string_view arg) { bare.push_back(arg); });

  std::vector<string_view> args{{"hello", "world"}};
  parser.Parse(args);
  ASSERT_EQ(2, bare.size());
  EXPECT_EQ("hello", bare[0]);
  EXPECT_EQ("world", bare[1]);
}

TEST(ArgParserTest, BareWithFlags) {
  int count = 0;
  std::vector<string_view> bare;

  ArgParser parser;
  parser.Add('f', [&]() { count++; });
  parser.Add([&](string_view arg) { bare.push_back(arg); });

  std::vector<string_view> args{{"-f", "bare", "-ff"}};
  parser.Parse(args);
  ASSERT_EQ(1, bare.size());
  EXPECT_EQ("bare", bare[0]);
  EXPECT_EQ(3, count);
}

TEST(ArgParserTest, UnknownBare) {
  ArgParser parser;
  std::vector<string_view> args{{"foo", "bar"}};
  parser.Parse(args);
}

TEST(ArgParserTest, RestOfArgs) {
  span<string_view> rest;

  ArgParser parser;
  parser.Add('a', []() {})
      .Add('b', []() {})
      .Add('c', []() {})
      .Add('h', []() {})
      .Add([&](string_view arg) {
        if (arg == "here") {
          rest = parser.RestOfArgs();
        }
      });

  std::vector<string_view> args{{"-abc", "-h", "here", "1", "2", "3"}};
  parser.Parse(args);

  ASSERT_EQ(3, rest.size());
  EXPECT_EQ("1", rest[0]);
  EXPECT_EQ("2", rest[1]);
  EXPECT_EQ("3", rest[2]);
}

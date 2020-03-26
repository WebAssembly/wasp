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

TEST(ArgParserTest, LongFlag) {
  bool flag = false;
  ArgParser parser{"prog"};
  parser.Add("--flag", "help", [&]() { flag = true; });

  std::vector<string_view> args{{"--flag"}};
  parser.Parse(args);
  EXPECT_EQ(true, flag);
}

TEST(ArgParserTest, BothFlag) {
  int count = 0;
  ArgParser parser{"prog"};
  parser.Add('f', "--flag", "help", [&]() { ++count; });

  std::vector<string_view> args{{"-f", "--flag", "-f", "--flag"}};
  parser.Parse(args);
  EXPECT_EQ(4, count);
}

TEST(ArgParserTest, ShortFlagCombined) {
  int count = 0;
  ArgParser parser{"prog"};
  parser.Add('a', "--a", "help", [&]() { count += 1; });
  parser.Add('b', "--b", "help", [&]() { count += 2; });

  std::vector<string_view> args{{"-aa", "-abb"}};
  parser.Parse(args);
  EXPECT_EQ(7, count);
}

TEST(ArgParserTest, UnknownFlag) {
  ArgParser parser{"prog"};
  std::vector<string_view> args{{"-f", "-gh"}};
  parser.Parse(args);
}

TEST(ArgParserTest, LongParam) {
  string_view param;
  ArgParser parser{"prog"};
  parser.Add("--param", "metavar", "help",
             [&](string_view arg) { param = arg; });

  std::vector<string_view> args{{"--param", "hello"}};
  parser.Parse(args);
  EXPECT_EQ("hello", param);
}

TEST(ArgParserTest, BothParam) {
  std::string param;
  ArgParser parser{"prog"};
  parser.Add('p', "--param", "metavar", "help",
             [&](string_view arg) { param += arg; });

  std::vector<string_view> args{{"-p", "hello", "--param", "world"}};
  parser.Parse(args);
  EXPECT_EQ("helloworld", param);
}

TEST(ArgParserTest, MissingParam) {
  string_view param;
  ArgParser parser{"prog"};
  parser.Add("--param", "metavar", "help",
             [&](string_view arg) { param = arg; });

  std::vector<string_view> args{{"--param"}};
  parser.Parse(args);
  EXPECT_EQ(string_view{}, param);
}

TEST(ArgParserTest, FlagCombinedAfterShortParam) {
  string_view param;
  bool has_x = false;

  ArgParser parser{"prog"};
  parser.Add('p', "--p", "metavar", "help",
             [&](string_view arg) { param = arg; });
  parser.Add('x', "--x", "help", [&]() { has_x = true; });

  std::vector<string_view> args{{"-px", "stuff"}};
  parser.Parse(args);
  EXPECT_EQ("stuff", param);
  EXPECT_FALSE(has_x);
}

TEST(ArgParserTest, Bare) {
  std::vector<string_view> bare;

  ArgParser parser{"prog"};
  parser.Add("metavar", "help", [&](string_view arg) { bare.push_back(arg); });

  std::vector<string_view> args{{"hello", "world"}};
  parser.Parse(args);
  ASSERT_EQ(2, bare.size());
  EXPECT_EQ("hello", bare[0]);
  EXPECT_EQ("world", bare[1]);
}

TEST(ArgParserTest, BareWithFlags) {
  int count = 0;
  std::vector<string_view> bare;

  ArgParser parser{"prog"};
  parser.Add('f', "--f", "help", [&]() { count++; });
  parser.Add("metavar", "help", [&](string_view arg) { bare.push_back(arg); });

  std::vector<string_view> args{{"-f", "bare", "-ff"}};
  parser.Parse(args);
  ASSERT_EQ(1, bare.size());
  EXPECT_EQ("bare", bare[0]);
  EXPECT_EQ(3, count);
}

TEST(ArgParserTest, UnknownBare) {
  ArgParser parser{"prog"};
  std::vector<string_view> args{{"foo", "bar"}};
  parser.Parse(args);
}

TEST(ArgParserTest, RestOfArgs) {
  span<string_view> rest;

  ArgParser parser{"prog"};
  parser.Add('a', "--a", "help", []() {})
      .Add('b', "--b", "help", []() {})
      .Add('c', "--c", "help", []() {})
      .Add('h', "--h", "help", []() {})
      .Add("metavar", "help", [&](string_view arg) {
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

TEST(ArgParserTest, Help) {
  ArgParser parser{"prog"};
  parser.Add('f', "--flag", "help for flag", []() {})
      .Add("--long-only-flag", "help for long-only-flag", []() {})
      .Add('p', "--param", "<param>", "help for param", [&](string_view arg) {})
      .Add("--long-only-param", "<loparam>", "help for long-only-param",
           [&](string_view arg) {})
      .Add("<bare>", "help for bare", [&](string_view arg) {});

  EXPECT_EQ(R"(usage: prog [options] <bare>

options:
 -f, --flag                       help for flag
     --long-only-flag             help for long-only-flag
 -p, --param <param>              help for param
     --long-only-param <loparam>  help for long-only-param

positional:
 <bare>                           help for bare
)", parser.GetHelpString());
}

TEST(ArgParserTest, Features) {
  ArgParser parser{"prog"};
  Features features;
  parser.AddFeatureFlags(features);

  std::vector<string_view> args{{"--enable-simd"}};
  parser.Parse(args);

  EXPECT_TRUE(features.simd_enabled());
}

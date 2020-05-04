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

#include "wasp/text/read/token.h"

#include "gtest/gtest.h"

#include "wasp/base/enumerate.h"

using namespace ::wasp;
using namespace ::wasp::text;

TEST(TextTokenTest, TextToString_Basic) {
  Text value{"\"hello, world!\"", 13};
  EXPECT_EQ("hello, world!", value.ToString());
}

TEST(TextTokenTest, TextToString_Escapes) {
  struct {
    Text text;
    string_view expected;
  } tests[] = {
    {Text{R"("a tab \t")", 7}, "a tab \t"_sv},
    {Text{R"("a newline \n")", 11}, "a newline \n"_sv},
    {Text{R"("a CR \r")", 6}, "a CR \r"_sv},
    {Text{R"("a double quote \"")", 16}, "a double quote \""_sv},
    {Text{R"("a quote \'")", 16}, "a quote '"_sv},
    {Text{R"("a slash \\")", 16}, "a slash \\"_sv},
  };
  for (auto test: tests) {
    EXPECT_EQ(test.expected, test.text.ToString());
  }
}

TEST(TextTokenTest, TextToString_HexEscape) {
  struct {
    Text text;
    string_view expected;
  } tests[] = {
      {Text{R"("\00\01\02\03\04\05\06\07\08\09\0a\0b\0c\0d\0e\0f")", 16},
       "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"_sv},
      {Text{R"("\10\11\12\13\14\15\16\17\18\19\1a\1b\1c\1d\1e\1f")", 16},
       "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"_sv},
      {Text{R"("\20\21\22\23\24\25\26\27\28\29\2a\2b\2c\2d\2e\2f")", 16},
       "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"_sv},
      {Text{R"("\30\31\32\33\34\35\36\37\38\39\3a\3b\3c\3d\3e\3f")", 16},
       "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"_sv},
      {Text{R"("\40\41\42\43\44\45\46\47\48\49\4a\4b\4c\4d\4e\4f")", 16},
       "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"_sv},
      {Text{R"("\50\51\52\53\54\55\56\57\58\59\5a\5b\5c\5d\5e\5f")", 16},
       "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"_sv},
      {Text{R"("\60\61\62\63\64\65\66\67\68\69\6a\6b\6c\6d\6e\6f")", 16},
       "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"_sv},
      {Text{R"("\70\71\72\73\74\75\76\77\78\79\7a\7b\7c\7d\7e\7f")", 16},
       "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"_sv},
      {Text{R"("\80\81\82\83\84\85\86\87\88\89\8a\8b\8c\8d\8e\8f")", 16},
       "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"_sv},
      {Text{R"("\90\91\92\93\94\95\96\97\98\99\9a\9b\9c\9d\9e\9f")", 16},
       "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"_sv},
      {Text{R"("\a0\a1\a2\a3\a4\a5\a6\a7\a8\a9\aa\ab\ac\ad\ae\af")", 16},
       "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"_sv},
      {Text{R"("\b0\b1\b2\b3\b4\b5\b6\b7\b8\b9\ba\bb\bc\bd\be\bf")", 16},
       "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"_sv},
      {Text{R"("\c0\c1\c2\c3\c4\c5\c6\c7\c8\c9\ca\cb\cc\cd\ce\cf")", 16},
       "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"_sv},
      {Text{R"("\d0\d1\d2\d3\d4\d5\d6\d7\d8\d9\da\db\dc\dd\de\df")", 16},
       "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"_sv},
      {Text{R"("\e0\e1\e2\e3\e4\e5\e6\e7\e8\e9\ea\eb\ec\ed\ee\ef")", 16},
       "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"_sv},
      {Text{R"("\f0\f1\f2\f3\f4\f5\f6\f7\f8\f9\fa\fb\fc\fd\fe\ff")", 16},
       "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"_sv},
  };
  for (auto test : tests) {
    EXPECT_EQ(test.expected, test.text.ToString());
  }
}

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

#include "wasp/text/read/name_map.h"

#include "gtest/gtest.h"

using namespace ::wasp;
using namespace ::wasp::text;

void ExpectGet(NameMap& map, string_view name, Index expected) {
  ASSERT_TRUE(map.Has(name));
  EXPECT_EQ(map.Get(name), expected);
}

TEST(TextNameMapTest, Basic) {
  NameMap map;
  map.NewUnbound();       // 0
  map.NewBound("$1"_sv);  // 1
  map.NewUnbound();       // 2
  map.NewBound("$3"_sv);  // 3

  ExpectGet(map, "$1"_sv, 1);
  ExpectGet(map, "$3"_sv, 3);
}

TEST(TextNameMapTest, NoDuplicates) {
  NameMap map;
  EXPECT_TRUE(map.NewBound("$1"_sv));
  EXPECT_FALSE(map.NewBound("$1"_sv));
}

TEST(TextNameMapTest, PushPopLabels) {
  NameMap map;
  map.Push();
  map.NewBound("$a"_sv);  // $a=0
  ExpectGet(map, "$a"_sv, 0);

  map.Push();
  map.NewBound("$b"_sv);  // $a=1 $b=0
  ExpectGet(map, "$a"_sv, 1);
  ExpectGet(map, "$b"_sv, 0);

  map.Pop();
  ExpectGet(map, "$a"_sv, 0);
}

TEST(TextNameMapTest, ShadowLabels) {
  NameMap map;
  map.Push();
  map.NewBound("$a"_sv);
  ExpectGet(map, "$a"_sv, 0);

  map.Push();
  map.NewBound("$a"_sv);
  ExpectGet(map, "$a"_sv, 0);

  map.Pop();
  ExpectGet(map, "$a"_sv, 0);
}

TEST(TextNameMapTest, LetBindings) {
  NameMap map;
  map.Push();
  map.NewBound("$a"_sv);
  map.NewUnbound();
  map.NewBound("$c"_sv);
  // 0  1  2
  // $a -- $c
  ExpectGet(map, "$a"_sv, 0);
  ExpectGet(map, "$c"_sv, 2);

  map.Push();
  map.NewUnbound();
  map.NewBound("$d"_sv);
  // 0  1  2  3  4
  // -- $d $a -- $c
  ExpectGet(map, "$a"_sv, 2);
  ExpectGet(map, "$c"_sv, 4);
  ExpectGet(map, "$d"_sv, 1);

  map.Push();
  map.NewBound("$e"_sv);
  map.NewBound("$f"_sv);
  map.NewBound("$g"_sv);
  // 0  1  2  3  4  5  6  7
  // $e $f $g -- $d $a -- $c
  ExpectGet(map, "$a"_sv, 5);
  ExpectGet(map, "$c"_sv, 7);
  ExpectGet(map, "$d"_sv, 4);
  ExpectGet(map, "$e"_sv, 0);
  ExpectGet(map, "$f"_sv, 1);
  ExpectGet(map, "$g"_sv, 2);

  map.Pop();
  // 0  1  2  3  4
  // -- $d $a -- $c
  ExpectGet(map, "$a"_sv, 2);
  ExpectGet(map, "$c"_sv, 4);
  ExpectGet(map, "$d"_sv, 1);

  map.Push();
  map.NewUnbound();
  map.NewUnbound();
  // 0  1  2  3  4  5  6
  // -- -- -- $d $a -- $c
  ExpectGet(map, "$a"_sv, 4);
  ExpectGet(map, "$c"_sv, 6);
  ExpectGet(map, "$d"_sv, 3);

  map.Pop();
  map.Pop();
  // 0  1  2
  // $a -- $c
  ExpectGet(map, "$a"_sv, 0);
  ExpectGet(map, "$c"_sv, 2);
}

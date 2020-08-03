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

#include "wasp/valid/local_map.h"

#include "gtest/gtest.h"

#include "test/binary/constants.h"

using namespace ::wasp;
using namespace ::wasp::valid;
using namespace ::wasp::binary::test;

void ExpectTypes(const LocalMap& locals,
                 const binary::ValueTypeList& value_types) {
  ASSERT_EQ(value_types.size(), locals.GetCount());
  for (Index i = 0; i < value_types.size(); ++i) {
    const auto& value_type = value_types[i];
    EXPECT_EQ(value_type, locals.GetType(i)) << "at index " << i;
  }
  EXPECT_EQ(nullopt, locals.GetType(locals.GetCount() + 1));
}

TEST(ValidLocalMapTest, Append_CountType) {
  LocalMap locals;
  EXPECT_TRUE(locals.Append(1, VT_I32));
  EXPECT_TRUE(locals.Append(2, VT_F32));
  EXPECT_TRUE(locals.Append(3, VT_I64));

  ExpectTypes(locals, {VT_I32, VT_F32, VT_F32, VT_I64, VT_I64, VT_I64});
}

TEST(ValidLocalMapTest, Append_ValueTypeList) {
  LocalMap locals;
  EXPECT_TRUE(locals.Append({VT_I32, VT_F32, VT_F32, VT_I64, VT_I64, VT_I32}));

  ExpectTypes(locals, {VT_I32, VT_F32, VT_F32, VT_I64, VT_I64, VT_I32});
}

TEST(ValidLocalMapTest, Append_TooMany) {
  LocalMap locals;
  EXPECT_TRUE(locals.Append(0xffff'ffff, VT_I64)); // Maximum is 2**32 - 1.

  EXPECT_EQ(VT_I64, locals.GetType(0xffff'fffe));
  EXPECT_EQ(nullopt, locals.GetType(0xffff'ffff));

  EXPECT_FALSE(locals.Append(1, VT_I32));
  EXPECT_FALSE(locals.Append({VT_I32}));
}

TEST(ValidLocalMapTest, Reset) {
  LocalMap locals;
  EXPECT_TRUE(locals.Append(100, VT_I32));
  EXPECT_TRUE(locals.Append({VT_F32, VT_I64}));

  EXPECT_EQ(102u, locals.GetCount());

  locals.Reset();

  EXPECT_EQ(0u, locals.GetCount());
  EXPECT_EQ(nullopt, locals.GetType(0));
}

TEST(ValidLocalMapTest, PushPop) {
  LocalMap locals;

  EXPECT_TRUE(locals.Append(2, VT_I32));
  ExpectTypes(locals, {VT_I32, VT_I32});

  locals.Push();
  EXPECT_TRUE(locals.Append(2, VT_F32));
  ExpectTypes(locals, {VT_F32, VT_F32, VT_I32, VT_I32});

  locals.Pop();
  ExpectTypes(locals, {VT_I32, VT_I32});
}

TEST(ValidLocalMapTest, PushPop_SameType) {
  LocalMap locals;

  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32});

  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32, VT_I32});

  // Push a let block, then append the same type. Make sure that popping
  // afterward still works.
  locals.Push();
  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32, VT_I32, VT_I32});

  locals.Pop();
  ExpectTypes(locals, {VT_I32, VT_I32});
}

TEST(ValidLocalMapTest, PushPop_Multi) {
  LocalMap locals;

  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32});

  locals.Push();
  EXPECT_TRUE(locals.Append(1, VT_F32));
  ExpectTypes(locals, {VT_F32, VT_I32});

  // Push a let block, then append twice. The second append should come after
  // the first.
  locals.Push();
  EXPECT_TRUE(locals.Append(1, VT_F64));
  ExpectTypes(locals, {VT_F64, VT_F32, VT_I32});
  EXPECT_TRUE(locals.Append(1, VT_I64));
  ExpectTypes(locals, {VT_F64, VT_I64, VT_F32, VT_I32});

  locals.Pop();
  ExpectTypes(locals, {VT_F32, VT_I32});

  locals.Pop();
  ExpectTypes(locals, {VT_I32});
}

TEST(ValidLocalMapTest, PushPop_TooMany) {
  LocalMap locals;

  EXPECT_TRUE(locals.Append(100, VT_I32));
  EXPECT_EQ(100u, locals.GetCount());

  locals.Push();
  EXPECT_TRUE(locals.Append(0xffff'ffff - 100, VT_I32));
  EXPECT_FALSE(locals.Append(1, VT_I32));

  locals.Pop();
  EXPECT_EQ(100u, locals.GetCount());
}

TEST(ValidLocalMapTest, PopToEmpty) {
  LocalMap locals;
  ExpectTypes(locals, {});

  locals.Push();
  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32});

  locals.Push();
  EXPECT_TRUE(locals.Append(1, VT_I32));
  ExpectTypes(locals, {VT_I32, VT_I32});

  locals.Pop();
  ExpectTypes(locals, {VT_I32});

  locals.Pop();
  ExpectTypes(locals, {});
}

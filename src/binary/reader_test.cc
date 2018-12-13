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

#include "src/binary/reader.h"

#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "src/base/types.h"

using namespace ::wasp;
using namespace ::wasp::binary;

namespace {

struct ErrorContext {
  SpanU8 pos;
  std::string desc;
};

struct ErrorContextLoc {
  SpanU8::index_type pos;
  std::string desc;
};

using Error = std::vector<ErrorContext>;
using ExpectedError = std::vector<ErrorContextLoc>;

class TestErrors {
 public:
  void PushContext(SpanU8 pos, string_view desc) {
    context_stack.push_back(ErrorContext{pos, desc.to_string()});
  }

  void PopContext() {
    context_stack.pop_back();
  }

  void OnError(SpanU8 pos, string_view message) {
    errors.emplace_back();
    auto& error = errors.back();
    for (const auto& ctx: context_stack) {
      error.push_back(ctx);
    }
    error.push_back(ErrorContext{pos, message.to_string()});
  }

  std::vector<ErrorContext> context_stack;
  std::vector<Error> errors;
};

template <size_t N>
SpanU8 MakeSpanU8(const char (&str)[N]) {
  return SpanU8{
      reinterpret_cast<const u8*>(str),
      static_cast<SpanU8::index_type>(N - 1)};  // -1 to remove \0 at end.
}

void ExpectNoErrors(const TestErrors& errors) {
  EXPECT_TRUE(errors.errors.empty());
  EXPECT_TRUE(errors.context_stack.empty());
}

void ExpectError(const ExpectedError& expected,
                 const TestErrors& errors,
                 SpanU8 orig_data) {
  EXPECT_TRUE(errors.context_stack.empty());
  ASSERT_EQ(1u, errors.errors.size());
  const Error& actual = errors.errors[0];
  ASSERT_EQ(actual.size(), expected.size());
  for (size_t i = 0; i < actual.size(); ++i) {
    EXPECT_EQ(expected[i].pos, actual[i].pos.data() - orig_data.data());
    EXPECT_EQ(expected[i].desc, actual[i].desc);
  }
}

template <typename T>
void ExpectEmptyOptional(const optional<T>& actual) {
  EXPECT_FALSE(actual.has_value());
}

template <typename T>
void ExpectOptional(const T& expected, const optional<T>& actual) {
  ASSERT_TRUE(actual.has_value());
  EXPECT_EQ(expected, *actual);
}

template <typename T>
void ExpectRead(const T& expected, SpanU8 data) {
  TestErrors errors;
  auto result = Read<T>(&data, errors);
  ExpectNoErrors(errors);
  ExpectOptional(expected, result);
  EXPECT_EQ(0u, data.size());
}

template <typename T>
void ExpectReadFailure(const ExpectedError& expected, SpanU8 data) {
  TestErrors errors;
  SpanU8 orig_data = data;
  auto result = Read<T>(&data, errors);
  ExpectError(expected, errors, orig_data);
  ExpectEmptyOptional(result);
}

}  // namespace

TEST(ReaderTest, ReadU8) {
  ExpectRead<u8>(32, MakeSpanU8("\x20"));
  ExpectReadFailure<u8>({{0, "Unable to read u8"}}, MakeSpanU8(""));
}

TEST(ReaderTest, ReadBytes) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, errors);
  ExpectNoErrors(errors);
  ExpectOptional(data, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadBytes_Leftovers) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, errors);
  ExpectNoErrors(errors);
  ExpectOptional(data.subspan(0, 2), result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadBytes_Fail) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, errors);
  ExpectEmptyOptional(result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST(ReaderTest, ReadU32) {
  ExpectRead<u32>(32u, MakeSpanU8("\x20"));
  ExpectRead<u32>(448u, MakeSpanU8("\xc0\x03"));
  ExpectRead<u32>(33360u, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<u32>(101718048u, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<u32>(1042036848u, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
}

TEST(ReaderTest, ReadU32_TooLong) {
  ExpectReadFailure<u32>(
      {{0, "vu32"},
       {5, "Last byte of vu32 must be zero extension: expected 0x2, got 0x12"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\x12"));
}

TEST(ReaderTest, ReadU32_PastEnd) {
  ExpectReadFailure<u32>({{0, "vu32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<u32>({{0, "vu32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<u32>({{0, "vu32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<u32>({{0, "vu32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<u32>({{0, "vu32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReaderTest, ReadS32) {
  ExpectRead<s32>(32, MakeSpanU8("\x20"));
  ExpectRead<s32>(-16, MakeSpanU8("\x70"));
  ExpectRead<s32>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s32>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s32>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s32>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s32>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s32>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s32>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s32>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
}

TEST(ReaderTest, ReadS32_TooLong) {
  ExpectReadFailure<s32>({{0, "vs32"},
                          {5,
                           "Last byte of vs32 must be sign extension: expected "
                           "0x5 or 0x7d, got 0x15"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0\x15"));
  ExpectReadFailure<s32>({{0, "vs32"},
                          {5,
                           "Last byte of vs32 must be sign extension: expected "
                           "0x3 or 0x7b, got 0x73"}},
                         MakeSpanU8("\xff\xff\xff\xff\x73"));
}

TEST(ReaderTest, ReadS32_PastEnd) {
  ExpectReadFailure<s32>({{0, "vs32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s32>({{0, "vs32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s32>({{0, "vs32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s32>({{0, "vs32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s32>({{0, "vs32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReaderTest, ReadS64) {
  ExpectRead<s64>(32, MakeSpanU8("\x20"));
  ExpectRead<s64>(-16, MakeSpanU8("\x70"));
  ExpectRead<s64>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s64>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s64>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s64>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s64>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s64>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s64>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s64>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
  ExpectRead<s64>(13893120096, MakeSpanU8("\xe0\xe0\xe0\xe0\x33"));
  ExpectRead<s64>(-12413554592, MakeSpanU8("\xe0\xe0\xe0\xe0\x51"));
  ExpectRead<s64>(1533472417872, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"));
  ExpectRead<s64>(-287593715632, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"));
  ExpectRead<s64>(139105536057408, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"));
  ExpectRead<s64>(-124777254608832, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"));
  ExpectRead<s64>(1338117014066474,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"));
  ExpectRead<s64>(-12172681868045014,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"));
  ExpectRead<s64>(1070725794579330814,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"));
  ExpectRead<s64>(-3540960223848057090,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"));
}

TEST(ReaderTest, ReadS64_TooLong) {
  ExpectReadFailure<s64>(
      {{0, "vs64"},
       {10,
        "Last byte of vs64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xf0"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>(
      {{0, "vs64"},
       {10,
        "Last byte of vs64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xff"}},
      MakeSpanU8("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"));
}

TEST(ReaderTest, ReadS64_PastEnd) {
  ExpectReadFailure<s64>({{0, "vs64"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s64>({{0, "vs64"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s64>({{0, "vs64"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {5, "Unable to read u8"}},
                         MakeSpanU8("\xe0\xe0\xe0\xe0\xe0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {6, "Unable to read u8"}},
                         MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\xc0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {7, "Unable to read u8"}},
                         MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x84"));
  ExpectReadFailure<s64>({{0, "vs64"}, {8, "Unable to read u8"}},
                         MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "vs64"}, {9, "Unable to read u8"}},
                         MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"));
}

TEST(ReaderTest, ReadF32) {
  ExpectRead<f32>(0.0f, MakeSpanU8("\x00\x00\x00\x00"));
  ExpectRead<f32>(-1.0f, MakeSpanU8("\x00\x00\x80\xbf"));
  ExpectRead<f32>(1234567.0f, MakeSpanU8("\x38\xb4\x96\x49"));
  ExpectRead<f32>(INFINITY, MakeSpanU8("\x00\x00\x80\x7f"));
  ExpectRead<f32>(-INFINITY, MakeSpanU8("\x00\x00\x80\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\xc0\x7f");
    TestErrors errors;
    auto result = Read<f32>(&data, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, ReadF32_PastEnd) {
  ExpectReadFailure<f32>({{0, "Unable to read 4 bytes"}},
                         MakeSpanU8("\x00\x00\x00"));
}

TEST(ReaderTest, ReadF64) {
  ExpectRead<f64>(0.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<f64>(-1.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xbf"));
  ExpectRead<f64>(111111111111111,
                  MakeSpanU8("\xc0\x71\xbc\x93\x84\x43\xd9\x42"));
  ExpectRead<f64>(INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\x7f"));
  ExpectRead<f64>(-INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf8\x7f");
    TestErrors errors;
    auto result = Read<f64>(&data, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, ReadF64_PastEnd) {
  ExpectReadFailure<f64>({{0, "Unable to read 8 bytes"}},
                         MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00"));
}

TEST(ReaderTest, ReadCount) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectNoErrors(errors);
  ExpectOptional(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadCount_PastEnd) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectError({{1, "Count is longer than the data length: 5 > 3"}}, errors,
              data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadStr) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadStr(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(string_view{"hello"}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadStr_Leftovers) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01more");
  SpanU8 copy = data;
  auto result = ReadStr(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(string_view{"m"}, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadStr_FailLength) {
  {
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("");
    SpanU8 copy = data;
    auto result = ReadStr(&copy, errors, "test");
    ExpectError({{0, "test"}, {0, "index"}, {0, "Unable to read u8"}}, errors,
                data);
    ExpectEmptyOptional(result);
    EXPECT_EQ(0u, copy.size());
  }

  {
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("\xc0");
    SpanU8 copy = data;
    auto result = ReadStr(&copy, errors, "test");
    ExpectError({{0, "test"}, {0, "index"}, {1, "Unable to read u8"}}, errors,
                data);
    ExpectEmptyOptional(result);
    EXPECT_EQ(0u, copy.size());
  }
}

TEST(ReaderTest, ReadStr_Fail) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x06small");
  SpanU8 copy = data;
  auto result = ReadStr(&copy, errors, "test");
  ExpectError({{0, "test"}, {1, "Count is longer than the data length: 6 > 5"}},
              errors, data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(5u, copy.size());
}

TEST(ReaderTest, ReadVec_u8) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadVec<u8>(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(std::vector<u8>{'h', 'e', 'l', 'l', 'o'}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVec_u32) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c");
  SpanU8 copy = data;
  auto result = ReadVec<u32>(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(std::vector<u32>{5, 128, 206412}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVec_FailLength) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05");
  SpanU8 copy = data;
  auto result = ReadVec<u32>(&copy, errors, "test");
  ExpectError({{0, "test"}, {1, "Count is longer than the data length: 2 > 1"}},
              errors, data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadVec_PastEnd) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05"
      "\x80");
  SpanU8 copy = data;
  auto result = ReadVec<u32>(&copy, errors, "test");
  ExpectError({{0, "test"},
               {2, "vu32"},
               {3, "Unable to read u8"}},
              errors, data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadValType) {
  ExpectRead<ValType>(ValType::I32, MakeSpanU8("\x7f"));
  ExpectRead<ValType>(ValType::I64, MakeSpanU8("\x7e"));
  ExpectRead<ValType>(ValType::F32, MakeSpanU8("\x7d"));
  ExpectRead<ValType>(ValType::F64, MakeSpanU8("\x7c"));
  ExpectRead<ValType>(ValType::Anyfunc, MakeSpanU8("\x70"));
  ExpectRead<ValType>(ValType::Func, MakeSpanU8("\x60"));
  ExpectRead<ValType>(ValType::Void, MakeSpanU8("\x40"));
}

TEST(ReaderTest, ReadValType_Unknown) {
  ExpectReadFailure<ValType>({{0, "value type"}, {1, "Unknown value type 16"}},
                             MakeSpanU8("\x10"));
}

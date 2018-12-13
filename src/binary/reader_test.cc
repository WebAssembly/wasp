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

TEST(ReaderTest, ReadBytes_Ok) {
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

TEST(ReaderTest, ReadU32_Ok) {
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

TEST(ReaderTest, ReadS32_Ok) {
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

TEST(ReaderTest, ReadS64_Ok) {
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

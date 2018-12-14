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

namespace wasp {
namespace binary {
namespace test {

template <size_t N>
SpanU8 MakeSpanU8(const char (&str)[N]) {
  return SpanU8{
      reinterpret_cast<const u8*>(str),
      static_cast<SpanU8::index_type>(N - 1)};  // -1 to remove \0 at end.
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

}  // namespace test
}  // namespace binary
}  // namespace wasp

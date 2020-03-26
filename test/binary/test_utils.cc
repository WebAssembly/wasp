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

#include "test/binary/test_utils.h"

#include "gtest/gtest.h"

namespace wasp {
namespace binary {
namespace test {

std::string ErrorToString(const Error& error) {
  std::string result;
  bool first = true;
  for (auto&& s : error) {
    if (!first) {
      result += ": ";
    }
    first = false;
    result += s.desc;
  }
  return result;
}

std::string TestErrorsToString(const TestErrors& errors) {
  std::string result;
  for (auto&& error : errors.errors) {
    result += ErrorToString(error) + "\n";
  }
  return result;
}

void TestErrors::HandlePushContext(Location loc, string_view desc) {
  context_stack.push_back(ErrorContext{loc, std::string{desc}});
}

void TestErrors::HandlePopContext() {
  context_stack.pop_back();
}

void TestErrors::HandleOnError(Location loc, string_view message) {
  errors.emplace_back();
  auto& error = errors.back();
  error = context_stack;
  error.push_back(ErrorContext{loc, std::string{message}});
}

void ExpectNoErrors(const TestErrors& errors) {
  EXPECT_TRUE(errors.errors.empty()) << TestErrorsToString(errors);
  EXPECT_TRUE(errors.context_stack.empty());
}

void ExpectErrors(const std::vector<ExpectedError>& expected_errors,
                  const TestErrors& errors,
                  SpanU8 orig_data) {
  EXPECT_TRUE(errors.context_stack.empty());
  ASSERT_EQ(expected_errors.size(), errors.errors.size());
  for (size_t j = 0; j < expected_errors.size(); ++j) {
    const ExpectedError& expected = expected_errors[j];
    const Error& actual = errors.errors[j];
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].pos, actual[i].loc.data() - orig_data.data());
      EXPECT_EQ(expected[i].desc, actual[i].desc);
    }
  }
}

void ExpectError(const ExpectedError& expected,
                 const TestErrors& errors,
                 SpanU8 orig_data) {
  ExpectErrors({expected}, errors, orig_data);
}

}  // namespace test
}  // namespace binary
}  // namespace wasp

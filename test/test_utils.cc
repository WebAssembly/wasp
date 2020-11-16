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

#include "test/test_utils.h"

#include "gtest/gtest.h"

namespace wasp::test {

std::string ErrorListToString(const ErrorList& error) {
  std::string result;
  bool first = true;
  for (auto&& s : error) {
    if (!first) {
      result += ": ";
    }
    first = false;
    result += s.message;
  }
  return result;
}

std::string TestErrorsToString(const TestErrors& errors) {
  std::string result;
  for (auto&& error : errors.errors) {
    result += ErrorListToString(error) + "\n";
  }
  return result;
}

void TestErrors::Clear() {
  context_stack.clear();
  errors.clear();
}

bool TestErrors::HasError() const {
  return !errors.empty();
}

void TestErrors::HandlePushContext(Location loc, string_view desc) {
  context_stack.push_back(Error{loc, std::string{desc}});
}

void TestErrors::HandlePopContext() {
  context_stack.pop_back();
}

void TestErrors::HandleOnError(Location loc, string_view message) {
  errors.emplace_back();
  auto& error = errors.back();
  error = context_stack;
  error.push_back(Error{loc, std::string{message}});
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
    const ErrorList& actual = errors.errors[j];
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].pos, actual[i].loc.data() - orig_data.data());
      EXPECT_EQ(expected[i].message, actual[i].message);
    }
  }
}

void ExpectError(const ExpectedError& expected,
                 const TestErrors& errors,
                 SpanU8 orig_data) {
  ExpectErrors({expected}, errors, orig_data);
}

void ExpectErrors(const std::vector<ErrorList>& expected_errors,
                  const TestErrors& errors) {
  EXPECT_TRUE(errors.context_stack.empty());
  ASSERT_EQ(expected_errors.size(), errors.errors.size());
  for (size_t j = 0; j < expected_errors.size(); ++j) {
    const ErrorList& expected = expected_errors[j];
    const ErrorList& actual = errors.errors[j];
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].loc, actual[i].loc);
      EXPECT_EQ(expected[i].message, actual[i].message);
    }
  }
}

void ExpectError(const ErrorList& expected, const TestErrors& errors) {
  ExpectErrors({expected}, errors);
}

}  // namespace wasp::test

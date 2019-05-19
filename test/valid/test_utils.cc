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

#include "test/valid/test_utils.h"

#include "gtest/gtest.h"

namespace wasp {
namespace valid {
namespace test {

void TestErrors::HandlePushContext(string_view desc) {
  context_stack.push_back(desc.to_string());
}

void TestErrors::HandlePopContext() {
  context_stack.pop_back();
}

void TestErrors::HandleOnError(string_view message) {
  errors.emplace_back();
  auto& error = errors.back();
  error = context_stack;
  error.push_back(message.to_string());
}

void ExpectNoErrors(const TestErrors& errors) {
  EXPECT_TRUE(errors.errors.empty());
  EXPECT_TRUE(errors.context_stack.empty());
}

void ExpectErrors(const std::vector<ExpectedError>& expected_errors,
                  TestErrors& errors) {
  EXPECT_TRUE(errors.context_stack.empty());
  EXPECT_EQ(expected_errors, errors.errors);
  ClearErrors(errors);
}

void ExpectError(const ExpectedError& expected,
                 TestErrors& errors) {
  ExpectErrors({expected}, errors);
  ClearErrors(errors);
}

void ExpectErrorSubstr(const ExpectedError& expected, TestErrors& errors) {
  auto substr = [](const std::string& str, const std::string& sub) {
    return str.find(sub) != std::string::npos;
  };

  ASSERT_EQ(1, errors.errors.size());
  ASSERT_EQ(expected.size(), errors.errors[0].size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_PRED2(substr, errors.errors[0][i], expected[i]);
  }
  ClearErrors(errors);
}

void ClearErrors(TestErrors& errors) {
  errors.errors.clear();
}

}  // namespace test
}  // namespace binary
}  // namespace wasp


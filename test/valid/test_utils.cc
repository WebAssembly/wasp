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

void ExpectErrors(const std::vector<ExpectedError>& expected_errors,
                  TestErrors& errors) {
  // TODO: Share w/ binary/test_utils.cc
  EXPECT_TRUE(errors.context_stack.empty());
  ASSERT_EQ(expected_errors.size(), errors.errors.size());
  for (size_t j = 0; j < expected_errors.size(); ++j) {
    const ExpectedError& expected = expected_errors[j];
    const Error& actual = errors.errors[j];
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i], actual[i].desc);
    }
  }

  ClearErrors(errors);
}

void ExpectError(const ExpectedError& expected,
                 TestErrors& errors) {
  // TODO: Share w/ binary/test_utils.cc
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
    EXPECT_PRED2(substr, errors.errors[0][i].desc, expected[i]);
  }
  ClearErrors(errors);
}

void ClearErrors(TestErrors& errors) {
  errors.context_stack.clear();
  errors.errors.clear();
}

}  // namespace test
}  // namespace binary
}  // namespace wasp


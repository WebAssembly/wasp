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

#ifndef WASP_BINARY_TEST_UTILS_H_
#define WASP_BINARY_TEST_UTILS_H_

#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "src/base/types.h"
#include "src/binary/types.h"

namespace wasp {
namespace binary {
namespace test {

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
  void PushContext(SpanU8 pos, string_view desc);
  void PopContext();
  void OnError(SpanU8 pos, string_view message);

  std::vector<ErrorContext> context_stack;
  std::vector<Error> errors;
};

template <size_t N>
SpanU8 MakeSpanU8(const char (&str)[N]);

template <size_t N>
Expression MakeExpression(const char (&str)[N]);

template <size_t N>
ConstantExpression MakeConstantExpression(const char (&str)[N]);

void ExpectNoErrors(const TestErrors&);
void ExpectErrors(const std::vector<ExpectedError>&,
                  const TestErrors&,
                  SpanU8 orig_data);
void ExpectError(const ExpectedError&, const TestErrors&, SpanU8 orig_data);

}  // namespace test
}  // namespace binary
}  // namespace wasp

#include "src/binary/test_utils-inl.h"

#endif // WASP_BINARY_TEST_UTILS_H_

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

#ifndef WASP_TEST_UTILS_H_
#define WASP_TEST_UTILS_H_

#include <vector>

#include "wasp/base/error.h"
#include "wasp/base/errors.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp::test {

struct ErrorWithPos {
  span_extent_t pos;
  std::string message;
};

using ErrorList = std::vector<Error>;
using ExpectedError = std::vector<ErrorWithPos>;

class TestErrors : public Errors {
 public:
  std::vector<Error> context_stack;
  std::vector<ErrorList> errors;

  void Clear();
  bool HasError() const override;

 protected:
  void HandlePushContext(Location loc, string_view desc) override;
  void HandlePopContext() override;
  void HandleOnError(Location loc, string_view message) override;
};

void ExpectNoErrors(const TestErrors&);
void ExpectErrors(const std::vector<ExpectedError>&,
                  const TestErrors&,
                  SpanU8 orig_data);
void ExpectError(const ExpectedError&, const TestErrors&, SpanU8 orig_data);

// Expected errors with full Location.
void ExpectErrors(const std::vector<ErrorList>&, const TestErrors&);
void ExpectError(const ErrorList&, const TestErrors&);

}  // namespace wasp::test

#endif // WASP_TEST_UTILS_H_

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

#ifndef WASP_VALID_TEST_UTILS_H_
#define WASP_VALID_TEST_UTILS_H_

#include <string>
#include <vector>

#include "wasp/base/format.h"
#include "wasp/valid/errors.h"

namespace wasp {
namespace valid {
namespace test {

using Error = std::vector<std::string>;
using ExpectedError = std::vector<std::string>;

class TestErrors : public Errors {
 public:
  std::vector<std::string> context_stack;
  std::vector<Error> errors;

 protected:
  void HandlePushContext(string_view desc);
  void HandlePopContext();
  void HandleOnError(string_view message);
};

void ExpectNoErrors(const TestErrors&);
void ExpectErrors(const std::vector<ExpectedError>&, TestErrors&);
void ExpectError(const ExpectedError&, TestErrors&);
void ExpectErrorSubstr(const ExpectedError&, TestErrors&);
void ClearErrors(TestErrors&);

}  // namespace test
}  // namespace valid
}  // namespace wasp

#endif // WASP_VALID_TEST_UTILS_H_

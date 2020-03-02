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

#include "test/binary/test_utils.h"

namespace wasp {
namespace valid {
namespace test {

using Error = wasp::binary::test::Error;
using TestErrors = wasp::binary::test::TestErrors;

// TODO: Support location data in tests?
using ExpectedError = std::vector<std::string>;

void ExpectErrors(const std::vector<ExpectedError>&, TestErrors&);
void ExpectError(const ExpectedError& expected, TestErrors&);
void ExpectErrorSubstr(const ExpectedError&, TestErrors&);
void ClearErrors(TestErrors&);

}  // namespace test
}  // namespace valid
}  // namespace wasp

#endif // WASP_VALID_TEST_UTILS_H_

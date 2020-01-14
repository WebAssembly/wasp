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

#ifndef WASP_BINARY_READ_TEST_UTILS_H_
#define WASP_BINARY_READ_TEST_UTILS_H_

#include <utility>
#include "gtest/gtest.h"
#include "test/binary/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/binary/read.h"

namespace wasp {
namespace binary {
namespace test {

template <typename T, typename... Args>
void ExpectRead(const T& expected,
                wasp::SpanU8 data,
                const wasp::Features& features,
                Args&&... args) {
  wasp::binary::test::TestErrors errors;
  auto result = wasp::binary::Read<T>(&data, features, errors,
                                      std::forward<Args>(args)...);
  wasp::binary::test::ExpectNoErrors(errors);
  EXPECT_EQ(expected, result);
  EXPECT_EQ(0u, data.size());
}

template <typename T>
void ExpectRead(const T& expected, wasp::SpanU8 data) {
  ExpectRead<T>(expected, data, Features{});
}

template <typename T, typename... Args>
void ExpectReadFailure(const wasp::binary::test::ExpectedError& expected,
                       wasp::SpanU8 data,
                       const wasp::Features& features,
                       Args&&... args) {
  wasp::binary::test::TestErrors errors;
  const wasp::SpanU8 orig_data = data;
  auto result = wasp::binary::Read<T>(&data, features, errors,
                                      std::forward<Args>(args)...);
  wasp::binary::test::ExpectError(expected, errors, orig_data);
  EXPECT_EQ(wasp::nullopt, result);
}

template <typename T>
void ExpectReadFailure(const wasp::binary::test::ExpectedError& expected,
                       wasp::SpanU8 data) {
  ExpectReadFailure<T>(expected, data, Features{});
}

}  // namespace test
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_TEST_UTILS_H_

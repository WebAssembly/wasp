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
#include "test/test_utils.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/binary/read.h"
#include "wasp/binary/read/context.h"

namespace wasp {
namespace binary {
namespace test {

template <typename T, typename... Args>
void ExpectRead(const T& expected,
                wasp::SpanU8 data,
                const wasp::Features& features,
                Args&&... args) {
  wasp::test::TestErrors errors;
  wasp::binary::Context context{features, errors};
  auto result =
      wasp::binary::Read<T>(&data, context, std::forward<Args>(args)...);
  wasp::test::ExpectNoErrors(errors);
  EXPECT_EQ(0u, data.size());
  EXPECT_NE(nullptr, result->loc().data());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(expected, **result);
}

template <typename T>
void ExpectRead(const T& expected, wasp::SpanU8 data) {
  ExpectRead<T>(expected, data, Features{});
}

template <typename T, typename... Args>
void ExpectReadFailure(const wasp::test::ExpectedError& expected,
                       wasp::SpanU8 data,
                       const wasp::Features& features,
                       Args&&... args) {
  wasp::test::TestErrors errors;
  wasp::binary::Context context{features, errors};
  const wasp::SpanU8 orig_data = data;
  auto result =
      wasp::binary::Read<T>(&data, context, std::forward<Args>(args)...);
  wasp::test::ExpectError(expected, errors, orig_data);
  EXPECT_EQ(wasp::nullopt, result);
}

template <typename T>
void ExpectReadFailure(const wasp::test::ExpectedError& expected,
                       wasp::SpanU8 data) {
  ExpectReadFailure<T>(expected, data, Features{});
}

}  // namespace test
}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_TEST_UTILS_H_

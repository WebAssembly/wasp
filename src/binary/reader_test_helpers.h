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

#include "wasp/binary/read/read.h"

#include "src/binary/test_utils.h"
#include "wasp/base/span.h"

#include "gtest/gtest.h"

template <typename T>
void ExpectRead(const T& expected, wasp::SpanU8 data) {
  wasp::binary::test::TestErrors errors;
  auto result = wasp::binary::Read<T>(&data, errors);
  wasp::binary::test::ExpectNoErrors(errors);
  EXPECT_EQ(expected, result);
  EXPECT_EQ(0u, data.size());
}

template <typename T>
void ExpectReadFailure(const wasp::binary::test::ExpectedError& expected,
                       wasp::SpanU8 data) {
  wasp::binary::test::TestErrors errors;
  const wasp::SpanU8 orig_data = data;
  auto result = wasp::binary::Read<T>(&data, errors);
  wasp::binary::test::ExpectError(expected, errors, orig_data);
  EXPECT_EQ(wasp::nullopt, result);
}

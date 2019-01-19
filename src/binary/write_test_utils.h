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

#include <iterator>
#include <vector>

#include "wasp/base/features.h"
#include "wasp/base/span.h"

#include "gtest/gtest.h"

template <typename T>
void ExpectWrite(wasp::SpanU8 expected,
                 const T& value,
                 const wasp::Features& features = wasp::Features{}) {
  std::vector<wasp::u8> result;
  wasp::binary::Write(value, std::back_inserter(result), features);
  EXPECT_EQ(expected, wasp::SpanU8{result});
}

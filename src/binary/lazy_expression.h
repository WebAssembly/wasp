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

#ifndef WASP_BINARY_LAZY_EXPRESSION_H
#define WASP_BINARY_LAZY_EXPRESSION_H

#include "src/base/span.h"
#include "src/binary/lazy_sequence.h"
#include "src/binary/types.h"

namespace wasp {
namespace binary {

/// ---
template <typename Errors>
using LazyExpression = LazySequence<Instruction, Errors>;

template <typename Errors>
LazyExpression<Errors> ReadExpression(SpanU8 data, Errors& errors) {
  return LazyExpression<Errors>{data, errors};
}

template <typename Errors>
LazyExpression<Errors> ReadExpression(Expression expr, Errors& errors) {
  return ReadExpression(expr.data, errors);
}

template <typename Errors>
LazyExpression<Errors> ReadExpression(ConstantExpression expr, Errors& errors) {
  return ReadExpression(expr.data, errors);
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_LAZY_EXPRESSION_H

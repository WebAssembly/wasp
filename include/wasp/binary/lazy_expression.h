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

#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/binary/expression.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/read/read_instruction.h"

namespace wasp {
namespace binary {

/// ---
using LazyExpression = LazySequence<Instruction>;

inline LazyExpression ReadExpression(SpanU8 data,
                                     const Features& features,
                                     Errors& errors) {
  return LazyExpression{data, features, errors};
}

inline LazyExpression ReadExpression(Expression expr,
                                     const Features& features,
                                     Errors& errors) {
  return ReadExpression(expr.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_LAZY_EXPRESSION_H

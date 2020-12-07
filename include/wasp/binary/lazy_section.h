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

#ifndef WASP_BINARY_LAZY_SECTION_H_
#define WASP_BINARY_LAZY_SECTION_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/read.h"
#include "wasp/binary/read/read_ctx.h"

namespace wasp::binary {

struct ReadCtx;

template <typename T>
class LazySection {
 public:
  explicit LazySection(SpanU8, string_view name, ReadCtx&);

  OptAt<Index> count;
  LazySequence<T> sequence;
};

template <typename T>
LazySection<T>::LazySection(SpanU8 data, string_view name, ReadCtx& ctx)
    : count{ReadCount(&data, ctx)}, sequence{data, count, name, ctx} {}

}  // namespace wasp::binary

#endif // WASP_BINARY_LAZY_SECTION_H_

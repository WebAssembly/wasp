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

#include "wasp/binary/read/read.h"

namespace wasp {
namespace binary {

template <typename Sequence>
LazySequenceIterator<Sequence>::LazySequenceIterator(const Sequence* seq,
                                                     SpanU8 data)
    : sequence_(seq), data_(data) {
  if (empty()) {
    clear();
  } else {
    operator++();
  }
}

template <typename Sequence>
auto LazySequenceIterator<Sequence>::operator++() -> LazySequenceIterator& {
  if (empty()) {
    clear();
  } else {
    value_ = Read<value_type>(&data_, sequence_->features_, sequence_->errors_);
    if (!value_) {
      clear();
    }
  }
  return *this;
}

template <typename Sequence>
auto LazySequenceIterator<Sequence>::operator++(int) -> LazySequenceIterator {
  auto temp = *this;
  operator++();
  return temp;
}

}  // namespace binary
}  // namespace wasp

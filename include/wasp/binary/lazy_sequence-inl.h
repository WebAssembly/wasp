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

#include "wasp/binary/read.h"

namespace wasp::binary {

template <typename T>
void LazySequence<T>::NotifyRead(const u8* pos, bool ok) {
  if (pos > last_pos_) {
    last_pos_ = pos;
    if (ok) {
      count_++;
    } else if (expected_count_ && count_ != *expected_count_) {
      // Reached the end, but there was a mismatch.
      LazySequenceBase::OnCountError(context_.errors,
                                     MakeSpan(last_pos_, data_.end()), name_,
                                     *expected_count_, count_);
    }
  }
}

template <typename Sequence>
LazySequenceIterator<Sequence>::LazySequenceIterator(Sequence* seq, SpanU8 data)
    : sequence_(seq), data_(data) {
  if (empty()) {
    clear();
  } else {
    operator++();
  }
}

template <typename Sequence>
auto LazySequenceIterator<Sequence>::operator++() -> LazySequenceIterator& {
  const u8* pos = data_.data();
  if (empty()) {
    sequence_->NotifyRead(pos, false);
    clear();
  } else {
    value_ = Read<typename value_type::value_type>(&data_, sequence_->context_);
    sequence_->NotifyRead(pos, !!value_);
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

}  // namespace wasp::binary

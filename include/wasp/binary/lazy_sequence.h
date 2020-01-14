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

#ifndef WASP_BINARY_LAZY_SEQUENCE_H_
#define WASP_BINARY_LAZY_SEQUENCE_H_

#include <iterator>

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp {
namespace binary {

class Errors;

template <typename Sequence>
class LazySequenceIterator;

class LazySequenceBase {
 protected:
  static void OnCountError(Errors&,
                           SpanU8,
                           string_view name,
                           Index expected,
                           Index actual);
};

/// ---
template <typename T>
class LazySequence : public LazySequenceBase {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = LazySequenceIterator<LazySequence>;
  using const_iterator = LazySequenceIterator<LazySequence>;

  explicit LazySequence(SpanU8 data, const Features& features, Errors& errors)
      : data_{data}, features_{features}, errors_{errors} {}

  explicit LazySequence(SpanU8 data,
                        optional<Index> expected_count,
                        string_view name,
                        const Features& features,
                        Errors& errors)
      : data_{data},
        features_{features},
        errors_{errors},
        name_{name},
        expected_count_{expected_count} {}

  iterator begin() { return iterator{this, data_}; }
  iterator end() { return iterator{this, SpanU8{}}; }
  const_iterator begin() const { return const_iterator{this, data_}; }
  const_iterator end() const { return const_iterator{this, SpanU8{}}; }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

 private:
  template <typename Sequence>
  friend class LazySequenceIterator;

  void NotifyRead(const u8*, bool);

  SpanU8 data_;
  const Features& features_;
  Errors& errors_;

  // Check for errors when there is an expected count.
  string_view name_;
  optional<Index> expected_count_;
  Index count_ = 0;
  // Tracks the last value read by any iterator that uses this sequence.
  const u8* last_pos_ = nullptr;
};

/// ---
template <typename Sequence>
class LazySequenceIterator {
 public:
  using difference_type = typename Sequence::difference_type;
  using value_type = typename Sequence::value_type;
  using pointer = typename Sequence::const_pointer;
  using reference = typename Sequence::const_reference;
  using iterator_category = std::forward_iterator_tag;

  explicit LazySequenceIterator(Sequence* seq, SpanU8 data);

  SpanU8 data() const { return data_; }

  reference operator*() const { return *value_; }
  pointer operator->() const { return &*value_; }

  LazySequenceIterator& operator++();
  LazySequenceIterator operator++(int);

  friend bool operator==(const LazySequenceIterator& lhs,
                         const LazySequenceIterator& rhs) {
    return lhs.data_.begin() == rhs.data_.begin();
  }

  friend bool operator!=(const LazySequenceIterator& lhs,
                         const LazySequenceIterator& rhs) {
    return !(lhs == rhs);
  }

 private:
  bool empty() const { return data_.empty(); }
  void clear() { data_ = {}; }

  Sequence* sequence_;
  SpanU8 data_;
  optional<value_type> value_;
};

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/lazy_sequence-inl.h"

#endif // WASP_BINARY_LAZY_SEQUENCE_H_

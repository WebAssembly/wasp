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

#ifndef WASP_WRITE_TEST_UTILS_H_
#define WASP_WRITE_TEST_UTILS_H_

#include <iterator>

namespace wasp {
namespace test {

template <typename Iterator>
class ClampedIterator {
 public:
  using difference_type = typename Iterator::difference_type;
  using value_type = typename Iterator::value_type;
  using pointer = typename Iterator::pointer;
  using reference = typename Iterator::reference;
  using iterator_category = typename Iterator::iterator_category;

  explicit ClampedIterator(Iterator begin, Iterator end)
      : iter_{begin}, end_{end} {}

  bool overflow() const { return overflow_; }

  reference operator*() { return iter_ != end_ ? *iter_ : dummy_; };
  pointer operator->() { return &(*this); };

  Iterator base() { return iter_; }

  ClampedIterator& operator++() {
    if (iter_ != end_) {
      ++iter_;
    } else {
      overflow_ = true;
    }
    return *this;
  }

  ClampedIterator operator++(int) {
    auto temp = *this;
    operator++();
    return temp;
  }

  friend bool operator==(const ClampedIterator& lhs,
                         const ClampedIterator& rhs) {
    return lhs.iter_ == rhs.iter_;
  }

  friend bool operator!=(const ClampedIterator& lhs,
                         const ClampedIterator& rhs) {
    return !(lhs == rhs);
  }

 private:
  Iterator iter_, end_;
  value_type dummy_{};
  bool overflow_ = false;
};

template <typename Iterator>
ClampedIterator<Iterator> MakeClampedIterator(Iterator begin, Iterator end) {
  return ClampedIterator<Iterator>{begin, end};
}

}  // namespace test
}  // namespace wasp

#endif  // WASP_WRITE_TEST_UTILS_H_

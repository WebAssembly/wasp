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

#ifndef WASP_BASE_ENUMERATE_H_
#define WASP_BASE_ENUMERATE_H_

#include <iterator>
#include <type_traits>

#include "wasp/base/types.h"

namespace wasp {
namespace enumerate_ {

template <typename I, typename T>
struct Pair {
  I index;
  T value;
};

template <typename I, typename Iter>
struct Iterator {
  using pair = Pair<I, decltype(*std::declval<Iter>())>;

  pair operator*() const;
  Iterator& operator++();
  Iterator operator++(int);

  friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
    return lhs.iter == rhs.iter;
  }

  friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
    return !(lhs == rhs);
  }

  I index;
  Iter iter;
};

template <typename I, typename Seq>
struct Sequence {
  using iterator = Iterator<I, decltype(std::begin(std::declval<Seq>()))>;

  Sequence(Seq&& seq, I start);
  iterator begin();
  iterator end();

  Seq seq;
  I start;
};

}  // namespace enumerate

template <typename Seq, typename I = Index>
enumerate_::Sequence<I, Seq> enumerate(Seq&& seq, I start = I{0});

}  // namespace wasp

#include "wasp/base/enumerate-inl.h"

#endif  // WASP_BASE_ENUMERATE_H_

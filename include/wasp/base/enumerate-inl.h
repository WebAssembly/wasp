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

#include "wasp/base/enumerate.h"

namespace wasp {
namespace enumerate_ {

template <typename I, typename Iter>
auto Iterator<I, Iter>::operator*() const -> pair {
  return pair{index, *iter};
}

template <typename I, typename Iter>
auto Iterator<I, Iter>::operator++() -> Iterator& {
  ++index;
  ++iter;
  return *this;
}

template <typename I, typename Iter>
auto Iterator<I, Iter>::operator++(int) -> Iterator {
  auto temp = *this;
  operator++();
  return temp;
}

template <typename I, typename Seq>
Sequence<I, Seq>::Sequence(Seq&& seq, I start)
    : seq{std::forward<Seq>(seq)}, start{start} {}

template <typename I, typename Seq>
auto Sequence<I, Seq>::begin() -> iterator {
  return iterator{start, seq.begin()};
}

template <typename I, typename Seq>
auto Sequence<I, Seq>::end() -> iterator {
  return iterator{0, seq.end()};
}

}  // namespace enumerate_

template <typename Seq, typename I>
enumerate_::Sequence<I, Seq> enumerate(Seq&& seq, I start) {
  return enumerate_::Sequence<I, Seq>{std::forward<Seq>(seq), start};
}

}  // namespace wasp

//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_BASE_AT_H_
#define WASP_BASE_AT_H_

#include <utility>

#include "wasp/base/hash.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"

namespace wasp {

template <typename T>
struct At : std::pair<Location, T> {
  using value_type = T;

  At() = default;
  At(T v) : std::pair<Location, T>{{}, std::move(v)} {}
  explicit At(Location loc, T v) : std::pair<Location, T>{loc, std::move(v)} {}

  At& operator=(T v) {
    this->first = Location{};
    this->second = v;
    return *this;
  }

  operator const T&() const { return this->second; }

  Location loc() const { return this->first; }

  const T& value() const { return this->second; }
  T& value() { return this->second; }

  const T* operator->() const { return &this->second; }
  T* operator->() { return &this->second; }
  const T& operator*() const { return this->second; }
  T& operator*() { return this->second; }
};

template <typename T>
using OptAt = optional<At<T>>;

template <typename T>
inline bool operator==(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() == rhs.value();
}

template <typename T>
inline bool operator!=(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() != rhs.value();
}

template <typename T>
inline bool operator<(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() < rhs.value();
}

template <typename T>
inline bool operator<=(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() <= rhs.value();
}

template <typename T>
inline bool operator>(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() > rhs.value();
}

template <typename T>
inline bool operator>=(const At<T>& lhs, const At<T>& rhs) {
  return lhs.value() >= rhs.value();
}

template <typename T>
At<T> MakeAt(T val) {
  return At<T>{val};
}

template <typename T>
At<T> MakeAt(Location loc, T val) {
  return At<T>{loc, val};
}

}  // namespace wasp

namespace std {

template <typename T>
struct hash<::wasp::At<T>> {
  size_t operator()(const ::wasp::At<T>& v) const {
    return std::hash<T>{}(*v);
  }
};

}

#endif // WASP_BASE_AT_H_

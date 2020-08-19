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

#ifndef WASP_BASE_FORMAT_H_
#define WASP_BASE_FORMAT_H_

#include <iostream>
#include <sstream>
#include <string>

#include "wasp/base/at.h"

namespace wasp {

namespace internal {

template <typename T>
void format_single(std::stringstream& ss, const T& x) {
  ss << x;
}

template <typename T>
void format_single(std::stringstream& ss, const At<T>& x) {
  format_single(ss, x.value());
}

// Print unsigned/signed char as a number instead of a character.

template <>
inline void format_single(std::stringstream& ss, const char& x) {
  ss.operator<<(x);
}

template <>
inline void format_single(std::stringstream& ss, const unsigned char& x) {
  ss.operator<<(x);
}

template <>
inline void format_single(std::stringstream& ss, const signed char& x) {
  ss.operator<<(x);
}

}  // namespace internal

template <typename... Args>
std::string format(Args&&... args) {
  std::stringstream ss;
  (internal::format_single(ss, args), ...);
  return ss.str();
}

}  // namespace wasp

#endif  // WASP_BASE_FORMAT_H_

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

#ifndef WASP_BASE_ABSL_HASH_VALUE_MACROS_H_
#define WASP_BASE_ABSL_HASH_VALUE_MACROS_H_

#define WASP_ABSL_HASH_VALUE_VARGS(Name, Count, ...) \
  WASP_ABSL_HASH_VALUE_##Count(Name, __VA_ARGS__)

#define WASP_ABSL_HASH_VALUE_0(Name, ...)     \
  template <typename H>                       \
  H AbslHashValue(H h, const ::wasp::Name&) { \
    return h;                                 \
  }

#define WASP_ABSL_HASH_VALUE_1(Name, f1)        \
  template <typename H>                         \
  H AbslHashValue(H h, const ::wasp::Name& v) { \
    return H::combine(std::move(h), v.f1);      \
  }

#define WASP_ABSL_HASH_VALUE_2(Name, f1, f2)     \
  template <typename H>                          \
  H AbslHashValue(H h, const ::wasp::Name& v) {  \
    return H::combine(std::move(h), v.f1, v.f2); \
  }

#define WASP_ABSL_HASH_VALUE_3(Name, f1, f2, f3)       \
  template <typename H>                                \
  H AbslHashValue(H h, const ::wasp::Name& v) {        \
    return H::combine(std::move(h), v.f1, v.f2, v.f3); \
  }

#define WASP_ABSL_HASH_VALUE_4(Name, f1, f2, f3, f4)         \
  template <typename H>                                      \
  H AbslHashValue(H h, const ::wasp::Name& v) {              \
    return H::combine(std::move(h), v.f1, v.f2, v.f3, v.f4); \
  }

#define WASP_ABSL_HASH_VALUE_5(Name, f1, f2, f3, f4, f5)           \
  template <typename H>                                            \
  H AbslHashValue(H h, const ::wasp::Name& v) {                    \
    return H::combine(std::move(h), v.f1, v.f2, v.f3, v.f4, v.f5); \
  }

#define WASP_ABSL_HASH_VALUE_CONTAINER(Name)    \
  template <typename H>                         \
  H AbslHashValue(H h, const ::wasp::Name& v) { \
    return H::combine(std::move(h), v);         \
  }

#endif  // WASP_BASE_ABSL_HASH_VALUE_MACROS_H_

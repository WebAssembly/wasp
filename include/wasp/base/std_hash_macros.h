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

#ifndef WASP_BASE_STD_HASH_MACROS_H_
#define WASP_BASE_STD_HASH_MACROS_H_

#define WASP_DECLARE_STD_HASH(Name, ...)          \
  namespace std {                                 \
  template <>                                     \
  struct hash<::wasp::Name> {                     \
    size_t operator()(const ::wasp::Name&) const; \
  };                                              \
  }

#define WASP_STD_HASH_VARGS(Name, Count, ...) \
  WASP_STD_HASH_##Count(Name, __VA_ARGS__)

#define WASP_STD_HASH_0(Name, ...)                                     \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return 0;                                                          \
  }                                                                    \
  }

#define WASP_STD_HASH_1(Name, f1)                                      \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return ::wasp::HashState::combine(0, v.f1);                        \
  }                                                                    \
  }

#define WASP_STD_HASH_2(Name, f1, f2)                                  \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return ::wasp::HashState::combine(0, v.f1, v.f2);                  \
  }                                                                    \
  }

#define WASP_STD_HASH_3(Name, f1, f2, f3)                              \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return ::wasp::HashState::combine(0, v.f1, v.f2, v.f3);            \
  }                                                                    \
  }

#define WASP_STD_HASH_4(Name, f1, f2, f3, f4)                          \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return ::wasp::HashState::combine(0, v.f1, v.f2, v.f3, v.f4);      \
  }                                                                    \
  }

#define WASP_STD_HASH_5(Name, f1, f2, f3, f4, f5)                       \
  namespace std {                                                       \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const {  \
    return ::wasp::HashState::combine(0, v.f1, v.f2, v.f3, v.f4, v.f5); \
  }                                                                     \
  }

#define WASP_STD_HASH_CONTAINER(Name)                                  \
  namespace std {                                                      \
  size_t hash<::wasp::Name>::operator()(const ::wasp::Name& v) const { \
    return ::wasp::HashContainer(v);                                   \
  }                                                                    \
  }

#endif  // WASP_BASE_STD_HASH_MACROS_H_

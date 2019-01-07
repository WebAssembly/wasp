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

#ifndef WASP_BASE_OPERATOR_EQ_NE_MACROS_H_
#define WASP_BASE_OPERATOR_EQ_NE_MACROS_H_

#define WASP_OPERATOR_EQ_NE_0(Name)                                  \
  bool operator==(const Name& lhs, const Name& rhs) { return true; } \
  bool operator!=(const Name& lhs, const Name& rhs) { return false; }

#define WASP_OPERATOR_EQ_NE_1(Name, f1)               \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1;                          \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#define WASP_OPERATOR_EQ_NE_2(Name, f1, f2)           \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2;      \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#define WASP_OPERATOR_EQ_NE_3(Name, f1, f2, f3)                      \
  bool operator==(const Name& lhs, const Name& rhs) {                \
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2 && lhs.f3 == rhs.f3; \
  }                                                                  \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#endif // WASP_BASE_OPERATOR_EQ_NE_MACROS_H_

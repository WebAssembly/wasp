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

#ifndef WASP_BASE_WASM_TYPES_H_
#define WASP_BASE_WASM_TYPES_H_

#include <iosfwd>

#include "wasp/base/at.h"
#include "wasp/base/types.h"

namespace wasp {

enum class Opcode : u32 {
#define WASP_V(prefix, val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
};

enum class ValueType : s32 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class ElementType : s32 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/def/element_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class ExternalKind : u8 {
#define WASP_V(val, Name, str) Name,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/def/external_kind.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class EventAttribute : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/base/def/event_attribute.def"
#undef WASP_V
};

enum class Mutability : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/base/def/mutability.def"
#undef WASP_V
};

enum class SegmentType {
  Active,
  Passive,
  Declared,
};

enum class Shared {
  No,
  Yes,
};

struct Limits {
  explicit Limits(At<u32> min);
  explicit Limits(At<u32> min, OptAt<u32> max);
  explicit Limits(At<u32> min, OptAt<u32> max, At<Shared>);

  At<u32> min;
  OptAt<u32> max;
  At<Shared> shared;
};

#define WASP_BASE_WASM_TYPES(WASP_V) WASP_V(Limits)

#define WASP_DECLARE_OPERATOR_EQ_NE(Type)    \
  bool operator==(const Type&, const Type&); \
  bool operator!=(const Type&, const Type&);

WASP_BASE_WASM_TYPES(WASP_DECLARE_OPERATOR_EQ_NE)

#undef WASP_DECLARE_OPERATOR_EQ_NE

// Used for gtest.

void PrintTo(const Opcode&, std::ostream*);
void PrintTo(const ValueType&, std::ostream*);
void PrintTo(const ElementType&, std::ostream*);
void PrintTo(const ExternalKind&, std::ostream*);
void PrintTo(const EventAttribute&, std::ostream*);
void PrintTo(const Mutability&, std::ostream*);
void PrintTo(const SegmentType&, std::ostream*);
void PrintTo(const Shared&, std::ostream*);

#define WASP_DECLARE_PRINT_TO(Type) void PrintTo(const Type&, std::ostream*);
WASP_BASE_WASM_TYPES(WASP_DECLARE_PRINT_TO)
#undef WASP_DECLARE_PRINT_TO

}  // namespace wasp

namespace std {

#define WASP_DECLARE_STD_HASH(Type)               \
  template <>                                     \
  struct hash<::wasp::Type> {                     \
    size_t operator()(const ::wasp::Type&) const; \
  };

WASP_BASE_WASM_TYPES(WASP_DECLARE_STD_HASH)

#undef WASP_DECLARE_STD_HASH

}  // namespace std

#endif // WASP_BASE_WASM_TYPES_H_

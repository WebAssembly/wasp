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
#include <vector>

#include "wasp/base/absl_hash_value_macros.h"
#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/types.h"

namespace wasp {

enum class Opcode : u32 {
#define WASP_V(prefix, val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/opcode.inc"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
};

enum class PackedType : u8 {
#define WASP_V(val, Name, str) Name = val,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/inc/packed_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class NumericType : u8 {
#define WASP_V(val, Name, str) Name = val,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/inc/numeric_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class ReferenceKind : u8 {
#define WASP_V(val, Name, str) Name = val,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/inc/reference_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class HeapKind : u8 {
#define WASP_V(val, Name, str) Name = val,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/inc/heap_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class ExternalKind : u8 {
#define WASP_V(val, Name, str) Name = val,
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/base/inc/external_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
};

enum class EventAttribute : u8 {
#define WASP_V(val, Name, str) Name = val,
#include "wasp/base/inc/event_attribute.inc"
#undef WASP_V
};

enum class Mutability : u8 {
#define WASP_V(val, Name, str) Name = val,
#include "wasp/base/inc/mutability.inc"
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

enum class Null {
  No,
  Yes,
};

enum class IndexType {
  I32,
  I64,
};

struct Limits {
  explicit Limits(At<u32> min);
  explicit Limits(At<u32> min, OptAt<u32> max);
  explicit Limits(At<u32> min, OptAt<u32> max, At<Shared>);
  explicit Limits(At<u32> min, OptAt<u32> max, At<Shared>, At<IndexType>);

  At<u32> min;
  OptAt<u32> max;
  At<Shared> shared;
  At<IndexType> index_type;
};

struct MemoryType {
  At<Limits> limits;
};

using ShuffleImmediate = std::array<u8, 16>;

#define WASP_BASE_WASM_ENUMS(WASP_V) \
  WASP_V(Opcode)                     \
  WASP_V(PackedType)                 \
  WASP_V(NumericType)                \
  WASP_V(ReferenceKind)              \
  WASP_V(HeapKind)                   \
  WASP_V(ExternalKind)               \
  WASP_V(EventAttribute)             \
  WASP_V(Mutability)                 \
  WASP_V(SegmentType)                \
  WASP_V(Shared)                     \
  WASP_V(Null)                       \
  WASP_V(IndexType)

#define WASP_BASE_WASM_STRUCTS(WASP_V) \
  WASP_V(Limits, 3, min, max, shared)  \
  WASP_V(MemoryType, 1, limits)

#define WASP_BASE_WASM_CONTAINERS(WASP_V) \
  WASP_V(ShuffleImmediate)

WASP_BASE_WASM_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_BASE_WASM_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

WASP_BASE_WASM_STRUCTS(WASP_ABSL_HASH_VALUE_VARGS)
WASP_BASE_WASM_CONTAINERS(WASP_ABSL_HASH_VALUE_CONTAINER)

}  // namespace wasp

#endif // WASP_BASE_WASM_TYPES_H_

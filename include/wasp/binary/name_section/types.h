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

#ifndef WASP_BINARY_NAME_SECTION_TYPES_H_
#define WASP_BINARY_NAME_SECTION_TYPES_H_

#include <vector>

#include "wasp/base/absl_hash_value_macros.h"
#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp::binary {

enum class NameSubsectionId : u8 {
#define WASP_V(val, Name, str) Name = val,
#include "wasp/binary/def/name_subsection_id.def"
#undef WASP_V
};

struct NameSubsection {
  At<NameSubsectionId> id;
  SpanU8 data;
};

struct NameAssoc {
  At<Index> index;
  At<string_view> name;
};

using NameMap = std::vector<At<NameAssoc>>;

struct IndirectNameAssoc {
  At<Index> index;
  NameMap name_map;
};

#define WASP_BINARY_NAME_ENUMS(WASP_V) \
  WASP_V(binary::NameSubsectionId)

#define WASP_BINARY_NAME_STRUCTS(WASP_V)                \
  WASP_V(binary::IndirectNameAssoc, 2, index, name_map) \
  WASP_V(binary::NameAssoc, 2, index, name)             \
  WASP_V(binary::NameSubsection, 2, id, data)

#define WASP_BINARY_NAME_CONTAINERS(WASP_V) \
  WASP_V(binary::NameMap)

WASP_BINARY_NAME_STRUCTS(WASP_DECLARE_OPERATOR_EQ_NE)
WASP_BINARY_NAME_CONTAINERS(WASP_DECLARE_OPERATOR_EQ_NE)

WASP_BINARY_NAME_STRUCTS(WASP_ABSL_HASH_VALUE_VARGS)
WASP_BINARY_NAME_CONTAINERS(WASP_ABSL_HASH_VALUE_CONTAINER)

}  // namespace wasp::binary

#endif // WASP_BINARY_NAME_SECTION_TYPES_H_

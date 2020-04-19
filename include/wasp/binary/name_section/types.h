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

#include <functional>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/operator_eq_ne_macros.h"
#include "wasp/base/print_to_macros.h"
#include "wasp/base/span.h"
#include "wasp/base/std_hash_macros.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp {
namespace binary {

enum class NameSubsectionId : u8 {
#define WASP_V(val, Name, str) Name,
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

// Used for gtest.

WASP_BINARY_NAME_ENUMS(WASP_DECLARE_PRINT_TO)
WASP_BINARY_NAME_STRUCTS(WASP_DECLARE_PRINT_TO)

}  // namespace binary
}  // namespace wasp

WASP_BINARY_NAME_STRUCTS(WASP_DECLARE_STD_HASH)
WASP_BINARY_NAME_CONTAINERS(WASP_DECLARE_STD_HASH)

#endif // WASP_BINARY_NAME_SECTION_TYPES_H_

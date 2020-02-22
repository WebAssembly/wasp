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

#ifndef WASP_BINARY_TYPES_NAME_H_
#define WASP_BINARY_TYPES_NAME_H_

#include <functional>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"

namespace wasp {
namespace binary {

enum class NameSubsectionId : u8 {
#define WASP_V(val, Name, str) Name,
#include "wasp/binary/name_subsection_id.def"
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

#define WASP_TYPES(WASP_V)  \
  WASP_V(IndirectNameAssoc) \
  WASP_V(NameAssoc)         \
  WASP_V(NameSubsection)

#define WASP_DECLARE_OPERATOR_EQ_NE(Type)    \
  bool operator==(const Type&, const Type&); \
  bool operator!=(const Type&, const Type&);

WASP_TYPES(WASP_DECLARE_OPERATOR_EQ_NE)

#undef WASP_DECLARE_OPERATOR_EQ_NE

}  // namespace binary
}  // namespace wasp

namespace std {

#define WASP_DECLARE_STD_HASH(Type)                       \
  template <>                                             \
  struct hash<::wasp::binary::Type> {                     \
    size_t operator()(const ::wasp::binary::Type&) const; \
  };

WASP_TYPES(WASP_DECLARE_STD_HASH)

#undef WASP_DECLARE_STD_HASH
#undef WASP_TYPES

}  // namespace std

#endif // WASP_BINARY_TYPES_NAME_H_

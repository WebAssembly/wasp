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

#include "wasp/binary/name_section/types.h"

#include "src/base/operator_eq_ne_macros.h"
#include "src/base/std_hash_macros.h"

namespace wasp {
namespace binary {

WASP_OPERATOR_EQ_NE_2(IndirectNameAssoc, index, name_map)
WASP_OPERATOR_EQ_NE_2(NameAssoc, index, name)
WASP_OPERATOR_EQ_NE_2(NameSubsection, id, data)

}  // namespace binary
}  // namespace wasp

WASP_STD_HASH_2(::wasp::binary::NameAssoc, index, name)
WASP_STD_HASH_2(::wasp::binary::NameSubsection, id, data)

namespace std {

size_t hash<::wasp::binary::IndirectNameAssoc>::operator()(
    const ::wasp::binary::IndirectNameAssoc& v) const {
  return ::wasp::HashState::combine(0, v.index,
                                    ::wasp::HashContainer(v.name_map));
}

}  // namespace std

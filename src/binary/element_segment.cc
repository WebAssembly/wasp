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

#include "wasp/binary/element_segment.h"

#include "src/base/operator_eq_ne_macros.h"

namespace wasp {
namespace binary {

ElementSegment::ElementSegment(Index table_index,
                               ConstantExpression offset,
                               const std::vector<Index>& init)
    : init{init}, desc{Active{table_index, offset}} {}

ElementSegment::ElementSegment(ElementType element_type,
                               const std::vector<Index>& init)
    : init{init}, desc{Passive{element_type}} {}

WASP_OPERATOR_EQ_NE_2(ElementSegment, init, desc)
WASP_OPERATOR_EQ_NE_2(ElementSegment::Active, table_index, offset)
WASP_OPERATOR_EQ_NE_1(ElementSegment::Passive, element_type)

}  // namespace binary
}  // namespace wasp

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

#include "wasp/binary/data_segment.h"

#include "src/base/operator_eq_ne_macros.h"
#include "src/base/std_hash_macros.h"

namespace wasp {
namespace binary {

DataSegment::DataSegment(Index memory_index,
                         ConstantExpression offset,
                         SpanU8 init)
    : init{init}, desc{Active{memory_index, offset}} {}

DataSegment::DataSegment(SpanU8 init) : init{init}, desc{Passive{}} {}

WASP_OPERATOR_EQ_NE_2(DataSegment, init, desc)
WASP_OPERATOR_EQ_NE_2(DataSegment::Active, memory_index, offset)
WASP_OPERATOR_EQ_NE_0(DataSegment::Passive)

}  // namespace binary
}  // namespace wasp

WASP_STD_HASH_2(::wasp::binary::DataSegment, init, desc)
WASP_STD_HASH_2(::wasp::binary::DataSegment::Active, memory_index, offset)
WASP_STD_HASH_0(::wasp::binary::DataSegment::Passive)

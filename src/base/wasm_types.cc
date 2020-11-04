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

#include "wasp/base/wasm_types.h"

#include "wasp/base/hash.h"
#include "wasp/base/operator_eq_ne_macros.h"

namespace wasp {

Limits::Limits(At<u32> min)
    : min{min}, shared{Shared::No}, index_type{IndexType::I32} {}

Limits::Limits(At<u32> min, OptAt<u32> max)
    : min{min}, max{max}, shared{Shared::No}, index_type{IndexType::I32} {}

Limits::Limits(At<u32> min, OptAt<u32> max, At<Shared> shared)
    : min{min}, max{max}, shared{shared}, index_type{IndexType::I32} {}

Limits::Limits(At<u32> min,
               OptAt<u32> max,
               At<Shared> shared,
               At<IndexType> index_type)
    : min{min}, max{max}, shared{shared}, index_type{index_type} {}

WASP_BASE_WASM_STRUCTS(WASP_OPERATOR_EQ_NE_VARGS)
WASP_BASE_WASM_CONTAINERS(WASP_OPERATOR_EQ_NE_CONTAINER)

}  // namespace wasp

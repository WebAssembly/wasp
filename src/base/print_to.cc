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

#include <iostream>

#include "wasp/base/formatters.h"
#include "wasp/base/wasm_types.h"

namespace wasp {

#define WASP_DEFINE_PRINT_TO(Type)                    \
  void PrintTo(const Type& value, std::ostream* os) { \
    *os << format("{}", value);                       \
  }

WASP_DEFINE_PRINT_TO(Opcode)
WASP_DEFINE_PRINT_TO(ValueType)
WASP_DEFINE_PRINT_TO(ElementType)
WASP_DEFINE_PRINT_TO(ExternalKind)
WASP_DEFINE_PRINT_TO(EventAttribute)
WASP_DEFINE_PRINT_TO(Mutability)
WASP_DEFINE_PRINT_TO(SegmentType)
WASP_DEFINE_PRINT_TO(Shared)

WASP_BASE_WASM_TYPES(WASP_DEFINE_PRINT_TO)

#undef WASP_DEFINE_PRINT_TO

}  // namespace wasp

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

#include "wasp/binary/formatters.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {

#define WASP_DEFINE_PRINT_TO(Type)                    \
  void PrintTo(const Type& value, std::ostream* os) { \
    *os << format("{}", value);                       \
  }

WASP_DEFINE_PRINT_TO(BlockType)
WASP_DEFINE_PRINT_TO(SectionId)

WASP_BINARY_TYPES(WASP_DEFINE_PRINT_TO)

#undef WASP_DEFINE_PRINT_TO

}  // namespace binary
}  // namespace wasp

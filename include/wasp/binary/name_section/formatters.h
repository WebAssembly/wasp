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

#ifndef WASP_BINARY_NAME_SECTION_FORMATTERS_H_
#define WASP_BINARY_NAME_SECTION_FORMATTERS_H_

#include "wasp/base/formatters.h"
#include "wasp/base/formatter_macros.h"
#include "wasp/binary/name_section/types.h"

namespace wasp::binary {

using wasp::operator<<;

WASP_BINARY_NAME_ENUMS(WASP_DECLARE_FORMATTER)
WASP_BINARY_NAME_STRUCTS(WASP_DECLARE_FORMATTER)

}  // namespace wasp::binary

#endif  // WASP_BINARY_NAME_SECTION_FORMATTERS_H_

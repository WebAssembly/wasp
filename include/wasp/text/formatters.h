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

#ifndef WASP_TEXT_FORMATTERS_H_
#define WASP_TEXT_FORMATTERS_H_

#include "wasp/base/format.h"
#include "wasp/base/formatter_macros.h"
#include "wasp/text/types.h"

namespace fmt {

WASP_TEXT_ENUMS(WASP_DECLARE_FORMATTER)
WASP_TEXT_STRUCTS(WASP_DECLARE_FORMATTER)

// Special cases.
WASP_DECLARE_FORMATTER(text::Var)
WASP_DECLARE_FORMATTER(text::ModuleItem)
WASP_DECLARE_FORMATTER(text::Const)

}  // namespace fmt

#include "wasp/text/formatters-inl.h"

#endif  // WASP_TEXT_FORMATTERS_H_

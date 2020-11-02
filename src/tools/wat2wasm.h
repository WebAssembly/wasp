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

#ifndef WASP_TOOLS_WAT2WASM_H_
#define WASP_TOOLS_WAT2WASM_H_

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

namespace wasp::tools::wat2wasm {

int Main(span<const string_view> args);

}  // namespace wasp::tools::wat2wasm

#endif  // WASP_TOOLS_WAT2WASM_H_

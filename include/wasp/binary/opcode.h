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

#ifndef WASP_BINARY_OPCODE_H_
#define WASP_BINARY_OPCODE_H_

#include "wasp/base/types.h"

namespace wasp {
namespace binary {

enum class Opcode : u32 {
#define WASP_V(prefix, val, Name, str, ...) Name,
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
};

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_OPCODE_H_

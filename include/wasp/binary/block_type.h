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

#ifndef WASP_BINARY_BLOCK_TYPE_H_
#define WASP_BINARY_BLOCK_TYPE_H_

#include "wasp/base/types.h"

namespace wasp {
namespace binary {

// BlockType values are 0x40, and 0x7c through 0x7f in the MVP. In the
// multi-value proposal, a block type is extended to a s32 value, where
// negative values represent the standard value types, and non-negative values
// are indexes into the type section.
//
// The values 0x40, 0x7c..0x7f are all representations of small negative
// numbers encoded as signed LEB128. For example, 0x40 is the encoding for -64.
// Signed LEB128 values have their sign bit as the 6th bit (instead of the 7th
// bit), so to convert them to a s32 value, we must shift by 25.
constexpr s32 ConvertValueTypeToBlockType(u8 value) {
  return (value << 25) >> 25;
}

enum class BlockType : s32 {
#define WASP_V(val, Name, str) Name = ConvertValueTypeToBlockType(val),
#define WASP_FEATURE_V(val, Name, str, feature) WASP_V(val, Name, str)
#include "wasp/binary/block_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
};

static_assert(s32(BlockType::I32) == -1, "Invalid value for BlockType::I32");
static_assert(s32(BlockType::I64) == -2, "Invalid value for BlockType::I64");
static_assert(s32(BlockType::F32) == -3, "Invalid value for BlockType::F32");
static_assert(s32(BlockType::F64) == -4, "Invalid value for BlockType::F64");
static_assert(s32(BlockType::Void) == -64, "Invalid value for BlockType::Void");

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_BLOCK_TYPE_H_

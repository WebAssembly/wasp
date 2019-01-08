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

#ifndef WASP_BINARY_BLOCK_TYPE_ENCODING_H
#define WASP_BINARY_BLOCK_TYPE_ENCODING_H

#include "wasp/base/types.h"
#include "wasp/base/optional.h"
#include "wasp/binary/block_type.h"

namespace wasp {
namespace binary {
namespace encoding {

struct BlockType {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
#include "wasp/binary/block_type.def"
#undef WASP_V

  static optional<::wasp::binary::BlockType> Decode(u8);
};

// static
inline optional<::wasp::binary::BlockType> BlockType::Decode(u8 val) {
  switch (val) {
#define WASP_V(val, Name, str) \
  case Name:                   \
    return ::wasp::binary::BlockType::Name;
#include "wasp/binary/block_type.def"
#undef WASP_V
    default:
      return nullopt;
  }
}

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_BLOCK_TYPE_ENCODING_H

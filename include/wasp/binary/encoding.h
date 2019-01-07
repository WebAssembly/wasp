//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_ENCODING_H
#define WASP_BINARY_ENCODING_H

#include "wasp/base/types.h"
#include "wasp/binary/defs.h"
#include "wasp/binary/instruction.h"
#include "wasp/binary/name_section.h"
#include "wasp/binary/section.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {
namespace encoding {

constexpr u8 Magic[] = {0, 'a', 's', 'm'};
constexpr u8 Version[] = {1, 0, 0, 0};

struct Type {
  static constexpr u8 Function = 0x60;
};

struct ValueType {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_VALUE_TYPE(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::ValueType> Decode(u8);
};

struct BlockType {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_BLOCK_TYPE(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::BlockType> Decode(u8);
};

struct ElementType {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_ELEMENT_TYPE(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::ElementType> Decode(u8);
};

struct ExternalKind {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_EXTERNAL_KIND(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::ExternalKind> Decode(u8);
};

struct Mutability {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_MUTABILITY(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::Mutability> Decode(u8);
};

struct Section {
#define WASP_V(val, Name, str) static constexpr u32 Name = val;
  WASP_FOREACH_SECTION(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::SectionId> Decode(u32);
};

struct Opcode {
#define WASP_V(prefix, val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_OPCODE(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::Opcode> Decode(u8);
};

struct Limits {
  static constexpr u8 Flags_NoMax = 0;
  static constexpr u8 Flags_HasMax = 1;
};

struct NameSubsectionId {
#define WASP_V(val, Name, str) static constexpr u8 Name = val;
  WASP_FOREACH_NAME_SUBSECTION_ID(WASP_V)
#undef WASP_V

  static optional<::wasp::binary::NameSubsectionId> Decode(u8);
};

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#include "wasp/binary/encoding-inl.h"

#endif // WASP_BINARY_ENCODING_H

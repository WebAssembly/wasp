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

#include "wasp/base/formatters.h"

#include <iomanip>
#include <iostream>

namespace wasp {

std::ostream& operator<<(std::ostream& os, const ::std::nullopt_t& self) {
  return os << "none";
}

template <>
std::ostream& operator<<(std::ostream& os, ::wasp::SpanU8 self) {
  os << "\"" << std::hex << std::setfill('0');
  for (auto x : self) {
    os << "\\" << std::setw(2) << static_cast<int>(x);
  }
  os << std::dec << "\"";
  return os;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::v128& self) {
  string_view space = "";
  os << std::hex;
  for (const auto& x : self.as<::wasp::u32x4>()) {
    os << space << "0x" << x;
    space = " ";
  }
  os << std::dec;
  return os;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Opcode& self) {
  string_view result;
  switch (self) {
#define WASP_V(prefix, val, Name, str, ...) \
  case ::wasp::Opcode::Name:                \
    result = str;                           \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/opcode.inc"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default: {
      // Special case for opcodes with unknown ids.
      os << "<unknown:" << static_cast<::wasp::u32>(self) << ">";
      return os;
    }
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::PackedType& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::PackedType::Name:    \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/packed_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::NumericType& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::NumericType::Name:     \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/numeric_type.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::ReferenceKind& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ReferenceKind::Name: \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/reference_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::HeapKind& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::HeapKind::Name:      \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/heap_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::ExternalKind& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ExternalKind::Name:  \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/inc/external_kind.inc"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::TagAttribute& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)       \
  case ::wasp::TagAttribute::Name: \
    result = str;                    \
    break;
#include "wasp/base/inc/tag_attribute.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Mutability& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)   \
  case ::wasp::Mutability::Name: \
    result = str;                \
    break;
#include "wasp/base/inc/mutability.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::SegmentType& self) {
  string_view result;
  switch (self) {
    case ::wasp::SegmentType::Active:
      result = "active";
      break;
    case ::wasp::SegmentType::Passive:
      result = "passive";
      break;
    case ::wasp::SegmentType::Declared:
      result = "declared";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Features& self) {
  string_view separator;
#define WASP_V(enum_, variable, flag, default_) \
  if (self.variable##_enabled()) {              \
    os << #variable << separator;               \
    separator = "|";                            \
  }
#include "wasp/base/features.inc"
#undef WASP_V

  if (separator.size() == 0) {
    os << "none";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::IndexType& self) {
  string_view result;
  switch (self) {
    case ::wasp::IndexType::I32:
      result = "i32";
      break;
    case ::wasp::IndexType::I64:
      result = "i64";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Null& self) {
  string_view result;
  switch (self) {
    case ::wasp::Null::No:
      result = "no";
      break;
    case ::wasp::Null::Yes:
      result = "yes";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Shared& self) {
  string_view result;
  switch (self) {
    case ::wasp::Shared::No:
      result = "unshared";
      break;
    case ::wasp::Shared::Yes:
      result = "shared";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::Limits& self) {
  os << "{min " << self.min;
  if (self.max) {
    os << ", max " << *self.max;
    if (self.shared == ::wasp::Shared::Yes) {
      os << ", " << self.shared;
    }
  }
  if (self.index_type == ::wasp::IndexType::I64) {
    os << ", " << self.index_type;
  }
  return os << "}";
}

std::ostream& operator<<(std::ostream& os, const ::wasp::MemoryType& self) {
  return os << self.limits;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::monostate& self) {
  return os;
}

std::ostream& operator<<(std::ostream& os, const ShuffleImmediate& self) {
  string_view space = "";
  os << "[";
  for (const auto& x : self) {
    os << space << static_cast<int>(x);
    space = " ";
  }
  return os << "]";
}

}  // namespace wasp

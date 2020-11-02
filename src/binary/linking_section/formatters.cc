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

#include "wasp/binary/linking_section/formatters.h"

#include <iostream>

#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace wasp::binary {

std::ostream& operator<<(std::ostream& os, const ::wasp::binary::Comdat& self) {
  return os << "{name " << self.name << ", flags " << self.flags << ", symbols "
            << self.symbols << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::ComdatSymbol& self) {
  return os << "{kind " << self.kind << ", index " << self.index << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::ComdatSymbolKind& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)                 \
  case ::wasp::binary::ComdatSymbolKind::Name: \
    result = str;                              \
    break;
#include "wasp/binary/inc/comdat_symbol_kind.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::LinkingSubsection& self) {
  return os << self.id << " " << self.data;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::LinkingSubsectionId& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)                    \
  case ::wasp::binary::LinkingSubsectionId::Name: \
    result = str;                                 \
    break;
#include "wasp/binary/inc/linking_subsection_id.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::RelocationEntry& self) {
  os << "{type " << self.type << ", offset " << self.offset << ", index "
     << self.index;
  if (self.addend) {
    os << ", addend " << *self.addend;
  }
  return os << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::InitFunction& self) {
  return os << "{priority " << self.priority << ", index " << self.index << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::RelocationType& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)               \
  case ::wasp::binary::RelocationType::Name: \
    result = str;                            \
    break;
#include "wasp/binary/inc/relocation_type.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SegmentInfo& self) {
  return os << "{name " << self.name << ", align " << self.align_log2
            << ", flags " << self.flags << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfo& self) {
  os << "{" << self.flags->binding << " " << self.flags->visibility << " "
     << self.flags->undefined << " " << self.flags->explicit_name;

  if (self.is_base()) {
    const auto& base = self.base();
    os << ", kind " << base.kind << ", index " << base.index;
    if (base.name) {
      os << ", name " << *base.name;
    }
  } else if (self.is_data()) {
    const auto& data = self.data();
    os << ", name " << data.name;
    if (data.defined) {
      os << ", index " << data.defined->index << ", offset "
         << data.defined->offset << ", size " << data.defined->size;
    }
  } else if (self.is_section()) {
    const auto& section = self.section();
    os << ", section " << section.section;
  }

  return os << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfo::Base& self) {
  os << "{kind " << self.kind << ", index " << self.index;
  if (self.name) {
    os << ", name " << *self.name;
  }
  return os << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfo::Data& self) {
  os << "{name " << self.name;
  if (self.defined) {
    os << ", index " << self.defined->index << ", offset "
       << self.defined->offset << ", size " << self.defined->size;
  }
  return os << "}";
}

std::ostream& operator<<(
    std::ostream& os,
    const ::wasp::binary::SymbolInfo::Data::Defined& self) {
  return os << "{index " << self.index << ", offset " << self.offset
            << ", size " << self.size << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfo::Flags& self) {
  return os << "{" << self.binding << " " << self.visibility << " "
            << self.undefined << " " << self.explicit_name << "}";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfo::Section& self) {
  return os << "{section " << self.section << "}";
}

std::ostream& operator<<(
    std::ostream& os,
    const ::wasp::binary::SymbolInfo::Flags::Binding& self) {
  string_view result;
  switch (self) {
    case ::wasp::binary::SymbolInfo::Flags::Binding::Global:
      result = "global";
      break;
    case ::wasp::binary::SymbolInfo::Flags::Binding::Weak:
      result = "weak";
      break;
    case ::wasp::binary::SymbolInfo::Flags::Binding::Local:
      result = "local";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(
    std::ostream& os,
    const ::wasp::binary::SymbolInfo::Flags::Visibility& self) {
  string_view result;
  switch (self) {
    case ::wasp::binary::SymbolInfo::Flags::Visibility::Default:
      result = "default";
      break;
    case ::wasp::binary::SymbolInfo::Flags::Visibility::Hidden:
      result = "hidden";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(
    std::ostream& os,
    const ::wasp::binary::SymbolInfo::Flags::Undefined& self) {
  string_view result;
  switch (self) {
    case ::wasp::binary::SymbolInfo::Flags::Undefined::No:
      result = "defined";
      break;
    case ::wasp::binary::SymbolInfo::Flags::Undefined::Yes:
      result = "undefined";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(
    std::ostream& os,
    const ::wasp::binary::SymbolInfo::Flags::ExplicitName& self) {
  string_view result;
  switch (self) {
    case ::wasp::binary::SymbolInfo::Flags::ExplicitName::No:
      result = "import name";
      break;
    case ::wasp::binary::SymbolInfo::Flags::ExplicitName::Yes:
      result = "explicit name";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::binary::SymbolInfoKind& self) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)               \
  case ::wasp::binary::SymbolInfoKind::Name: \
    result = str;                            \
    break;
#include "wasp/binary/inc/symbol_info_kind.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

}  // namespace wasp::binary

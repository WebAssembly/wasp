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

#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Comdat>::format(
    const ::wasp::binary::Comdat& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{name {}, flags {}, symbols {}}}", self.name, self.flags,
            self.symbols);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ComdatSymbol>::format(
    const ::wasp::binary::ComdatSymbol& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{kind {}, index {}}}", self.kind, self.index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ComdatSymbolKind>::format(
    const ::wasp::binary::ComdatSymbolKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)                 \
  case ::wasp::binary::ComdatSymbolKind::Name: \
    result = str;                              \
    break;
#include "wasp/binary/def/comdat_symbol_kind.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::LinkingSubsection>::format(
    const ::wasp::binary::LinkingSubsection& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.id, self.data);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::LinkingSubsectionId>::format(
    const ::wasp::binary::LinkingSubsectionId& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)                    \
  case ::wasp::binary::LinkingSubsectionId::Name: \
    result = str;                                 \
    break;
#include "wasp/binary/def/linking_subsection_id.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::RelocationEntry>::format(
    const ::wasp::binary::RelocationEntry& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}, offset {}, index {}", self.type, self.offset,
            self.index);
  if (self.addend) {
    format_to(buf, ", addend {}", *self.addend);
  }
  format_to(buf, "}}");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::InitFunction>::format(
    const ::wasp::binary::InitFunction& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{priority {}, index {}}}", self.priority, self.index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::RelocationType>::format(
    const ::wasp::binary::RelocationType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)               \
  case ::wasp::binary::RelocationType::Name: \
    result = str;                            \
    break;
#include "wasp/binary/def/relocation_type.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::SegmentInfo>::format(
    const ::wasp::binary::SegmentInfo& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{name {}, align {}, flags {}}}", self.name, self.align_log2,
            self.flags);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::SymbolInfo>::format(
    const ::wasp::binary::SymbolInfo& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{{} {} {} {}", self.flags->binding, self.flags->visibility,
            self.flags->undefined, self.flags->explicit_name);

  if (self.is_base()) {
    const auto& base = self.base();
    format_to(buf, ", kind {}, index {}", base.kind, base.index);
    if (base.name) {
      format_to(buf, ", name {}", *base.name);
    }
  } else if (self.is_data()) {
    const auto& data = self.data();
    format_to(buf, ", name {}", data.name);
    if (data.defined) {
      format_to(buf, ", index {}, offset {}, size {}", data.defined->index,
                data.defined->offset, data.defined->size);
    }
  } else if (self.is_section()) {
    const auto& section = self.section();
    format_to(buf, ", section {}", section.section);
  }

  format_to(buf, "}}");

  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::SymbolInfo::Flags::Binding>::format(
    const ::wasp::binary::SymbolInfo::Flags::Binding& self,
    Ctx& ctx) {
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
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::SymbolInfo::Flags::Visibility>::format(
    const ::wasp::binary::SymbolInfo::Flags::Visibility& self,
    Ctx& ctx) {
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
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::SymbolInfo::Flags::Undefined>::format(
    const ::wasp::binary::SymbolInfo::Flags::Undefined& self,
    Ctx& ctx) {
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
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::SymbolInfo::Flags::ExplicitName>::format(
    const ::wasp::binary::SymbolInfo::Flags::ExplicitName& self,
    Ctx& ctx) {
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
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::SymbolInfoKind>::format(
    const ::wasp::binary::SymbolInfoKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)               \
  case ::wasp::binary::SymbolInfoKind::Name: \
    result = str;                            \
    break;
#include "wasp/binary/def/symbol_info_kind.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

}  // namespace fmt

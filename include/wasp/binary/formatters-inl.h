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

#include "wasp/binary/formatters.h"

#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::BlockType>::format(
    const ::wasp::binary::BlockType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...)     \
  case ::wasp::binary::BlockType::Name: \
    result = "[" str "]";               \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/def/block_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      // Block types that are indexes in the type section.
      memory_buffer buf;
      format_to(buf, "type[{}]", static_cast<::wasp::s32>(self));
      return formatter<string_view>::format(to_string_view(buf), ctx);
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::SectionId>::format(
    const ::wasp::binary::SectionId& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...)     \
  case ::wasp::binary::SectionId::Name: \
    result = str;                       \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/binary/def/section_id.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default: {
      // Special case for sections with unknown ids.
      memory_buffer buf;
      format_to(buf, "{}", static_cast<::wasp::u32>(self));
      return formatter<string_view>::format(to_string_view(buf), ctx);
    }
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::MemArgImmediate>::format(
    const ::wasp::binary::MemArgImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{align {}, offset {}}}", self.align_log2, self.offset);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Locals>::format(
    const ::wasp::binary::Locals& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} ** {}", self.type, self.count);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Section>::format(
    const ::wasp::binary::Section& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_known()) {
    format_to(buf, "{}", self.known());
  } else if (self.is_custom()) {
    format_to(buf, "{}", self.custom());
  } else {
    WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::KnownSection>::format(
    const ::wasp::binary::KnownSection& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{id {}, contents {}}}", self.id, self.data);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::CustomSection>::format(
    const ::wasp::binary::CustomSection& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{name \"{}\", contents {}}}", self.name, self.data);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::TypeEntry>::format(
    const ::wasp::binary::TypeEntry& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.type);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::FunctionType>::format(
    const ::wasp::binary::FunctionType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} -> {}", self.param_types, self.result_types);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::TableType>::format(
    const ::wasp::binary::TableType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.limits, self.elemtype);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::MemoryType>::format(
    const ::wasp::binary::MemoryType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.limits);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::GlobalType>::format(
    const ::wasp::binary::GlobalType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.mut, self.valtype);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::EventType>::format(
    const ::wasp::binary::EventType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.attribute, self.type_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Import>::format(
    const ::wasp::binary::Import& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{module \"{}\", name \"{}\", desc {}", self.module,
            self.name, self.kind());

  switch (self.kind()) {
    case ::wasp::ExternalKind::Function:
      format_to(buf, " {}}}", self.index());
      break;

    case ::wasp::ExternalKind::Table:
      format_to(buf, " {}}}", self.table_type());
      break;

    case ::wasp::ExternalKind::Memory:
      format_to(buf, " {}}}", self.memory_type());
      break;

    case ::wasp::ExternalKind::Global:
      format_to(buf, " {}}}", self.global_type());
      break;

    case ::wasp::ExternalKind::Event:
      format_to(buf, " {}}}", self.event_type());
      break;

    default:
      break;
  }

  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Export>::format(
    const ::wasp::binary::Export& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{name \"{}\", desc {} {}}}", self.name, self.kind,
            self.index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Expression>::format(
    const ::wasp::binary::Expression& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.data);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ConstantExpression>::format(
    const ::wasp::binary::ConstantExpression& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} end", self.instruction);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ElementExpression>::format(
    const ::wasp::binary::ElementExpression& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} end", self.instruction);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::CallIndirectImmediate>::format(
    const ::wasp::binary::CallIndirectImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.index, self.table_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::BrTableImmediate>::format(
    const ::wasp::binary::BrTableImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.targets, self.default_target);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::BrOnExnImmediate>::format(
    const ::wasp::binary::BrOnExnImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.target, self.event_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::InitImmediate>::format(
    const ::wasp::binary::InitImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.segment_index, self.dst_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::CopyImmediate>::format(
    const ::wasp::binary::CopyImmediate& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.dst_index, self.src_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Instruction>::format(
    const ::wasp::binary::Instruction& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.opcode);

  switch (self.immediate.index()) {
    case 0: /* Nothing. */ break;
    case 1: format_to(buf, " {}", self.reference_type_immediate()); break;
    case 2: format_to(buf, " {}", self.block_type_immediate()); break;
    case 3: format_to(buf, " {}", self.index_immediate()); break;
    case 4: format_to(buf, " {}", self.call_indirect_immediate()); break;
    case 5: format_to(buf, " {}", self.br_table_immediate()); break;
    case 6: format_to(buf, " {}", self.br_on_exn_immediate()); break;
    case 7: format_to(buf, " {}", self.u8_immediate()); break;
    case 8: format_to(buf, " {}", self.mem_arg_immediate()); break;
    case 9: format_to(buf, " {}", self.s32_immediate()); break;
    case 10: format_to(buf, " {}", self.s64_immediate()); break;
    case 11: format_to(buf, " {:f}", self.f32_immediate()); break;
    case 12: format_to(buf, " {:f}", self.f64_immediate()); break;
    case 13: format_to(buf, " {}", self.v128_immediate()); break;
    case 14: format_to(buf, " {}", self.init_immediate()); break;
    case 15: format_to(buf, " {}", self.copy_immediate()); break;
    case 16: format_to(buf, " {}", self.shuffle_immediate()); break;
    case 17: format_to(buf, " {}", self.value_types_immediate()); break;
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Function>::format(
    const ::wasp::binary::Function& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}}}", self.type_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Table>::format(
    const ::wasp::binary::Table& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}}}", self.table_type);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Memory>::format(
    const ::wasp::binary::Memory& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}}}", self.memory_type);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Global>::format(
    const ::wasp::binary::Global& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}, init {}}}", self.global_type, self.init);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Start>::format(
    const ::wasp::binary::Start& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{func {}}}", self.func_index);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ElementSegment>::format(
    const ::wasp::binary::ElementSegment& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.has_indexes()) {
    format_to(buf, "{{type {}, init {}, ", self.indexes().kind,
              self.indexes().init);
  } else if (self.has_expressions()) {
    format_to(buf, "{{type {}, init {}, ", self.expressions().elemtype,
              self.expressions().init);
  }

  switch (self.type) {
    case ::wasp::SegmentType::Active:
      format_to(buf, "mode active {{table {}, offset {}}}}}", *self.table_index,
                *self.offset);
      break;

    case ::wasp::SegmentType::Passive:
      format_to(buf, "mode passive}}");
      break;

    case ::wasp::SegmentType::Declared:
      format_to(buf, "mode declared}}");
      break;
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::ElementSegment::IndexesInit>::format(
    const ::wasp::binary::ElementSegment::IndexesInit& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}, init {}}}", self.kind, self.init);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::ElementSegment::ExpressionsInit>::format(
    const ::wasp::binary::ElementSegment::ExpressionsInit& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}, init {}}}", self.elemtype, self.init);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Code>::format(
    const ::wasp::binary::Code& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{locals {}, body {}}}", self.locals, self.body);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::DataSegment>::format(
    const ::wasp::binary::DataSegment& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{init {}, ", self.init);
  switch (self.type) {
    case ::wasp::SegmentType::Active:
      format_to(buf, "mode active {{memory {}, offset {}}}}}",
                *self.memory_index, *self.offset);
      break;

    case ::wasp::SegmentType::Passive:
      format_to(buf, "mode passive}}");
      break;

    case ::wasp::SegmentType::Declared:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::DataCount>::format(
    const ::wasp::binary::DataCount& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{count {}}}", self.count);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Event>::format(
    const ::wasp::binary::Event& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{{type {}}}", self.event_type);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

}  // namespace fmt

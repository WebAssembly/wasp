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

#include "src/binary/formatters.h"

#include "src/base/macros.h"
#include "src/base/formatters.h"

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ValType>::format(
    ::wasp::binary::ValType self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::binary::ValType::I32: result = "i32"; break;
    case ::wasp::binary::ValType::I64: result = "i64"; break;
    case ::wasp::binary::ValType::F32: result = "f32"; break;
    case ::wasp::binary::ValType::F64: result = "f64"; break;
    case ::wasp::binary::ValType::Anyfunc: result = "anyfunc"; break;
    case ::wasp::binary::ValType::Func: result = "func"; break;
    case ::wasp::binary::ValType::Void: result = "void"; break;
    default: WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::ExternalKind>::format(
    ::wasp::binary::ExternalKind self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::binary::ExternalKind::Func: result = "func"; break;
    case ::wasp::binary::ExternalKind::Table: result = "table"; break;
    case ::wasp::binary::ExternalKind::Memory: result = "memory"; break;
    case ::wasp::binary::ExternalKind::Global: result = "global"; break;
    default: WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Mutability>::format(
    ::wasp::binary::Mutability self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::binary::Mutability::Const: result = "const"; break;
    case ::wasp::binary::Mutability::Var: result = "var"; break;
    default: WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::MemArg>::format(
    const ::wasp::binary::MemArg& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{align {}, offset {}}}", self.align_log2,
                   self.offset);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Limits>::format(
    const ::wasp::binary::Limits& self,
    Ctx& ctx) {
  if (self.max) {
    return format_to(ctx.begin(), "{{min {}, max {}}}", self.min, *self.max);
  } else {
    return format_to(ctx.begin(), "{{min {}}}", self.min);
  }
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::LocalDecl>::format(
    const ::wasp::binary::LocalDecl& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} ** {}", self.type, self.count);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Section<Traits>>::format(
    const ::wasp::binary::Section<Traits>& self,
    Ctx& ctx) {
  if (self.is_known()) {
    return format_to(ctx.begin(), "{}", self.known());
  } else if (self.is_custom()) {
    return format_to(ctx.begin(), "{}", self.custom());
  } else {
    WASP_UNREACHABLE();
  }
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::KnownSection<Traits>>::format(
    const ::wasp::binary::KnownSection<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{id {}, contents {}}}", self.id, self.data);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::CustomSection<Traits>>::format(
    const ::wasp::binary::CustomSection<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{name \"{}\", contents {}}}", self.name,
                   self.data);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::TypeEntry>::format(
    const ::wasp::binary::TypeEntry& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} {}", self.form, self.type);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::FuncType>::format(
    const ::wasp::binary::FuncType& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} -> {}", self.param_types,
                   self.result_types);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::TableType>::format(
    const ::wasp::binary::TableType& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} {}", self.limits, self.elemtype);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::MemoryType>::format(
    const ::wasp::binary::MemoryType& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{}", self.limits);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::GlobalType>::format(
    const ::wasp::binary::GlobalType& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} {}", self.mut, self.valtype);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Import<Traits>>::format(
    const ::wasp::binary::Import<Traits>& self,
    Ctx& ctx) {
  using ::wasp::get;
  using ::wasp::holds_alternative;

  auto it = ctx.begin();
  it = format_to(it, "{{module \"{}\", name \"{}\", desc {}", self.module,
                 self.name, self.kind());

  if (holds_alternative<::wasp::Index>(self.desc)) {
    it = format_to(it, " {}}}", get<::wasp::Index>(self.desc));
  } else if (holds_alternative<::wasp::binary::TableType>(self.desc)) {
    it = format_to(it, " {}}}", get<::wasp::binary::TableType>(self.desc));
  } else if (holds_alternative<::wasp::binary::MemoryType>(self.desc)) {
    it = format_to(it, " {}}}", get<::wasp::binary::MemoryType>(self.desc));
  } else if (holds_alternative<::wasp::binary::GlobalType>(self.desc)) {
    it = format_to(it, " {}}}", get<::wasp::binary::GlobalType>(self.desc));
  }
  return it;
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Export<Traits>>::format(
    const ::wasp::binary::Export<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{name \"{}\", desc {} {}}}", self.name,
                   self.kind, self.index);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Expr<Traits>>::format(
    const ::wasp::binary::Expr<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{}", self.data);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Opcode>::format(
    const ::wasp::binary::Opcode& self,
    Ctx& ctx) {
  if (self.prefix) {
    return format_to(ctx.begin(), "{:02x} {:08x}", *self.prefix, self.code);
  } else {
    return format_to(ctx.begin(), "{:02x}", self.code);
  }
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::CallIndirectImmediate>::format(
    const ::wasp::binary::CallIndirectImmediate& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} {}", self.index, self.reserved);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::BrTableImmediate>::format(
    const ::wasp::binary::BrTableImmediate& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{} {}", self.targets, self.default_target);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Instr>::format(
    const ::wasp::binary::Instr& self,
    Ctx& ctx) {
  using ::wasp::get;
  using ::wasp::holds_alternative;

  auto it = ctx.begin();
  it = format_to(it, "{}", self.opcode);

  if (holds_alternative<::wasp::binary::EmptyImmediate>(self.immediate)) {
    // Nothing.
  } else if (holds_alternative<::wasp::binary::ValType>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::binary::ValType>(self.immediate));
  } else if (holds_alternative<::wasp::Index>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::Index>(self.immediate));
  } else if (holds_alternative<::wasp::binary::CallIndirectImmediate>(
                 self.immediate)) {
    it = format_to(it, " {}",
                get<::wasp::binary::CallIndirectImmediate>(self.immediate));
  } else if (holds_alternative<::wasp::binary::BrTableImmediate>(
                 self.immediate)) {
    it = format_to(it, " {}",
                get<::wasp::binary::BrTableImmediate>(self.immediate));
  } else if (holds_alternative<::wasp::u8>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::u8>(self.immediate));
  } else if (holds_alternative<::wasp::binary::MemArg>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::binary::MemArg>(self.immediate));
  } else if (holds_alternative<::wasp::s32>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::s32>(self.immediate));
  } else if (holds_alternative<::wasp::s64>(self.immediate)) {
    it = format_to(it, " {}", get<::wasp::s64>(self.immediate));
  } else if (holds_alternative<::wasp::f32>(self.immediate)) {
    it = format_to(it, " {:f}", get<::wasp::f32>(self.immediate));
  } else if (holds_alternative<::wasp::f64>(self.immediate)) {
    it = format_to(it, " {:f}", get<::wasp::f64>(self.immediate));
  }
  return it;
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Func>::format(
    const ::wasp::binary::Func& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{type {}}}", self.type_index);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Table>::format(
    const ::wasp::binary::Table& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{type {}}}", self.table_type);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Memory>::format(
    const ::wasp::binary::Memory& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{type {}}}", self.memory_type);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Global<Traits>>::format(
    const ::wasp::binary::Global<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{type {}, init {}}}", self.global_type,
                   self.init_expr);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Start>::format(
    const ::wasp::binary::Start& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{func {}}}", self.func_index);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator
formatter<::wasp::binary::ElementSegment<Traits>>::format(
    const ::wasp::binary::ElementSegment<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{table {}, offset {}, init {}}}",
                   self.table_index, self.offset, self.init);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::Code<Traits>>::format(
    const ::wasp::binary::Code<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{locals {}, body {}}}", self.local_decls,
                   self.body);
}

template <typename Traits>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::binary::DataSegment<Traits>>::format(
    const ::wasp::binary::DataSegment<Traits>& self,
    Ctx& ctx) {
  return format_to(ctx.begin(), "{{memory {}, offset {}, init {}}}",
                   self.memory_index, self.offset, self.init);
}

}  // namespace fmt

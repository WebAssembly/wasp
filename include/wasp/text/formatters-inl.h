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

#include "wasp/text/formatters.h"

#include <cassert>

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace wasp {

// ReferenceType
WASP_DEFINE_VARIANT_NAME(text::RefType, "ref_type")

// ValueType.
WASP_DEFINE_VARIANT_NAME(text::ReferenceType, "reference_type")

// Instruction.
WASP_DEFINE_VARIANT_NAME(text::OpcodeInfo, "opcode_info")
WASP_DEFINE_VARIANT_NAME(text::LiteralInfo, "literal_info")
WASP_DEFINE_VARIANT_NAME(text::Text, "text")
WASP_DEFINE_VARIANT_NAME(text::BlockImmediate, "block")
WASP_DEFINE_VARIANT_NAME(text::BrTableImmediate, "br_table")
WASP_DEFINE_VARIANT_NAME(text::BrOnExnImmediate, "br_on_exn")
WASP_DEFINE_VARIANT_NAME(text::CallIndirectImmediate, "call_indirect")
WASP_DEFINE_VARIANT_NAME(text::CopyImmediate, "copy")
WASP_DEFINE_VARIANT_NAME(text::FuncBindImmediate, "func.bind")
WASP_DEFINE_VARIANT_NAME(text::InitImmediate, "init")
WASP_DEFINE_VARIANT_NAME(text::LetImmediate, "let")
WASP_DEFINE_VARIANT_NAME(text::MemArgImmediate, "mem_arg")
WASP_DEFINE_VARIANT_NAME(text::HeapType, "heap_type")
WASP_DEFINE_VARIANT_NAME(text::SelectImmediate, "select")
WASP_DEFINE_VARIANT_NAME(text::Var, "var")

// Import.
WASP_DEFINE_VARIANT_NAME(text::FunctionDesc, "func")
WASP_DEFINE_VARIANT_NAME(text::TableDesc, "table")
WASP_DEFINE_VARIANT_NAME(text::MemoryDesc, "memory")
WASP_DEFINE_VARIANT_NAME(text::GlobalDesc, "global")
WASP_DEFINE_VARIANT_NAME(text::EventDesc, "event")

// ElementList.
WASP_DEFINE_VARIANT_NAME(text::ElementListWithExpressions, "expression")
WASP_DEFINE_VARIANT_NAME(text::ElementListWithVars, "var")

// ModuleItem.
WASP_DEFINE_VARIANT_NAME(text::TypeEntry, "type")
WASP_DEFINE_VARIANT_NAME(text::Import, "import")
WASP_DEFINE_VARIANT_NAME(text::Function, "func")
WASP_DEFINE_VARIANT_NAME(text::Table, "table")
WASP_DEFINE_VARIANT_NAME(text::Memory, "memory")
WASP_DEFINE_VARIANT_NAME(text::Global, "global")
WASP_DEFINE_VARIANT_NAME(text::Export, "export")
WASP_DEFINE_VARIANT_NAME(text::Start, "start")
WASP_DEFINE_VARIANT_NAME(text::ElementSegment, "elem")
WASP_DEFINE_VARIANT_NAME(text::DataSegment, "data")
WASP_DEFINE_VARIANT_NAME(text::Event, "event")

// ScriptModule.
WASP_DEFINE_VARIANT_NAME(text::Module, "module")
WASP_DEFINE_VARIANT_NAME(text::TextList, "text_list")

// Const.
WASP_DEFINE_VARIANT_NAME(text::RefNullConst, "ref.null")
WASP_DEFINE_VARIANT_NAME(text::RefExternConst, "ref.extern")

// Action.
WASP_DEFINE_VARIANT_NAME(text::InvokeAction, "invoke")
WASP_DEFINE_VARIANT_NAME(text::GetAction, "get")

// FloatResult.
WASP_DEFINE_VARIANT_NAME(text::NanKind, "nan")

// ReturnResult.
WASP_DEFINE_VARIANT_NAME(text::F32Result, "f32")
WASP_DEFINE_VARIANT_NAME(text::F64Result, "f64")
WASP_DEFINE_VARIANT_NAME(text::F32x4Result, "f32x4")
WASP_DEFINE_VARIANT_NAME(text::F64x2Result, "f64x2")
WASP_DEFINE_VARIANT_NAME(text::RefExternResult, "ref.extern")
WASP_DEFINE_VARIANT_NAME(text::RefFuncResult, "ref.func")

// Assertion.
WASP_DEFINE_VARIANT_NAME(text::ModuleAssertion, "module")
WASP_DEFINE_VARIANT_NAME(text::ActionAssertion, "action")
WASP_DEFINE_VARIANT_NAME(text::ReturnAssertion, "return")

// Command.
WASP_DEFINE_VARIANT_NAME(text::ScriptModule, "module")
WASP_DEFINE_VARIANT_NAME(text::Register, "register")
WASP_DEFINE_VARIANT_NAME(text::Action, "action")
WASP_DEFINE_VARIANT_NAME(text::Assertion, "assertion")

}  // namespace wasp

namespace fmt {

WASP_TEXT_STRUCTS(WASP_FORMATTER_VARGS)

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::TokenType>::format(
    const ::wasp::text::TokenType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(Name)                  \
  case ::wasp::text::TokenType::Name: \
    result = #Name;                   \
    break;
#include "wasp/text/token_type.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Sign>::format(
    const ::wasp::text::Sign& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::Sign::None:  result = "None"; break;
    case ::wasp::text::Sign::Plus:  result = "Plus"; break;
    case ::wasp::text::Sign::Minus: result = "Minus"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::LiteralKind>::format(
    const ::wasp::text::LiteralKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::LiteralKind::Normal:     result = "Normal"; break;
    case ::wasp::text::LiteralKind::Nan:        result = "Nan"; break;
    case ::wasp::text::LiteralKind::NanPayload: result = "NanPayload"; break;
    case ::wasp::text::LiteralKind::Infinity:   result = "Infinity"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Base>::format(
    const ::wasp::text::Base& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::Base::Decimal: result = "Decimal"; break;
    case ::wasp::text::Base::Hex:     result = "Hex"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::HasUnderscores>::format(
    const ::wasp::text::HasUnderscores& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::HasUnderscores::No:  result = "No"; break;
    case ::wasp::text::HasUnderscores::Yes: result = "Yes"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Var>::format(
    const ::wasp::text::Var& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_index()) {
    format_to(buf, "{}", self.index());
  } else {
    format_to(buf, "{}", self.name());
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::ModuleItem>::format(
    const ::wasp::text::ModuleItem& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.desc);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Const>::format(
    const ::wasp::text::Const& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.value);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::ScriptModuleKind>::format(
    const ::wasp::text::ScriptModuleKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::ScriptModuleKind::Binary:  result = "binary"; break;
    case ::wasp::text::ScriptModuleKind::Text: result = "text"; break;
    case ::wasp::text::ScriptModuleKind::Quote: result = "quote"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::AssertionKind>::format(
    const ::wasp::text::AssertionKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::AssertionKind::Malformed:  result = "malformed"; break;
    case ::wasp::text::AssertionKind::Invalid: result = "invalid"; break;
    case ::wasp::text::AssertionKind::Unlinkable: result = "unlinkable"; break;
    case ::wasp::text::AssertionKind::ActionTrap: result = "action_trap"; break;
    case ::wasp::text::AssertionKind::Return: result = "return"; break;
    case ::wasp::text::AssertionKind::ModuleTrap: result = "module_trap"; break;
    case ::wasp::text::AssertionKind::Exhaustion: result = "exhaustion"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::NanKind>::format(
    const ::wasp::text::NanKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::NanKind::Canonical:  result = "canonical"; break;
    case ::wasp::text::NanKind::Arithmetic: result = "arithmetic"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Command>::format(
    const ::wasp::text::Command& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{}", self.contents);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::HeapType>::format(
    const ::wasp::text::HeapType& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_heap_kind()) {
    format_to(buf, "{}", self.heap_kind());
  } else {
    assert(self.is_var());
    format_to(buf, "{}", self.var());
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::RefType>::format(
    const ::wasp::text::RefType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "ref ");
  if (self.null == ::wasp::Null::Yes) {
    format_to(buf, "null ");
  }
  format_to(buf, "{}", self.heap_type);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::ReferenceType>::format(
    const ::wasp::text::ReferenceType& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_reference_kind()) {
    format_to(buf, "{}", self.reference_kind());
  } else {
    assert(self.is_ref());
    format_to(buf, "{}", self.ref());
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::ValueType>::format(
    const ::wasp::text::ValueType& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_numeric_type()) {
    format_to(buf, "{}", self.numeric_type());
  } else {
    assert(self.is_reference_type());
    format_to(buf, "{}", self.reference_type());
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::TableType>::format(
    const ::wasp::text::TableType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.limits, self.elemtype);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::GlobalType>::format(
    const ::wasp::text::GlobalType& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "{} {}", self.mut, self.valtype);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

}  // namespace fmt

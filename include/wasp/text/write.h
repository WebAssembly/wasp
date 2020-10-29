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

#ifndef WASP_TEXT_WRITE_H_
#define WASP_TEXT_WRITE_H_

#include <algorithm>
#include <cassert>
#include <string>
#include <type_traits>

#include "wasp/base/concat.h"
#include "wasp/base/formatters.h"
#include "wasp/base/types.h"
#include "wasp/base/v128.h"
#include "wasp/text/numeric.h"
#include "wasp/text/types.h"

namespace wasp::text {

// TODO: Rename to Context and put in write namespace?
struct WriteContext {
  void ClearSeparator() { separator = ""; }
  void Space() { separator = " "; }
  void Newline() { separator = indent; }

  void Indent() { indent += "  "; }
  void Dedent() { indent.erase(indent.size() - 2); }

  std::string separator;
  std::string indent = "\n";
  Base base = Base::Decimal;
};

// WriteRaw
template <typename Iterator>
Iterator WriteRaw(WriteContext& context, char value, Iterator out) {
  *out++ = value;
  return out;
}

template <typename Iterator>
Iterator WriteRaw(WriteContext& context, string_view value, Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteRaw(WriteContext& context,
                  const std::string& value,
                  Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteSeparator(WriteContext& context, Iterator out) {
  out = WriteRaw(context, context.separator, out);
  context.ClearSeparator();
  return out;
}

// WriteFormat
template <typename Iterator, typename T>
Iterator WriteFormat(WriteContext& context, const T& value, Iterator out) {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, concat(value), out);
  context.Space();
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteContext& context, Iterator out) {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, '(', out);
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteContext& context, string_view name, Iterator out) {
  out = WriteLpar(context, out);
  out = WriteRaw(context, name, out);
  context.Space();
  return out;
}

template <typename Iterator>
Iterator WriteRpar(WriteContext& context, Iterator out) {
  context.ClearSeparator();
  out = WriteRaw(context, ')', out);
  context.Space();
  return out;
}

template <typename Iterator, typename SourceIter>
Iterator WriteRange(WriteContext& context,
                    SourceIter begin,
                    SourceIter end,
                    Iterator out) {
  for (auto iter = begin; iter != end; ++iter) {
    out = Write(context, *iter, out);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator WriteVector(WriteContext& context,
                     const std::vector<T>& values,
                     Iterator out) {
  return WriteRange(context, values.begin(), values.end(), out);
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context,
               const optional<T>& value_opt,
               Iterator out) {
  if (value_opt) {
    out = Write(context, *value_opt, out);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context, const At<T>& value, Iterator out) {
  return Write(context, *value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, string_view value, Iterator out) {
  out = WriteSeparator(context, out);
  out = WriteRaw(context, value, out);
  context.Space();
  return out;
}

template <typename Iterator, typename T>
Iterator WriteNat(WriteContext& context, const At<T>& value, Iterator out) {
  return WriteNat(context, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteInt(WriteContext& context, const At<T>& value, Iterator out) {
  return WriteInt(context, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteFloat(WriteContext& context, const At<T>& value, Iterator out) {
  return WriteFloat(context, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteNat(WriteContext& context, T value, Iterator out) {
  return Write(context, string_view{NatToStr<T>(value, context.base)}, out);
}

template <typename Iterator, typename T>
Iterator WriteInt(WriteContext& context, T value, Iterator out) {
  return Write(context, string_view{IntToStr<T>(value, context.base)}, out);
}

template <typename Iterator, typename T >
Iterator WriteFloat(WriteContext& context, T value, Iterator out) {
  return Write(context, string_view{FloatToStr<T>(value, context.base)}, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, Var value, Iterator out) {
  if (value.is_index()) {
    return WriteNat(context, value.index(), out);
  } else {
    return Write(context, value.name(), out);
  }
}

template <typename Iterator>
Iterator Write(WriteContext& context, const VarList& values, Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Text& value, Iterator out) {
  return Write(context, value.text, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const TextList& values, Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ValueType& value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ValueTypeList& values,
               Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ValueTypeList& values,
               string_view name,
               Iterator out) {
  if (!values.empty()) {
    out = WriteLpar(context, name, out);
    out = Write(context, values, out);
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const StorageType& value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const FieldType& value, Iterator out) {
  out = WriteLpar(context, "field", out);
  out = Write(context, value.name, out);

  if (value.mut == Mutability::Var) {
    out = WriteLpar(context, "mut", out);
  }
  out = Write(context, value.type, out);
  if (value.mut == Mutability::Var) {
    out = WriteRpar(context, out);
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const FieldTypeList& values,
               Iterator out) {
  bool first = true;
  bool prev_has_name = false;
  for (auto& value : values) {
    bool has_name = value->name.has_value();
    if ((has_name || prev_has_name) && !first) {
      out = WriteRpar(context, out);
    }
    if (has_name || prev_has_name || first) {
      out = WriteLpar(context, "field", out);
    }
    if (has_name) {
      out = Write(context, value->name, out);
    }

    if (value->mut == Mutability::Var) {
      out = WriteLpar(context, "mut", out);
    }
    out = Write(context, value->type, out);
    if (value->mut == Mutability::Var) {
      out = WriteRpar(context, out);
    }

    prev_has_name = has_name;
    first = false;
  }
  if (!values.empty()) {
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const StructType& value, Iterator out) {
  out = WriteLpar(context, "struct", out);
  out = Write(context, value.fields, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ArrayType& value, Iterator out) {
  out = WriteLpar(context, "array", out);
  out = Write(context, value.field, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const FunctionType& value, Iterator out) {
  out = Write(context, value.params, "param", out);
  out = Write(context, value.results, "result", out);
  return out;
}

template <typename Iterator>
Iterator WriteTypeUse(WriteContext& context,
                      const OptAt<Var>& value,
                      Iterator out) {
  if (value) {
    out = WriteLpar(context, "type", out);
    out = Write(context, value->value(), out);
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const FunctionTypeUse& value,
               Iterator out) {
  out = WriteTypeUse(context, value.type_use, out);
  out = Write(context, *value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const v128& value, Iterator out) {
  u32x4 immediate = value.as<u32x4>();
  out = Write(context, "i32x4"_sv, out);
  for (auto& lane : immediate) {
    out = WriteInt(context, lane, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BlockImmediate& value,
               Iterator out) {
  out = Write(context, value.label, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const HeapType2Immediate& value,
               Iterator out) {
  out = Write(context, *value.parent, out);
  out = Write(context, *value.child, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BrOnCastImmediate& value,
               Iterator out) {
  out = Write(context, *value.target, out);
  out = Write(context, value.types, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BrOnExnImmediate& value,
               Iterator out) {
  out = Write(context, *value.target, out);
  out = Write(context, *value.event, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BrTableImmediate& value,
               Iterator out) {
  out = Write(context, value.targets, out);
  out = Write(context, *value.default_target, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const CallIndirectImmediate& value,
               Iterator out) {
  out = Write(context, value.table, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const CopyImmediate& value,
               Iterator out) {
  out = Write(context, value.dst, out);
  out = Write(context, value.src, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const InitImmediate& value,
               Iterator out) {
  // Write dcontext, out, st first, if it exists.
  if (value.dst) {
    out = Write(context, value.dst->value(), out);
  }
  out = Write(context, *value.segment, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const LetImmediate& value,
               Iterator out) {
  out = Write(context, value.block, out);
  out = Write(context, value.locals, "local", out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const MemArgImmediate& value,
               Iterator out) {
  if (value.offset) {
    out = Write(context, "offset="_sv, out);
    context.ClearSeparator();
    out = WriteNat(context, value.offset->value(), out);
  }

  if (value.align) {
    out = Write(context, "align="_sv, out);
    context.ClearSeparator();
    out = WriteNat(context, value.align->value(), out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const RttSubImmediate& value,
               Iterator out) {
  out = WriteNat(context, *value.depth, out);
  out = Write(context, value.types, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ShuffleImmediate& value,
               Iterator out) {
  for (auto& lane : value) {
    out = WriteNat(context, lane, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const StructFieldImmediate& value,
               Iterator out) {
  out = Write(context, *value.struct_, out);
  out = Write(context, *value.field, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Opcode& value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Instruction& value, Iterator out) {
  out = Write(context, *value.opcode, out);

  switch (value.kind()) {
    case InstructionKind::None:
      break;

    case InstructionKind::S32:
      out = WriteInt(context, value.s32_immediate(), out);
      break;

    case InstructionKind::S64:
      out = WriteInt(context, value.s64_immediate(), out);
      break;

    case InstructionKind::F32:
      out = WriteFloat(context, value.f32_immediate(), out);
      break;

    case InstructionKind::F64:
      out = WriteFloat(context, value.f64_immediate(), out);
      break;

    case InstructionKind::V128:
      out = Write(context, value.v128_immediate(), out);
      break;

    case InstructionKind::Var:
      out = Write(context, value.var_immediate(), out);
      break;

    case InstructionKind::Block:
      out = Write(context, value.block_immediate(), out);
      break;

    case InstructionKind::BrOnExn:
      out = Write(context, value.br_on_exn_immediate(), out);
      break;

    case InstructionKind::BrTable:
      out = Write(context, value.br_table_immediate(), out);
      break;

    case InstructionKind::CallIndirect:
      out = Write(context, value.call_indirect_immediate(), out);
      break;

    case InstructionKind::Copy:
      out = Write(context, value.copy_immediate(), out);
      break;

    case InstructionKind::Init:
      out = Write(context, value.init_immediate(), out);
      break;

    case InstructionKind::Let:
      out = Write(context, value.let_immediate(), out);
      break;

    case InstructionKind::MemArg:
      out = Write(context, value.mem_arg_immediate(), out);
      break;

    case InstructionKind::HeapType:
      out = Write(context, value.heap_type_immediate(), out);
      break;

    case InstructionKind::Select:
      out = Write(context, value.select_immediate(), out);
      break;

    case InstructionKind::Shuffle:
      out = Write(context, value.shuffle_immediate(), out);
      break;

    case InstructionKind::SimdLane:
      out = WriteNat(context, value.simd_lane_immediate(), out);
      break;

    case InstructionKind::FuncBind:
      out = Write(context, value.func_bind_immediate(), out);
      break;

    case InstructionKind::BrOnCast:
      out = Write(context, value.br_on_cast_immediate(), out);
      break;

    case InstructionKind::HeapType2:
      out = Write(context, value.heap_type_2_immediate(), out);
      break;

    case InstructionKind::RttSub:
      out = Write(context, value.rtt_sub_immediate(), out);
      break;

    case InstructionKind::StructField:
      out = Write(context, value.struct_field_immediate(), out);
      break;
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const InstructionList& values,
               Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator WriteWithNewlines(WriteContext& context,
                           const InstructionList& values,
                           Iterator out) {
  for (auto& value : values) {
    auto opcode = value->opcode;
    if (opcode == Opcode::End || opcode == Opcode::Else ||
        opcode == Opcode::Catch) {
      context.Dedent();
      context.Newline();
    }

    out = Write(context, value, out);

    if (value->has_block_immediate() || opcode == Opcode::Else ||
        opcode == Opcode::Catch) {
      context.Indent();
    }
    context.Newline();
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BoundValueType& value,
               Iterator out) {
  out = Write(context, value.name, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BoundValueTypeList& values,
               string_view prefix,
               Iterator out) {
  bool first = true;
  bool prev_has_name = false;
  for (auto& value : values) {
    bool has_name = value->name.has_value();
    if ((has_name || prev_has_name) && !first) {
      out = WriteRpar(context, out);
    }
    if (has_name || prev_has_name || first) {
      out = WriteLpar(context, prefix, out);
    }
    if (has_name) {
      out = Write(context, value->name, out);
    }
    out = Write(context, value->type, out);
    prev_has_name = has_name;
    first = false;
  }
  if (!values.empty()) {
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const BoundFunctionType& value,
               Iterator out) {
  out = Write(context, value.params, "param", out);
  out = Write(context, value.results, "result", out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const DefinedType& value, Iterator out) {
  out = WriteLpar(context, "type", out);
  out = Write(context, value.name, out);
  if (value.is_function_type()) {
    out = WriteLpar(context, "func", out);
    out = Write(context, value.function_type(), out);
    out = WriteRpar(context, out);
  } else if (value.is_struct_type()) {
    out = Write(context, value.struct_type(), out);
  } else {
    assert(value.is_array_type());
    out = Write(context, value.array_type(), out);
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const FunctionDesc& value, Iterator out) {
  out = Write(context, "func"_sv, out);
  out = Write(context, value.name, out);
  out = WriteTypeUse(context, value.type_use, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Limits& value, Iterator out) {
  out = WriteNat(context, value.min, out);
  if (value.max) {
    out = WriteNat(context, *value.max, out);
  }
  if (value.shared == Shared::Yes) {
    out = Write(context, "shared"_sv, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, HeapType value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, ReferenceType value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Rtt& value, Iterator out) {
  out = WriteLpar(context, "rtt", out);
  out = WriteNat(context, value.depth, out);
  out = Write(context, value.type, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const TableType& value, Iterator out) {
  out = Write(context, value.limits, out);
  out = WriteFormat(context, value.elemtype, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const TableDesc& value, Iterator out) {
  out = Write(context, "table"_sv, out);
  out = Write(context, value.name, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const MemoryType& value, Iterator out) {
  out = Write(context, value.limits, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const MemoryDesc& value, Iterator out) {
  out = Write(context, "memory"_sv, out);
  out = Write(context, value.name, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const GlobalType& value, Iterator out) {
  if (value.mut == Mutability::Var) {
    out = WriteLpar(context, "mut", out);
  }
  out = Write(context, value.valtype, out);
  if (value.mut == Mutability::Var) {
    out = WriteRpar(context, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const GlobalDesc& value, Iterator out) {
  out = Write(context, "global"_sv, out);
  out = Write(context, value.name, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const EventType& value, Iterator out) {
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const EventDesc& value, Iterator out) {
  out = Write(context, "event"_sv, out);
  out = Write(context, value.name, out);
  out = Write(context, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Import& value, Iterator out) {
  out = WriteLpar(context, "import"_sv, out);
  out = Write(context, value.module, out);
  out = Write(context, value.name, out);
  out = WriteLpar(context, out);
  switch (value.kind()) {
    case ExternalKind::Function:
      out = Write(context, value.function_desc(), out);
      break;

    case ExternalKind::Table:
      out = Write(context, value.table_desc(), out);
      break;

    case ExternalKind::Memory:
      out = Write(context, value.memory_desc(), out);
      break;

    case ExternalKind::Global:
      out = Write(context, value.global_desc(), out);
      break;

    case ExternalKind::Event:
      out = Write(context, value.event_desc(), out);
      break;
  }
  out = WriteRpar(context, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const InlineImport& value, Iterator out) {
  out = WriteLpar(context, "import"_sv, out);
  out = Write(context, value.module, out);
  out = Write(context, value.name, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const InlineExport& value, Iterator out) {
  out = WriteLpar(context, "export"_sv, out);
  out = Write(context, value.name, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const InlineExportList& values,
               Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Function& value, Iterator out) {
  out = WriteLpar(context, "func", out);

  // Can't write FunctionDesc directly, since inline imports/exports occur
  // between the bindvar and the type use.
  out = Write(context, value.desc.name, out);
  out = Write(context, value.exports, out);

  if (value.import) {
    out = Write(context, *value.import, out);
  }

  out = WriteTypeUse(context, value.desc.type_use, out);
  out = Write(context, value.desc.type, out);

  if (!value.import) {
    context.Indent();
    context.Newline();
    out = Write(context, value.locals, "local", out);
    context.Newline();
    out = WriteWithNewlines(context, value.instructions, out);
    context.Dedent();
  }

  out = WriteRpar(context, out);
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ElementExpressionList& elem_exprs,
               Iterator out) {
  // Use spaces instead of newlines for element expressions.
  for (auto& elem_expr : elem_exprs) {
    for (auto& instr : elem_expr->instructions) {
      // Expressions need to be wrapped in parens.
      out = WriteLpar(context, out);
      out = Write(context, instr, out);
      out = WriteRpar(context, out);
      context.Space();
    }
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ElementListWithExpressions& value,
               Iterator out) {
  out = Write(context, value.elemtype, out);
  out = Write(context, value.list, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, ExternalKind value, Iterator out) {
  return WriteFormat(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ElementListWithVars& value,
               Iterator out) {
  out = Write(context, value.kind, out);
  out = Write(context, value.list, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ElementList& value, Iterator out) {
  if (holds_alternative<ElementListWithVars>(value)) {
    return Write(context, get<ElementListWithVars>(value), out);
  } else {
    return Write(context, get<ElementListWithExpressions>(value), out);
  }
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Table& value, Iterator out) {
  out = WriteLpar(context, "table", out);

  // Can't write TableDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, value.desc.name, out);
  out = Write(context, value.exports, out);

  if (value.import) {
    out = Write(context, *value.import, out);
    out = Write(context, value.desc.type, out);
  } else if (value.elements) {
    // Don't write the limits, because they are implicitly defined by the
    // element segment length.
    out = Write(context, value.desc.type->elemtype, out);
    out = WriteLpar(context, "elem", out);
    // Only write the list of elements, without the ExternalKind or
    // ReferenceType.
    if (holds_alternative<ElementListWithVars>(*value.elements)) {
      out = Write(context, get<ElementListWithVars>(*value.elements).list, out);
    } else {
      out = Write(context,
                  get<ElementListWithExpressions>(*value.elements).list, out);
    }
    out = WriteRpar(context, out);
  } else {
    out = Write(context, value.desc.type, out);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Memory& value, Iterator out) {
  out = WriteLpar(context, "memory", out);

  // Can't write MemoryDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, value.desc.name, out);
  out = Write(context, value.exports, out);

  if (value.import) {
    out = Write(context, *value.import, out);
    out = Write(context, value.desc.type, out);
  } else if (value.data) {
    out = WriteLpar(context, "data", out);
    out = Write(context, value.data, out);
    out = WriteRpar(context, out);
  } else {
    out = Write(context, value.desc.type, out);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ConstantExpression& value,
               Iterator out) {
  return Write(context, value.instructions, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Global& value, Iterator out) {
  out = WriteLpar(context, "global", out);

  // Can't write GlobalDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, value.desc.name, out);
  out = Write(context, value.exports, out);

  if (value.import) {
    out = Write(context, *value.import, out);
    out = Write(context, value.desc.type, out);
  } else {
    out = Write(context, value.desc.type, out);
    out = Write(context, value.init, out);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Export& value, Iterator out) {
  out = WriteLpar(context, "export", out);
  out = Write(context, value.name, out);
  out = WriteLpar(context, out);
  out = Write(context, value.kind, out);
  out = Write(context, value.var, out);
  out = WriteRpar(context, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Start& value, Iterator out) {
  out = WriteLpar(context, "start", out);
  out = Write(context, value.var, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ElementExpression& value,
               Iterator out) {
  return Write(context, value.instructions, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ElementSegment& value,
               Iterator out) {
  out = WriteLpar(context, "elem", out);
  out = Write(context, value.name, out);
  switch (value.type) {
    case SegmentType::Active:
      if (value.table) {
        out = WriteLpar(context, "table", out);
        out = Write(context, *value.table, out);
        out = WriteRpar(context, out);
      }
      if (value.offset) {
        out = WriteLpar(context, "offset", out);
        out = Write(context, *value.offset, out);
        out = WriteRpar(context, out);
      }

      // When writing a function var list, we can omit the "func" keyword to
      // remain compatible with the MVP text format.
      if (holds_alternative<ElementListWithVars>(value.elements)) {
        auto& element_vars = get<ElementListWithVars>(value.elements);
        //  The legacy format which omits the external kind cannot be used with
        //  the "table use" or bind_var syntax.
        if (element_vars.kind != ExternalKind::Function || value.table ||
            value.name) {
          out = Write(context, element_vars.kind, out);
        }
        out = Write(context, element_vars.list, out);
      } else {
        out = Write(context,
                    get<ElementListWithExpressions>(value.elements), out);
      }
      break;

    case SegmentType::Passive:
      out = Write(context, value.elements, out);
      break;

    case SegmentType::Declared:
      out = Write(context, "declare"_sv, out);
      out = Write(context, value.elements, out);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const DataSegment& value, Iterator out) {
  out = WriteLpar(context, "data", out);
  out = Write(context, value.name, out);
  if (value.type == SegmentType::Active) {
    if (value.memory) {
      out = WriteLpar(context, "memory", out);
      out = Write(context, *value.memory, out);
      out = WriteRpar(context, out);
    }
    if (value.offset) {
      out = WriteLpar(context, "offset", out);
      out = Write(context, *value.offset, out);
      out = WriteRpar(context, out);
    }
  }

  out = Write(context, value.data, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Event& value, Iterator out) {
  out = WriteLpar(context, "event", out);

  // Can't write EventDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(context, value.desc.name, out);
  out = Write(context, value.exports, out);

  if (value.import) {
    out = Write(context, *value.import, out);
    out = Write(context, value.desc.type, out);
  } else {
    out = Write(context, value.desc.type, out);
  }

  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ModuleItem& value, Iterator out) {
  switch (value.kind()) {
    case ModuleItemKind::DefinedType:
      out = Write(context, value.defined_type(), out);
      break;

    case ModuleItemKind::Import:
      out = Write(context, value.import(), out);
      break;

    case ModuleItemKind::Function:
      out = Write(context, value.function(), out);
      break;

    case ModuleItemKind::Table:
      out = Write(context, value.table(), out);
      break;

    case ModuleItemKind::Memory:
      out = Write(context, value.memory(), out);
      break;

    case ModuleItemKind::Global:
      out = Write(context, value.global(), out);
      break;

    case ModuleItemKind::Export:
      out = Write(context, value.export_(), out);
      break;

    case ModuleItemKind::Start:
      out = Write(context, value.start(), out);
      break;

    case ModuleItemKind::ElementSegment:
      out = Write(context, value.element_segment(), out);
      break;

    case ModuleItemKind::DataSegment:
      out = Write(context, value.data_segment(), out);
      break;

    case ModuleItemKind::Event:
      out = Write(context, value.event(), out);
      break;
  }
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Module& value, Iterator out) {
  return WriteVector(context, value, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ScriptModule& value, Iterator out) {
  out = WriteLpar(context, "module", out);
  out = Write(context, value.name, out);
  switch (value.kind) {
    case ScriptModuleKind::Text:
      context.Indent();
      context.Newline();
      out = Write(context, value.module(), out);
      context.Dedent();
      break;

    case ScriptModuleKind::Binary:
      out = Write(context, "binary"_sv, out);
      out = Write(context, value.text_list(), out);
      break;

    case ScriptModuleKind::Quote:
      out = Write(context, "quote"_sv, out);
      out = Write(context, value.text_list(), out);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Const& value, Iterator out) {
  out = WriteLpar(context, out);
  switch (value.kind()) {
    case ConstKind::U32:
      out = Write(context, Opcode::I32Const, out);
      out = WriteInt(context, value.u32_(), out);
      break;

    case ConstKind::U64:
      out = Write(context, Opcode::I64Const, out);
      out = WriteInt(context, value.u64_(), out);
      break;

    case ConstKind::F32:
      out = Write(context, Opcode::F32Const, out);
      out = WriteFloat(context, value.f32_(), out);
      break;

    case ConstKind::F64:
      out = Write(context, Opcode::F64Const, out);
      out = WriteFloat(context, value.f64_(), out);
      break;

    case ConstKind::V128:
      out = Write(context, Opcode::V128Const, out);
      out = Write(context, value.v128_(), out);
      break;

    case ConstKind::RefNull:
      out = Write(context, Opcode::RefNull, out);
      break;

    case ConstKind::RefExtern:
      out = Write(context, "ref.extern"_sv, out);
      out = WriteNat(context, value.ref_extern().var, out);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ConstList& values, Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const InvokeAction& value, Iterator out) {
  out = WriteLpar(context, "invoke", out);
  out = Write(context, value.module, out);
  out = Write(context, value.name, out);
  out = Write(context, value.consts, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const GetAction& value, Iterator out) {
  out = WriteLpar(context, "get", out);
  out = Write(context, value.module, out);
  out = Write(context, value.name, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Action& value, Iterator out) {
  if (holds_alternative<InvokeAction>(value)) {
    out = Write(context, get<InvokeAction>(value), out);
  } else if (holds_alternative<GetAction>(value)) {
    out = Write(context, get<GetAction>(value), out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ModuleAssertion& value,
               Iterator out) {
  out = Write(context, value.module, out);
  context.Newline();
  out = Write(context, value.message, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ActionAssertion& value,
               Iterator out) {
  out = Write(context, value.action, out);
  out = Write(context, value.message, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const NanKind& value, Iterator out) {
  if (value == NanKind::Arithmetic) {
    return Write(context, "nan:arithmetic"_sv, out);
  } else {
    assert(value == NanKind::Canonical);
    return Write(context, "nan:canonical"_sv, out);
  }
}

template <typename Iterator, typename T>
Iterator Write(WriteContext& context,
               const FloatResult<T>& value,
               Iterator out) {
  if (holds_alternative<T>(value)) {
    return WriteFloat(context, get<T>(value), out);
  } else {
    return Write(context, get<NanKind>(value), out);
  }
}

template <typename Iterator, typename T, size_t N>
Iterator Write(WriteContext& context,
               const std::array<FloatResult<T>, N>& value,
               Iterator out) {
  return WriteRange(context, value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator Write(WriteContext& context, const ReturnResult& value, Iterator out) {
  out = WriteLpar(context, out);
  switch (value.index()) {
    case 0: // u32
      out = Write(context, Opcode::I32Const, out);
      out = WriteInt(context, get<u32>(value), out);
      break;

    case 1: // u64
      out = Write(context, Opcode::I64Const, out);
      out = WriteInt(context, get<u64>(value), out);
      break;

    case 2: // v128
      out = Write(context, Opcode::V128Const, out);
      out = Write(context, get<v128>(value), out);
      break;

    case 3: // F32Result
      out = Write(context, Opcode::F32Const, out);
      out = Write(context, get<F32Result>(value), out);
      break;

    case 4: // F64Result
      out = Write(context, Opcode::F64Const, out);
      out = Write(context, get<F64Result>(value), out);
      break;

    case 5: // F32x4Result
      out = Write(context, Opcode::V128Const, out);
      out = Write(context, "f32x4"_sv, out);
      out = Write(context, get<F32x4Result>(value), out);
      break;

    case 6: // F64x2Result
      out = Write(context, Opcode::V128Const, out);
      out = Write(context, "f64x2"_sv, out);
      out = Write(context, get<F64x2Result>(value), out);
      break;

    case 7: // RefNullConst
      out = Write(context, Opcode::RefNull, out);
      break;

    case 8: // RefExternConst
      out = Write(context, "ref.extern"_sv, out);
      out = WriteNat(context, *get<RefExternConst>(value).var, out);
      break;

    case 9: // RefExternResult
      out = Write(context, "ref.extern"_sv, out);
      break;

    case 10: // RefFuncResult
      out = Write(context, "ref.func"_sv, out);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ReturnResultList& values,
               Iterator out) {
  return WriteVector(context, values, out);
}

template <typename Iterator>
Iterator Write(WriteContext& context,
               const ReturnAssertion& value,
               Iterator out) {
  out = Write(context, value.action, out);
  out = Write(context, value.results, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Assertion& value, Iterator out) {
  switch (value.kind) {
    case AssertionKind::Malformed:
      out = WriteLpar(context, "assert_malformed", out);
      context.Indent();
      context.Newline();
      out = Write(context, get<ModuleAssertion>(value.desc), out);
      context.Dedent();
      break;

    case AssertionKind::Invalid:
      out = WriteLpar(context, "assert_invalid", out);
      context.Indent();
      context.Newline();
      out = Write(context, get<ModuleAssertion>(value.desc), out);
      context.Dedent();
      break;

    case AssertionKind::Unlinkable:
      out = WriteLpar(context, "assert_unlinkable", out);
      context.Indent();
      context.Newline();
      out = Write(context, get<ModuleAssertion>(value.desc), out);
      context.Dedent();
      break;

    case AssertionKind::ActionTrap:
      out = WriteLpar(context, "assert_trap", out);
      out = Write(context, get<ActionAssertion>(value.desc), out);
      break;

    case AssertionKind::Return:
      out = WriteLpar(context, "assert_return", out);
      out = Write(context, get<ReturnAssertion>(value.desc), out);
      break;

    case AssertionKind::ModuleTrap:
      out = WriteLpar(context, "assert_trap", out);
      context.Indent();
      context.Newline();
      out = Write(context, get<ModuleAssertion>(value.desc), out);
      context.Dedent();
      break;

    case AssertionKind::Exhaustion:
      out = WriteLpar(context, "assert_exhaustion", out);
      out = Write(context, get<ActionAssertion>(value.desc), out);
      break;
  }
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Register& value, Iterator out) {
  out = WriteLpar(context, "register", out);
  out = Write(context, value.name, out);
  out = Write(context, value.module, out);
  out = WriteRpar(context, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Command& value, Iterator out) {
  switch (value.kind()) {
    case CommandKind::ScriptModule:
      out = Write(context, value.script_module(), out);
      break;

    case CommandKind::Register:
      out = Write(context, value.register_(), out);
      break;

    case CommandKind::Action:
      out = Write(context, value.action(), out);
      break;

    case CommandKind::Assertion:
      out = Write(context, value.assertion(), out);
      break;
  }
  context.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteContext& context, const Script& values, Iterator out) {
  return WriteVector(context, values, out);
}

}  // namespace wasp::text

#endif  // WASP_TEXT_WRITE_H_

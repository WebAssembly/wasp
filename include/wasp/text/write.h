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

struct WriteCtx {
  void ClearSeparator() { separator = ""; }
  void Space() { separator = " "; }
  void Newline() { separator = indent; }

  void Indent() { indent += "  "; }

  void DedentWithMinimum(size_t minimum) {
    if (indent.size() > minimum) {
      indent.erase(indent.size() - 2);
    }
  }

  void Dedent() { DedentWithMinimum(2); }
  void DedentNoToplevel() { DedentWithMinimum(3); }

  std::string separator;
  std::string indent = "\n";
  Base base = Base::Decimal;
};

// WriteRaw
template <typename Iterator>
Iterator WriteRaw(WriteCtx& ctx, char value, Iterator out) {
  *out++ = value;
  return out;
}

template <typename Iterator>
Iterator WriteRaw(WriteCtx& ctx, string_view value, Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteRaw(WriteCtx& ctx, const std::string& value, Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteSeparator(WriteCtx& ctx, Iterator out) {
  out = WriteRaw(ctx, ctx.separator, out);
  ctx.ClearSeparator();
  return out;
}

// WriteFormat
template <typename Iterator, typename T>
Iterator WriteFormat(WriteCtx& ctx, const T& value, Iterator out) {
  out = WriteSeparator(ctx, out);
  out = WriteRaw(ctx, concat(value), out);
  ctx.Space();
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteCtx& ctx, Iterator out) {
  out = WriteSeparator(ctx, out);
  out = WriteRaw(ctx, '(', out);
  return out;
}

template <typename Iterator>
Iterator WriteLpar(WriteCtx& ctx, string_view name, Iterator out) {
  out = WriteLpar(ctx, out);
  out = WriteRaw(ctx, name, out);
  ctx.Space();
  return out;
}

template <typename Iterator>
Iterator WriteRpar(WriteCtx& ctx, Iterator out) {
  ctx.ClearSeparator();
  out = WriteRaw(ctx, ')', out);
  ctx.Space();
  return out;
}

template <typename Iterator, typename SourceIter>
Iterator WriteRange(WriteCtx& ctx,
                    SourceIter begin,
                    SourceIter end,
                    Iterator out) {
  for (auto iter = begin; iter != end; ++iter) {
    out = Write(ctx, *iter, out);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator WriteVector(WriteCtx& ctx,
                     const std::vector<T>& values,
                     Iterator out) {
  return WriteRange(ctx, values.begin(), values.end(), out);
}

template <typename Iterator, typename T>
Iterator Write(WriteCtx& ctx, const optional<T>& value_opt, Iterator out) {
  if (value_opt) {
    out = Write(ctx, *value_opt, out);
  }
  return out;
}

template <typename Iterator, typename T>
Iterator Write(WriteCtx& ctx, const At<T>& value, Iterator out) {
  return Write(ctx, *value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, string_view value, Iterator out) {
  out = WriteSeparator(ctx, out);
  out = WriteRaw(ctx, value, out);
  ctx.Space();
  return out;
}

template <typename Iterator, typename T>
Iterator WriteNat(WriteCtx& ctx, const At<T>& value, Iterator out) {
  return WriteNat(ctx, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteInt(WriteCtx& ctx, const At<T>& value, Iterator out) {
  return WriteInt(ctx, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteFloat(WriteCtx& ctx, const At<T>& value, Iterator out) {
  return WriteFloat(ctx, *value, out);
}

template <typename Iterator, typename T>
Iterator WriteNat(WriteCtx& ctx, T value, Iterator out) {
  return Write(ctx, string_view{NatToStr<T>(value, ctx.base)}, out);
}

template <typename Iterator, typename T>
Iterator WriteInt(WriteCtx& ctx, T value, Iterator out) {
  return Write(ctx, string_view{IntToStr<T>(value, ctx.base)}, out);
}

template <typename Iterator, typename T >
Iterator WriteFloat(WriteCtx& ctx, T value, Iterator out) {
  return Write(ctx, string_view{FloatToStr<T>(value, ctx.base)}, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, Var value, Iterator out) {
  if (value.is_index()) {
    return WriteNat(ctx, value.index(), out);
  } else {
    return Write(ctx, value.name(), out);
  }
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const VarList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Text& value, Iterator out) {
  return Write(ctx, value.text, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const TextList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ValueType& value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ValueTypeList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx,
               const ValueTypeList& values,
               string_view name,
               Iterator out) {
  if (!values.empty()) {
    out = WriteLpar(ctx, name, out);
    out = Write(ctx, values, out);
    out = WriteRpar(ctx, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const StorageType& value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const FieldType& value, Iterator out) {
  out = WriteLpar(ctx, "field", out);
  out = Write(ctx, value.name, out);

  if (value.mut == Mutability::Var) {
    out = WriteLpar(ctx, "mut", out);
  }
  out = Write(ctx, value.type, out);
  if (value.mut == Mutability::Var) {
    out = WriteRpar(ctx, out);
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const FieldTypeList& values, Iterator out) {
  bool first = true;
  bool prev_has_name = false;
  for (auto& value : values) {
    bool has_name = value->name.has_value();
    if ((has_name || prev_has_name) && !first) {
      out = WriteRpar(ctx, out);
    }
    if (has_name || prev_has_name || first) {
      out = WriteLpar(ctx, "field", out);
    }
    if (has_name) {
      out = Write(ctx, value->name, out);
    }

    if (value->mut == Mutability::Var) {
      out = WriteLpar(ctx, "mut", out);
    }
    out = Write(ctx, value->type, out);
    if (value->mut == Mutability::Var) {
      out = WriteRpar(ctx, out);
    }

    prev_has_name = has_name;
    first = false;
  }
  if (!values.empty()) {
    out = WriteRpar(ctx, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const StructType& value, Iterator out) {
  out = WriteLpar(ctx, "struct", out);
  out = Write(ctx, value.fields, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ArrayType& value, Iterator out) {
  out = WriteLpar(ctx, "array", out);
  out = Write(ctx, value.field, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const FunctionType& value, Iterator out) {
  out = Write(ctx, value.params, "param", out);
  out = Write(ctx, value.results, "result", out);
  return out;
}

template <typename Iterator>
Iterator WriteTypeUse(WriteCtx& ctx, const OptAt<Var>& value, Iterator out) {
  if (value) {
    out = WriteLpar(ctx, "type", out);
    out = Write(ctx, value->value(), out);
    out = WriteRpar(ctx, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const FunctionTypeUse& value, Iterator out) {
  out = WriteTypeUse(ctx, value.type_use, out);
  out = Write(ctx, *value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const v128& value, Iterator out) {
  u32x4 immediate = value.as<u32x4>();
  out = Write(ctx, "i32x4"_sv, out);
  for (auto& lane : immediate) {
    out = WriteInt(ctx, lane, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BlockImmediate& value, Iterator out) {
  out = Write(ctx, value.label, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const HeapType2Immediate& value, Iterator out) {
  out = Write(ctx, *value.parent, out);
  out = Write(ctx, *value.child, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BrOnCastImmediate& value, Iterator out) {
  out = Write(ctx, *value.target, out);
  out = Write(ctx, value.types, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BrOnExnImmediate& value, Iterator out) {
  out = Write(ctx, *value.target, out);
  out = Write(ctx, *value.event, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BrTableImmediate& value, Iterator out) {
  out = Write(ctx, value.targets, out);
  out = Write(ctx, *value.default_target, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx,
               const CallIndirectImmediate& value,
               Iterator out) {
  out = Write(ctx, value.table, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const CopyImmediate& value, Iterator out) {
  out = Write(ctx, value.dst, out);
  out = Write(ctx, value.src, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InitImmediate& value, Iterator out) {
  // Write dst first, if it exists.
  if (value.dst) {
    out = Write(ctx, value.dst->value(), out);
  }
  out = Write(ctx, *value.segment, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const LetImmediate& value, Iterator out) {
  out = Write(ctx, value.block, out);
  out = Write(ctx, value.locals, "local", out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const MemArgImmediate& value, Iterator out) {
  if (value.offset) {
    out = Write(ctx, "offset="_sv, out);
    ctx.ClearSeparator();
    out = WriteNat(ctx, value.offset->value(), out);
  }

  if (value.align) {
    out = Write(ctx, "align="_sv, out);
    ctx.ClearSeparator();
    out = WriteNat(ctx, value.align->value(), out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const RttSubImmediate& value, Iterator out) {
  out = WriteNat(ctx, *value.depth, out);
  out = Write(ctx, value.types, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ShuffleImmediate& value, Iterator out) {
  for (auto& lane : value) {
    out = WriteNat(ctx, lane, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const StructFieldImmediate& value, Iterator out) {
  out = Write(ctx, *value.struct_, out);
  out = Write(ctx, *value.field, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Opcode& value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Instruction& value, Iterator out) {
  out = Write(ctx, *value.opcode, out);

  switch (value.kind()) {
    case InstructionKind::None:
      break;

    case InstructionKind::S32:
      out = WriteInt(ctx, value.s32_immediate(), out);
      break;

    case InstructionKind::S64:
      out = WriteInt(ctx, value.s64_immediate(), out);
      break;

    case InstructionKind::F32:
      out = WriteFloat(ctx, value.f32_immediate(), out);
      break;

    case InstructionKind::F64:
      out = WriteFloat(ctx, value.f64_immediate(), out);
      break;

    case InstructionKind::V128:
      out = Write(ctx, value.v128_immediate(), out);
      break;

    case InstructionKind::Var:
      out = Write(ctx, value.var_immediate(), out);
      break;

    case InstructionKind::Block:
      out = Write(ctx, value.block_immediate(), out);
      break;

    case InstructionKind::BrOnExn:
      out = Write(ctx, value.br_on_exn_immediate(), out);
      break;

    case InstructionKind::BrTable:
      out = Write(ctx, value.br_table_immediate(), out);
      break;

    case InstructionKind::CallIndirect:
      out = Write(ctx, value.call_indirect_immediate(), out);
      break;

    case InstructionKind::Copy:
      out = Write(ctx, value.copy_immediate(), out);
      break;

    case InstructionKind::Init:
      out = Write(ctx, value.init_immediate(), out);
      break;

    case InstructionKind::Let:
      out = Write(ctx, value.let_immediate(), out);
      break;

    case InstructionKind::MemArg:
      out = Write(ctx, value.mem_arg_immediate(), out);
      break;

    case InstructionKind::HeapType:
      out = Write(ctx, value.heap_type_immediate(), out);
      break;

    case InstructionKind::Select:
      out = Write(ctx, value.select_immediate(), out);
      break;

    case InstructionKind::Shuffle:
      out = Write(ctx, value.shuffle_immediate(), out);
      break;

    case InstructionKind::SimdLane:
      out = WriteNat(ctx, value.simd_lane_immediate(), out);
      break;

    case InstructionKind::FuncBind:
      out = Write(ctx, value.func_bind_immediate(), out);
      break;

    case InstructionKind::BrOnCast:
      out = Write(ctx, value.br_on_cast_immediate(), out);
      break;

    case InstructionKind::HeapType2:
      out = Write(ctx, value.heap_type_2_immediate(), out);
      break;

    case InstructionKind::RttSub:
      out = Write(ctx, value.rtt_sub_immediate(), out);
      break;

    case InstructionKind::StructField:
      out = Write(ctx, value.struct_field_immediate(), out);
      break;
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InstructionList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator WriteWithNewlines(WriteCtx& ctx,
                           const InstructionList& values,
                           Iterator out) {
  // If the instruction list ends with and `end` instruction, don't write it
  // (it's implicit in the function definition, in the text format.)
  span<const At<Instruction>> instrs{values};
  if (!values.empty() && values.back()->opcode == Opcode::End) {
    instrs.remove_suffix(1);
  }
  for (auto& instr : instrs) {
    auto opcode = instr->opcode;
    if (opcode == Opcode::End || opcode == Opcode::Else ||
        opcode == Opcode::Catch) {
      ctx.DedentNoToplevel();
      ctx.Newline();
    }

    out = Write(ctx, instr, out);

    if (instr->has_block_immediate() || instr->has_let_immediate() ||
        opcode == Opcode::Else || opcode == Opcode::Catch) {
      ctx.Indent();
    }
    ctx.Newline();
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BoundValueType& value, Iterator out) {
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx,
               const BoundValueTypeList& values,
               string_view prefix,
               Iterator out) {
  bool first = true;
  bool prev_has_name = false;
  for (auto& value : values) {
    bool has_name = value->name.has_value();
    if ((has_name || prev_has_name) && !first) {
      out = WriteRpar(ctx, out);
    }
    if (has_name || prev_has_name || first) {
      out = WriteLpar(ctx, prefix, out);
    }
    if (has_name) {
      out = Write(ctx, value->name, out);
    }
    out = Write(ctx, value->type, out);
    prev_has_name = has_name;
    first = false;
  }
  if (!values.empty()) {
    out = WriteRpar(ctx, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const BoundFunctionType& value, Iterator out) {
  out = Write(ctx, value.params, "param", out);
  out = Write(ctx, value.results, "result", out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const DefinedType& value, Iterator out) {
  out = WriteLpar(ctx, "type", out);
  out = Write(ctx, value.name, out);
  if (value.is_function_type()) {
    out = WriteLpar(ctx, "func", out);
    out = Write(ctx, value.function_type(), out);
    out = WriteRpar(ctx, out);
  } else if (value.is_struct_type()) {
    out = Write(ctx, value.struct_type(), out);
  } else {
    assert(value.is_array_type());
    out = Write(ctx, value.array_type(), out);
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const FunctionDesc& value, Iterator out) {
  out = Write(ctx, "func"_sv, out);
  out = Write(ctx, value.name, out);
  out = WriteTypeUse(ctx, value.type_use, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Limits& value, Iterator out) {
  if (value.index_type == IndexType::I64) {
    out = Write(ctx, "i64"_sv, out);
  }
  out = WriteNat(ctx, value.min, out);
  if (value.max) {
    out = WriteNat(ctx, *value.max, out);
  }
  if (value.shared == Shared::Yes) {
    out = Write(ctx, "shared"_sv, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, HeapType value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, ReferenceType value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Rtt& value, Iterator out) {
  out = WriteLpar(ctx, "rtt", out);
  out = WriteNat(ctx, value.depth, out);
  out = Write(ctx, value.type, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const TableType& value, Iterator out) {
  out = Write(ctx, value.limits, out);
  out = WriteFormat(ctx, value.elemtype, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const TableDesc& value, Iterator out) {
  out = Write(ctx, "table"_sv, out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const MemoryType& value, Iterator out) {
  out = Write(ctx, value.limits, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const MemoryDesc& value, Iterator out) {
  out = Write(ctx, "memory"_sv, out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const GlobalType& value, Iterator out) {
  if (value.mut == Mutability::Var) {
    out = WriteLpar(ctx, "mut", out);
  }
  out = Write(ctx, value.valtype, out);
  if (value.mut == Mutability::Var) {
    out = WriteRpar(ctx, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const GlobalDesc& value, Iterator out) {
  out = Write(ctx, "global"_sv, out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const EventType& value, Iterator out) {
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const EventDesc& value, Iterator out) {
  out = Write(ctx, "event"_sv, out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Import& value, Iterator out) {
  out = WriteLpar(ctx, "import"_sv, out);
  out = Write(ctx, value.module, out);
  out = Write(ctx, value.name, out);
  out = WriteLpar(ctx, out);
  switch (value.kind()) {
    case ExternalKind::Function:
      out = Write(ctx, value.function_desc(), out);
      break;

    case ExternalKind::Table:
      out = Write(ctx, value.table_desc(), out);
      break;

    case ExternalKind::Memory:
      out = Write(ctx, value.memory_desc(), out);
      break;

    case ExternalKind::Global:
      out = Write(ctx, value.global_desc(), out);
      break;

    case ExternalKind::Event:
      out = Write(ctx, value.event_desc(), out);
      break;
  }
  out = WriteRpar(ctx, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InlineImport& value, Iterator out) {
  out = WriteLpar(ctx, "import"_sv, out);
  out = Write(ctx, value.module, out);
  out = Write(ctx, value.name, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InlineExport& value, Iterator out) {
  out = WriteLpar(ctx, "export"_sv, out);
  out = Write(ctx, value.name, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InlineExportList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Function& value, Iterator out) {
  out = WriteLpar(ctx, "func", out);

  // Can't write FunctionDesc directly, since inline imports/exports occur
  // between the bindvar and the type use.
  out = Write(ctx, value.desc.name, out);
  out = Write(ctx, value.exports, out);

  if (value.import) {
    out = Write(ctx, *value.import, out);
  }

  out = WriteTypeUse(ctx, value.desc.type_use, out);
  out = Write(ctx, value.desc.type, out);

  if (!value.import) {
    ctx.Indent();
    ctx.Newline();
    out = Write(ctx, value.locals, "local", out);
    ctx.Newline();
    out = WriteWithNewlines(ctx, value.instructions, out);
    ctx.Dedent();
  }

  out = WriteRpar(ctx, out);
  ctx.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx,
               const ElementExpressionList& elem_exprs,
               Iterator out) {
  // Use spaces instead of newlines for element expressions.
  for (auto& elem_expr : elem_exprs) {
    for (auto& instr : elem_expr->instructions) {
      // Expressions need to be wrapped in parens.
      out = WriteLpar(ctx, out);
      out = Write(ctx, instr, out);
      out = WriteRpar(ctx, out);
      ctx.Space();
    }
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx,
               const ElementListWithExpressions& value,
               Iterator out) {
  out = Write(ctx, value.elemtype, out);
  out = Write(ctx, value.list, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, ExternalKind value, Iterator out) {
  return WriteFormat(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ElementListWithVars& value, Iterator out) {
  out = Write(ctx, value.kind, out);
  out = Write(ctx, value.list, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ElementList& value, Iterator out) {
  if (holds_alternative<ElementListWithVars>(value)) {
    return Write(ctx, get<ElementListWithVars>(value), out);
  } else {
    return Write(ctx, get<ElementListWithExpressions>(value), out);
  }
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Table& value, Iterator out) {
  out = WriteLpar(ctx, "table", out);

  // Can't write TableDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(ctx, value.desc.name, out);
  out = Write(ctx, value.exports, out);

  if (value.import) {
    out = Write(ctx, *value.import, out);
    out = Write(ctx, value.desc.type, out);
  } else if (value.elements) {
    // Don't write the limits, because they are implicitly defined by the
    // element segment length.
    out = Write(ctx, value.desc.type->elemtype, out);
    out = WriteLpar(ctx, "elem", out);
    // Only write the list of elements, without the ExternalKind or
    // ReferenceType.
    if (holds_alternative<ElementListWithVars>(*value.elements)) {
      out = Write(ctx, get<ElementListWithVars>(*value.elements).list, out);
    } else {
      out = Write(ctx,
                  get<ElementListWithExpressions>(*value.elements).list, out);
    }
    out = WriteRpar(ctx, out);
  } else {
    out = Write(ctx, value.desc.type, out);
  }

  out = WriteRpar(ctx, out);
  return out;
}

template <typename T, typename Iterator>
Iterator WriteNumericValues(WriteCtx& ctx,
                            const NumericData& value,
                            Iterator out) {
  for (Index i = 0; i < value.count(); ++i) {
    if constexpr (std::is_same_v<T, v128>) {
      out = Write(ctx, value.value<T>(i), out);
    } else if constexpr (std::is_floating_point_v<T>) {
      out = WriteFloat(ctx, value.value<T>(i), out);
    } else {
      out = WriteInt(ctx, value.value<T>(i), out);
    }
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const NumericData& value, Iterator out) {
  switch (value.type) {
    case NumericDataType::I8:
      out = WriteLpar(ctx, "i8", out);
      out = WriteNumericValues<s8>(ctx, value, out);
      break;

    case NumericDataType::I16:
      out = WriteLpar(ctx, "i16", out);
      out = WriteNumericValues<s16>(ctx, value, out);
      break;

    case NumericDataType::I32:
      out = WriteLpar(ctx, "i32", out);
      out = WriteNumericValues<s32>(ctx, value, out);
      break;

    case NumericDataType::I64:
      out = WriteLpar(ctx, "i64", out);
      out = WriteNumericValues<s64>(ctx, value, out);
      break;

    case NumericDataType::F32:
      out = WriteLpar(ctx, "f32", out);
      out = WriteNumericValues<f32>(ctx, value, out);
      break;

    case NumericDataType::F64:
      out = WriteLpar(ctx, "f64", out);
      out = WriteNumericValues<f64>(ctx, value, out);
      break;

    case NumericDataType::V128:
      out = WriteLpar(ctx, "v128", out);
      out = WriteNumericValues<v128>(ctx, value, out);
      break;
  }

  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const DataItem& value, Iterator out) {
  if (value.is_text()) {
    return Write(ctx, value.text(), out);
  } else {
    assert(value.is_numeric_data());
    return Write(ctx, value.numeric_data(), out);
  }
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const DataItemList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Memory& value, Iterator out) {
  out = WriteLpar(ctx, "memory", out);

  // Can't write MemoryDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(ctx, value.desc.name, out);
  out = Write(ctx, value.exports, out);

  if (value.import) {
    out = Write(ctx, *value.import, out);
    out = Write(ctx, value.desc.type, out);
  } else if (value.data) {
    out = WriteLpar(ctx, "data", out);
    out = Write(ctx, value.data, out);
    out = WriteRpar(ctx, out);
  } else {
    out = Write(ctx, value.desc.type, out);
  }

  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ConstantExpression& value, Iterator out) {
  return Write(ctx, value.instructions, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Global& value, Iterator out) {
  out = WriteLpar(ctx, "global", out);

  // Can't write GlobalDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(ctx, value.desc.name, out);
  out = Write(ctx, value.exports, out);

  if (value.import) {
    out = Write(ctx, *value.import, out);
    out = Write(ctx, value.desc.type, out);
  } else {
    out = Write(ctx, value.desc.type, out);
    out = Write(ctx, value.init, out);
  }

  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Export& value, Iterator out) {
  out = WriteLpar(ctx, "export", out);
  out = Write(ctx, value.name, out);
  out = WriteLpar(ctx, out);
  out = Write(ctx, value.kind, out);
  out = Write(ctx, value.var, out);
  out = WriteRpar(ctx, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Start& value, Iterator out) {
  out = WriteLpar(ctx, "start", out);
  out = Write(ctx, value.var, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ElementExpression& value, Iterator out) {
  return Write(ctx, value.instructions, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ElementSegment& value, Iterator out) {
  out = WriteLpar(ctx, "elem", out);
  out = Write(ctx, value.name, out);
  switch (value.type) {
    case SegmentType::Active:
      if (value.table) {
        out = WriteLpar(ctx, "table", out);
        out = Write(ctx, *value.table, out);
        out = WriteRpar(ctx, out);
      }
      if (value.offset) {
        out = WriteLpar(ctx, "offset", out);
        out = Write(ctx, *value.offset, out);
        out = WriteRpar(ctx, out);
      }

      // When writing a function var list, we can omit the "func" keyword to
      // remain compatible with the MVP text format.
      if (holds_alternative<ElementListWithVars>(value.elements)) {
        auto& element_vars = get<ElementListWithVars>(value.elements);
        //  The legacy format which omits the external kind cannot be used with
        //  the "table use" or bind_var syntax.
        if (element_vars.kind != ExternalKind::Function || value.table ||
            value.name) {
          out = Write(ctx, element_vars.kind, out);
        }
        out = Write(ctx, element_vars.list, out);
      } else {
        out = Write(ctx, get<ElementListWithExpressions>(value.elements), out);
      }
      break;

    case SegmentType::Passive:
      out = Write(ctx, value.elements, out);
      break;

    case SegmentType::Declared:
      out = Write(ctx, "declare"_sv, out);
      out = Write(ctx, value.elements, out);
      break;
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const DataSegment& value, Iterator out) {
  out = WriteLpar(ctx, "data", out);
  out = Write(ctx, value.name, out);
  if (value.type == SegmentType::Active) {
    if (value.memory) {
      out = WriteLpar(ctx, "memory", out);
      out = Write(ctx, *value.memory, out);
      out = WriteRpar(ctx, out);
    }
    if (value.offset) {
      out = WriteLpar(ctx, "offset", out);
      out = Write(ctx, *value.offset, out);
      out = WriteRpar(ctx, out);
    }
  }

  out = Write(ctx, value.data, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Event& value, Iterator out) {
  out = WriteLpar(ctx, "event", out);

  // Can't write EventDesc directly, since inline imports/exports occur after
  // the bind var.
  out = Write(ctx, value.desc.name, out);
  out = Write(ctx, value.exports, out);

  if (value.import) {
    out = Write(ctx, *value.import, out);
    out = Write(ctx, value.desc.type, out);
  } else {
    out = Write(ctx, value.desc.type, out);
  }

  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ModuleItem& value, Iterator out) {
  switch (value.kind()) {
    case ModuleItemKind::DefinedType:
      out = Write(ctx, value.defined_type(), out);
      break;

    case ModuleItemKind::Import:
      out = Write(ctx, value.import(), out);
      break;

    case ModuleItemKind::Function:
      out = Write(ctx, value.function(), out);
      break;

    case ModuleItemKind::Table:
      out = Write(ctx, value.table(), out);
      break;

    case ModuleItemKind::Memory:
      out = Write(ctx, value.memory(), out);
      break;

    case ModuleItemKind::Global:
      out = Write(ctx, value.global(), out);
      break;

    case ModuleItemKind::Export:
      out = Write(ctx, value.export_(), out);
      break;

    case ModuleItemKind::Start:
      out = Write(ctx, value.start(), out);
      break;

    case ModuleItemKind::ElementSegment:
      out = Write(ctx, value.element_segment(), out);
      break;

    case ModuleItemKind::DataSegment:
      out = Write(ctx, value.data_segment(), out);
      break;

    case ModuleItemKind::Event:
      out = Write(ctx, value.event(), out);
      break;
  }
  ctx.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Module& value, Iterator out) {
  return WriteVector(ctx, value, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ScriptModule& value, Iterator out) {
  out = WriteLpar(ctx, "module", out);
  out = Write(ctx, value.name, out);
  switch (value.kind) {
    case ScriptModuleKind::Text:
      ctx.Indent();
      ctx.Newline();
      out = Write(ctx, value.module(), out);
      ctx.Dedent();
      break;

    case ScriptModuleKind::Binary:
      out = Write(ctx, "binary"_sv, out);
      out = Write(ctx, value.text_list(), out);
      break;

    case ScriptModuleKind::Quote:
      out = Write(ctx, "quote"_sv, out);
      out = Write(ctx, value.text_list(), out);
      break;
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Const& value, Iterator out) {
  out = WriteLpar(ctx, out);
  switch (value.kind()) {
    case ConstKind::U32:
      out = Write(ctx, Opcode::I32Const, out);
      out = WriteInt(ctx, value.u32_(), out);
      break;

    case ConstKind::U64:
      out = Write(ctx, Opcode::I64Const, out);
      out = WriteInt(ctx, value.u64_(), out);
      break;

    case ConstKind::F32:
      out = Write(ctx, Opcode::F32Const, out);
      out = WriteFloat(ctx, value.f32_(), out);
      break;

    case ConstKind::F64:
      out = Write(ctx, Opcode::F64Const, out);
      out = WriteFloat(ctx, value.f64_(), out);
      break;

    case ConstKind::V128:
      out = Write(ctx, Opcode::V128Const, out);
      out = Write(ctx, value.v128_(), out);
      break;

    case ConstKind::RefNull:
      out = Write(ctx, Opcode::RefNull, out);
      break;

    case ConstKind::RefExtern:
      out = Write(ctx, "ref.extern"_sv, out);
      out = WriteNat(ctx, value.ref_extern().var, out);
      break;
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ConstList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const InvokeAction& value, Iterator out) {
  out = WriteLpar(ctx, "invoke", out);
  out = Write(ctx, value.module, out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.consts, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const GetAction& value, Iterator out) {
  out = WriteLpar(ctx, "get", out);
  out = Write(ctx, value.module, out);
  out = Write(ctx, value.name, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Action& value, Iterator out) {
  if (holds_alternative<InvokeAction>(value)) {
    out = Write(ctx, get<InvokeAction>(value), out);
  } else if (holds_alternative<GetAction>(value)) {
    out = Write(ctx, get<GetAction>(value), out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ModuleAssertion& value, Iterator out) {
  out = Write(ctx, value.module, out);
  ctx.Newline();
  out = Write(ctx, value.message, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ActionAssertion& value, Iterator out) {
  out = Write(ctx, value.action, out);
  out = Write(ctx, value.message, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const NanKind& value, Iterator out) {
  if (value == NanKind::Arithmetic) {
    return Write(ctx, "nan:arithmetic"_sv, out);
  } else {
    assert(value == NanKind::Canonical);
    return Write(ctx, "nan:canonical"_sv, out);
  }
}

template <typename Iterator, typename T>
Iterator Write(WriteCtx& ctx, const FloatResult<T>& value, Iterator out) {
  if (holds_alternative<T>(value)) {
    return WriteFloat(ctx, get<T>(value), out);
  } else {
    return Write(ctx, get<NanKind>(value), out);
  }
}

template <typename Iterator, typename T, size_t N>
Iterator Write(WriteCtx& ctx,
               const std::array<FloatResult<T>, N>& value,
               Iterator out) {
  return WriteRange(ctx, value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ReturnResult& value, Iterator out) {
  out = WriteLpar(ctx, out);
  switch (value.index()) {
    case 0: // u32
      out = Write(ctx, Opcode::I32Const, out);
      out = WriteInt(ctx, get<u32>(value), out);
      break;

    case 1: // u64
      out = Write(ctx, Opcode::I64Const, out);
      out = WriteInt(ctx, get<u64>(value), out);
      break;

    case 2: // v128
      out = Write(ctx, Opcode::V128Const, out);
      out = Write(ctx, get<v128>(value), out);
      break;

    case 3: // F32Result
      out = Write(ctx, Opcode::F32Const, out);
      out = Write(ctx, get<F32Result>(value), out);
      break;

    case 4: // F64Result
      out = Write(ctx, Opcode::F64Const, out);
      out = Write(ctx, get<F64Result>(value), out);
      break;

    case 5: // F32x4Result
      out = Write(ctx, Opcode::V128Const, out);
      out = Write(ctx, "f32x4"_sv, out);
      out = Write(ctx, get<F32x4Result>(value), out);
      break;

    case 6: // F64x2Result
      out = Write(ctx, Opcode::V128Const, out);
      out = Write(ctx, "f64x2"_sv, out);
      out = Write(ctx, get<F64x2Result>(value), out);
      break;

    case 7: // RefNullConst
      out = Write(ctx, Opcode::RefNull, out);
      break;

    case 8: // RefExternConst
      out = Write(ctx, "ref.extern"_sv, out);
      out = WriteNat(ctx, *get<RefExternConst>(value).var, out);
      break;

    case 9: // RefExternResult
      out = Write(ctx, "ref.extern"_sv, out);
      break;

    case 10: // RefFuncResult
      out = Write(ctx, "ref.func"_sv, out);
      break;
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ReturnResultList& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const ReturnAssertion& value, Iterator out) {
  out = Write(ctx, value.action, out);
  out = Write(ctx, value.results, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Assertion& value, Iterator out) {
  switch (value.kind) {
    case AssertionKind::Malformed:
      out = WriteLpar(ctx, "assert_malformed", out);
      ctx.Indent();
      ctx.Newline();
      out = Write(ctx, get<ModuleAssertion>(value.desc), out);
      ctx.Dedent();
      break;

    case AssertionKind::Invalid:
      out = WriteLpar(ctx, "assert_invalid", out);
      ctx.Indent();
      ctx.Newline();
      out = Write(ctx, get<ModuleAssertion>(value.desc), out);
      ctx.Dedent();
      break;

    case AssertionKind::Unlinkable:
      out = WriteLpar(ctx, "assert_unlinkable", out);
      ctx.Indent();
      ctx.Newline();
      out = Write(ctx, get<ModuleAssertion>(value.desc), out);
      ctx.Dedent();
      break;

    case AssertionKind::ActionTrap:
      out = WriteLpar(ctx, "assert_trap", out);
      out = Write(ctx, get<ActionAssertion>(value.desc), out);
      break;

    case AssertionKind::Return:
      out = WriteLpar(ctx, "assert_return", out);
      out = Write(ctx, get<ReturnAssertion>(value.desc), out);
      break;

    case AssertionKind::ModuleTrap:
      out = WriteLpar(ctx, "assert_trap", out);
      ctx.Indent();
      ctx.Newline();
      out = Write(ctx, get<ModuleAssertion>(value.desc), out);
      ctx.Dedent();
      break;

    case AssertionKind::Exhaustion:
      out = WriteLpar(ctx, "assert_exhaustion", out);
      out = Write(ctx, get<ActionAssertion>(value.desc), out);
      break;
  }
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Register& value, Iterator out) {
  out = WriteLpar(ctx, "register", out);
  out = Write(ctx, value.name, out);
  out = Write(ctx, value.module, out);
  out = WriteRpar(ctx, out);
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Command& value, Iterator out) {
  switch (value.kind()) {
    case CommandKind::ScriptModule:
      out = Write(ctx, value.script_module(), out);
      break;

    case CommandKind::Register:
      out = Write(ctx, value.register_(), out);
      break;

    case CommandKind::Action:
      out = Write(ctx, value.action(), out);
      break;

    case CommandKind::Assertion:
      out = Write(ctx, value.assertion(), out);
      break;
  }
  ctx.Newline();
  return out;
}

template <typename Iterator>
Iterator Write(WriteCtx& ctx, const Script& values, Iterator out) {
  return WriteVector(ctx, values, out);
}

}  // namespace wasp::text

#endif  // WASP_TEXT_WRITE_H_

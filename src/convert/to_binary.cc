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

#include "wasp/convert/to_binary.h"

#include <cassert>

#include "wasp/binary/encoding.h"
#include "wasp/binary/write.h"

namespace wasp::convert {

BinCtx::BinCtx(const Features& features) : features{features} {}

string_view BinCtx::Add(std::string str) {
  strings.push_back(std::make_unique<std::string>(str));
  return string_view{*strings.back()};
}

SpanU8 BinCtx::Add(Buffer buffer) {
  buffers.push_back(std::make_unique<Buffer>(std::move(buffer)));
  return SpanU8{*buffers.back()};
}

auto ToBinary(BinCtx& ctx, const At<text::HeapType>& value)
    -> At<binary::HeapType> {
  if (value->is_heap_kind()) {
    return At{value.loc(), binary::HeapType{value->heap_kind()}};
  } else {
    assert(value->is_var());
    return At{value.loc(), binary::HeapType{ToBinary(ctx, value->var())}};
  }
}

auto ToBinary(BinCtx& ctx, const At<text::RefType>& value)
    -> At<binary::RefType> {
  return At{value.loc(),
            binary::RefType{ToBinary(ctx, value->heap_type), value->null}};
}

auto ToBinary(BinCtx& ctx, const At<text::ReferenceType>& value)
    -> At<binary::ReferenceType> {
  if (value->is_reference_kind()) {
    return At{value.loc(), binary::ReferenceType{value->reference_kind()}};
  } else {
    assert(value->is_ref());
    return At{value.loc(), binary::ReferenceType{ToBinary(ctx, value->ref())}};
  }
}

auto ToBinary(BinCtx& ctx, const At<text::Rtt>& value) -> At<binary::Rtt> {
  return At{value.loc(), binary::Rtt{value->depth, ToBinary(ctx, value->type)}};
}

auto ToBinary(BinCtx& ctx, const At<text::ValueType>& value)
    -> At<binary::ValueType> {
  if (value->is_numeric_type()) {
    return At{value.loc(), binary::ValueType{value->numeric_type()}};
  } else if (value->is_reference_type()) {
    return At{value.loc(),
              binary::ValueType{ToBinary(ctx, value->reference_type())}};
  } else {
    assert(value->is_rtt());
    return At{value.loc(), binary::ValueType{ToBinary(ctx, value->rtt())}};
  }
}

auto ToBinary(BinCtx& ctx, const text::ValueTypeList& values)
    -> binary::ValueTypeList {
  binary::ValueTypeList result;
  for (auto& value : values) {
    result.push_back(ToBinary(ctx, value));
  }
  return result;
}

auto ToBinary(BinCtx& ctx, const At<text::StorageType>& value)
    -> At<binary::StorageType> {
  if (value->is_value_type()) {
    return At{value.loc(),
              binary::StorageType{ToBinary(ctx, value->value_type())}};
  } else {
    assert(value->is_packed_type());
    return At{value.loc(), binary::StorageType{value->packed_type()}};
  }
}

auto ToBinary(BinCtx& ctx, const At<text::Text>& value) -> At<string_view> {
  return At{value.loc(), ctx.Add(value->ToString())};
}

auto ToBinary(BinCtx& ctx, const At<text::Var>& value) -> At<Index> {
  assert(value->is_index());
  return At{value.loc(), value->index()};
}

auto ToBinary(BinCtx& ctx, const OptAt<text::Var>& var) -> At<Index> {
  assert(var.has_value());
  return ToBinary(ctx, var.value());
}

auto ToBinary(BinCtx& ctx, const OptAt<text::Var>& var, Index default_index)
    -> At<Index> {
  if (var.has_value()) {
    return ToBinary(ctx, var.value());
  } else {
    return At{default_index};
  }
}

auto ToBinary(BinCtx& ctx, const text::VarList& vars) -> binary::IndexList {
  binary::IndexList result;
  for (auto var : vars) {
    result.push_back(ToBinary(ctx, var));
  }
  return result;
}

// Section 1: Type
auto ToBinary(BinCtx& ctx, const At<text::FunctionType>& value)
    -> At<binary::FunctionType> {
  return At{value.loc(), binary::FunctionType{ToBinary(ctx, value->params),
                                              ToBinary(ctx, value->results)}};
}

auto ToBinary(BinCtx& ctx, const text::BoundValueTypeList& values)
    -> binary::ValueTypeList {
  binary::ValueTypeList result;
  for (auto& value : values) {
    result.push_back(ToBinary(ctx, value->type));
  }
  return result;
}

auto ToBinary(BinCtx& ctx, const At<text::FieldType>& value)
    -> At<binary::FieldType> {
  return At{value.loc(),
            binary::FieldType{ToBinary(ctx, value->type), value->mut}};
}

auto ToBinary(BinCtx& ctx, const text::FieldTypeList& values)
    -> binary::FieldTypeList {
  binary::FieldTypeList result;
  for (auto& value : values) {
    result.push_back(ToBinary(ctx, value));
  }
  return result;
}

auto ToBinary(BinCtx& ctx, const At<text::StructType>& value)
    -> At<binary::StructType> {
  return At{value.loc(), binary::StructType{ToBinary(ctx, value->fields)}};
}

auto ToBinary(BinCtx& ctx, const At<text::ArrayType>& value)
    -> At<binary::ArrayType> {
  return At{value.loc(), binary::ArrayType{ToBinary(ctx, value->field)}};
}

auto ToBinary(BinCtx& ctx, const At<text::DefinedType>& value)
    -> At<binary::DefinedType> {
  if (value->is_function_type()) {
    return At{value.loc(),
              binary::DefinedType{
                  At{value->function_type().loc(),
                     binary::FunctionType{
                         ToBinary(ctx, value->function_type()->params),
                         ToBinary(ctx, value->function_type()->results)}}}};
  } else if (value->is_struct_type()) {
    return At{value.loc(),
              binary::DefinedType{ToBinary(ctx, value->struct_type())}};
  } else {
    assert(value->is_array_type());
    return At{value.loc(),
              binary::DefinedType{ToBinary(ctx, value->array_type())}};
  }
}

// Section 2: Import
auto ToBinary(BinCtx& ctx, const At<text::Import>& value)
    -> At<binary::Import> {
  auto module = At{value->module.loc(), ToBinary(ctx, value->module)};
  auto name = At{value->name.loc(), ToBinary(ctx, value->name)};

  switch (value->kind()) {
    case ExternalKind::Function: {
      auto& desc = value->function_desc();
      return At{value.loc(), binary::Import{module, name,
                                            At{desc.type_use->loc(),
                                               ToBinary(ctx, desc.type_use)}}};
    }

    case ExternalKind::Table: {
      auto& desc = value->table_desc();
      return At{value.loc(),
                binary::Import{module, name,
                               At{desc.type.loc(), ToBinary(ctx, desc.type)}}};
    }

    case ExternalKind::Memory: {
      auto& desc = value->memory_desc();
      return At{value.loc(),
                binary::Import{module, name, At{desc.type.loc(), desc.type}}};
    }

    case ExternalKind::Global: {
      auto& desc = value->global_desc();
      return At{value.loc(),
                binary::Import{module, name,
                               At{desc.type.loc(), ToBinary(ctx, desc.type)}}};
    }

    case ExternalKind::Tag: {
      auto& desc = value->tag_desc();
      return At{value.loc(),
                binary::Import{module, name,
                               At{desc.type.loc(), ToBinary(ctx, desc.type)}}};
    }

    default:
      WASP_UNREACHABLE();
  }
}

// Section 2: Function
auto ToBinary(BinCtx& ctx, const At<text::Function>& value)
    -> OptAt<binary::Function> {
  if (value->import) {
    return nullopt;
  }
  return At{value.loc(), binary::Function{ToBinary(ctx, value->desc.type_use)}};
}

// Section 4: Table
auto ToBinary(BinCtx& ctx, const At<text::TableType>& value)
    -> At<binary::TableType> {
  return At{value.loc(),
            binary::TableType{value->limits, ToBinary(ctx, value->elemtype)}};
}

auto ToBinary(BinCtx& ctx, const At<text::Table>& value)
    -> OptAt<binary::Table> {
  if (value->import) {
    return nullopt;
  }
  return At{value.loc(), binary::Table{ToBinary(ctx, value->desc.type)}};
}

// Section 5: Memory
auto ToBinary(BinCtx& ctx, const At<text::Memory>& value)
    -> OptAt<binary::Memory> {
  if (value->import) {
    return nullopt;
  }
  return At{value.loc(), binary::Memory{value->desc.type}};
}

// Section 6: Global
auto ToBinary(BinCtx& ctx, const At<text::GlobalType>& value)
    -> At<binary::GlobalType> {
  return At{value.loc(),
            binary::GlobalType{ToBinary(ctx, value->valtype), value->mut}};
}

auto ToBinary(BinCtx& ctx, const At<text::ConstantExpression>& value)
    -> At<binary::ConstantExpression> {
  return At{value.loc(),
            binary::ConstantExpression{ToBinary(ctx, value->instructions)}};
}

auto ToBinary(BinCtx& ctx, const At<text::Global>& value)
    -> OptAt<binary::Global> {
  if (value->import) {
    return nullopt;
  }
  assert(value->init.has_value());
  return At{value.loc(), binary::Global{ToBinary(ctx, value->desc.type),
                                        ToBinary(ctx, *value->init)}};
}

// Section 7: Export
auto ToBinary(BinCtx& ctx, const At<text::Export>& value)
    -> At<binary::Export> {
  return At{value.loc(), binary::Export{value->kind, ToBinary(ctx, value->name),
                                        ToBinary(ctx, value->var)}};
}

// Section 8: Start
auto ToBinary(BinCtx& ctx, const At<text::Start>& value) -> At<binary::Start> {
  return At{value.loc(), binary::Start{ToBinary(ctx, value->var)}};
}

auto ToBinary(BinCtx& ctx, const At<text::ElementExpression>& value)
    -> At<binary::ElementExpression> {
  return At{value.loc(),
            binary::ElementExpression{ToBinary(ctx, value->instructions)}};
}

auto ToBinary(BinCtx& ctx, const text::ElementExpressionList& value)
    -> binary::ElementExpressionList {
  binary::ElementExpressionList result;
  for (auto&& elemexpr : value) {
    result.push_back(ToBinary(ctx, elemexpr));
  }
  return result;
}

auto ToBinary(BinCtx& ctx, const text::ElementList& value)
    -> binary::ElementList {
  if (holds_alternative<text::ElementListWithExpressions>(value)) {
    auto&& elements = get<text::ElementListWithExpressions>(value);
    return binary::ElementListWithExpressions{ToBinary(ctx, elements.elemtype),
                                              ToBinary(ctx, elements.list)};
  } else {
    auto&& elements = get<text::ElementListWithVars>(value);
    return binary::ElementListWithIndexes{elements.kind,
                                          ToBinary(ctx, elements.list)};
  }
}

auto ToBinary(BinCtx& ctx, const At<text::ElementSegment>& value)
    -> At<binary::ElementSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(),
              binary::ElementSegment{ToBinary(ctx, value->table, 0),
                                     ToBinary(ctx, *value->offset),
                                     ToBinary(ctx, value->elements)}};
  } else {
    return At{value.loc(), binary::ElementSegment{
                               value->type, ToBinary(ctx, value->elements)}};
  }
}

// Section 10: Code
auto ToBinary(BinCtx& ctx, const At<text::BlockImmediate>& value)
    -> At<binary::BlockType> {
  if (value->type.IsInlineType()) {
    auto inline_type = value->type.GetInlineType();
    if (!inline_type) {
      return At{value.loc(), binary::BlockType{binary::VoidType{}}};
    }
    return At{value.loc(), binary::BlockType{ToBinary(ctx, *inline_type)}};
  } else {
    auto block_type_index = ToBinary(ctx, value->type.type_use);
    assert(block_type_index < 0x8000'0000);
    return At{value.loc(), binary::BlockType{block_type_index}};
  }
}

auto ToBinary(BinCtx& ctx, const At<text::BrOnCastImmediate>& value)
    -> At<binary::BrOnCastImmediate> {
  return At{value.loc(),
            binary::BrOnCastImmediate{ToBinary(ctx, value->target),
                                      ToBinary(ctx, value->types)}};
}

auto ToBinary(BinCtx& ctx, const At<text::BrTableImmediate>& value)
    -> At<binary::BrTableImmediate> {
  return At{value.loc(),
            binary::BrTableImmediate{ToBinary(ctx, value->targets),
                                     ToBinary(ctx, value->default_target)}};
}

auto ToBinary(BinCtx& ctx, const At<text::CallIndirectImmediate>& value)
    -> At<binary::CallIndirectImmediate> {
  return At{value.loc(),
            binary::CallIndirectImmediate{ToBinary(ctx, value->type.type_use),
                                          ToBinary(ctx, value->table, 0)}};
}

auto ToBinary(BinCtx& ctx, const At<text::CopyImmediate>& value)
    -> At<binary::CopyImmediate> {
  return At{value.loc(), binary::CopyImmediate{ToBinary(ctx, value->dst, 0),
                                               ToBinary(ctx, value->src, 0)}};
}

auto ToBinary(BinCtx& ctx, const At<text::FuncBindImmediate>& value)
    -> At<binary::FuncBindImmediate> {
  return At{value.loc(),
            binary::FuncBindImmediate{ToBinary(ctx, value->type_use)}};
}

auto ToBinary(BinCtx& ctx, const At<text::HeapType2Immediate>& value)
    -> At<binary::HeapType2Immediate> {
  return At{value.loc(),
            binary::HeapType2Immediate{ToBinary(ctx, value->parent),
                                       ToBinary(ctx, value->child)}};
}

auto ToBinary(BinCtx& ctx, const At<text::InitImmediate>& value)
    -> At<binary::InitImmediate> {
  return At{value.loc(), binary::InitImmediate{ToBinary(ctx, value->segment),
                                               ToBinary(ctx, value->dst, 0)}};
}

auto ToBinary(BinCtx& ctx, const At<text::LetImmediate>& value)
    -> At<binary::LetImmediate> {
  return At{value.loc(),
            binary::LetImmediate{ToBinary(ctx, value->block),
                                 ToBinaryLocalsList(ctx, value->locals)}};
}

u32 GetNaturalAlignment(Opcode opcode) {
  switch (opcode) {
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw8CmpxchgU:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw8XchgU:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicStore8:
    case Opcode::I32Load8S:
    case Opcode::I32Load8U:
    case Opcode::I32Store8:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw8CmpxchgU:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw8XchgU:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicStore8:
    case Opcode::I64Load8S:
    case Opcode::I64Load8U:
    case Opcode::I64Store8:
    case Opcode::V128Load8Splat:
    case Opcode::V128Load8Lane:
    case Opcode::V128Store8Lane:
      return 1;

    case Opcode::I32AtomicLoad16U:
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I32AtomicRmw16CmpxchgU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I32AtomicRmw16XchgU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I32AtomicStore16:
    case Opcode::I32Load16S:
    case Opcode::I32Load16U:
    case Opcode::I32Store16:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicStore16:
    case Opcode::I64Load16S:
    case Opcode::I64Load16U:
    case Opcode::I64Store16:
    case Opcode::V128Load16Splat:
    case Opcode::V128Load16Lane:
    case Opcode::V128Store16Lane:
      return 2;

    case Opcode::F32Load:
    case Opcode::F32Store:
    case Opcode::I32AtomicLoad:
    case Opcode::I32AtomicRmwAdd:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I32AtomicRmwCmpxchg:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I32AtomicRmwXchg:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I32AtomicStore:
    case Opcode::I32Load:
    case Opcode::I32Store:
    case Opcode::I64AtomicLoad32U:
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I64AtomicRmw32CmpxchgU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I64AtomicRmw32XchgU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I64AtomicStore32:
    case Opcode::I64Load32S:
    case Opcode::I64Load32U:
    case Opcode::I64Store32:
    case Opcode::MemoryAtomicNotify:
    case Opcode::MemoryAtomicWait32:
    case Opcode::V128Load32Splat:
    case Opcode::V128Load32Zero:
    case Opcode::V128Load32Lane:
    case Opcode::V128Store32Lane:
      return 4;

    case Opcode::F64Load:
    case Opcode::F64Store:
    case Opcode::I64AtomicLoad:
    case Opcode::I64AtomicRmwAdd:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I64AtomicRmwCmpxchg:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I64AtomicRmwXchg:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I64AtomicStore:
    case Opcode::I64Load:
    case Opcode::I64Store:
    case Opcode::MemoryAtomicWait64:
    case Opcode::V128Load16X4S:
    case Opcode::V128Load16X4U:
    case Opcode::V128Load32X2S:
    case Opcode::V128Load32X2U:
    case Opcode::V128Load64Splat:
    case Opcode::V128Load64Zero:
    case Opcode::V128Load8X8S:
    case Opcode::V128Load8X8U:
    case Opcode::V128Load64Lane:
    case Opcode::V128Store64Lane:
      return 8;

    case Opcode::V128Load:
    case Opcode::V128Store:
      return 16;

    default:
      assert(false);
      return 0;
  }
}

u32 GetAlignLog2(u32 align) {
  // Must be power-of-two.
  assert(align != 0 && (align & (align - 1)) == 0);
  u32 align_log2 = 0;
  while (align != 0) {
    align >>= 1;
    align_log2++;
  }
  return align_log2 - 1;
}

auto ToBinary(BinCtx& ctx,
              const At<text::MemArgImmediate>& value,
              u32 natural_align) -> At<binary::MemArgImmediate> {
  auto align = value->align
                   ? At{value->align->loc(), GetAlignLog2(*value->align)}
                   : At{GetAlignLog2(natural_align)};
  auto offset = value->offset ? *value->offset : At{u32{0}};
  OptAt<Index> memory_index =
      value->memory ? OptAt<Index>{ToBinary(ctx, *value->memory)} : nullopt;
  return At{value.loc(), binary::MemArgImmediate{align, offset, memory_index}};
}

auto ToBinary(BinCtx& ctx, const At<text::MemOptImmediate>& value)
    -> At<binary::MemOptImmediate> {
  At<Index> memory_index =
      value->memory ? ToBinary(ctx, *value->memory) : At{Index{0}};
  return At{value.loc(), binary::MemOptImmediate{memory_index}};
}

auto ToBinary(BinCtx& ctx, const At<text::RttSubImmediate>& value)
    -> At<binary::RttSubImmediate> {
  return At{value.loc(),
            binary::RttSubImmediate{value->depth, ToBinary(ctx, value->types)}};
}

auto ToBinary(BinCtx& ctx, const At<text::StructFieldImmediate>& value)
    -> At<binary::StructFieldImmediate> {
  return At{value.loc(),
            binary::StructFieldImmediate{ToBinary(ctx, value->struct_),
                                         ToBinary(ctx, value->field)}};
}

auto ToBinary(BinCtx& ctx,
              const At<text::SimdMemoryLaneImmediate>& value,
              u32 natural_align) -> At<binary::SimdMemoryLaneImmediate> {
  return At{value.loc(),
            binary::SimdMemoryLaneImmediate{
                ToBinary(ctx, value->memarg, natural_align), value->lane}};
}

auto ToBinary(BinCtx& ctx, const At<text::Instruction>& value)
    -> At<binary::Instruction> {
  switch (value->kind()) {
    case text::InstructionKind::None:
      return At{value.loc(), binary::Instruction{value->opcode}};

    case text::InstructionKind::S32:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->s32_immediate()}};

    case text::InstructionKind::S64:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->s64_immediate()}};

    case text::InstructionKind::F32:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->f32_immediate()}};

    case text::InstructionKind::F64:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->f64_immediate()}};

    case text::InstructionKind::V128:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->v128_immediate()}};

    case text::InstructionKind::Var:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->var_immediate())}};

    case text::InstructionKind::Block:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->block_immediate())}};

    case text::InstructionKind::BrTable:
      return At{value.loc(),
                binary::Instruction{
                    value->opcode, ToBinary(ctx, value->br_table_immediate())}};

    case text::InstructionKind::CallIndirect:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(ctx, value->call_indirect_immediate())}};

    case text::InstructionKind::Copy:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->copy_immediate())}};

    case text::InstructionKind::Init:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->init_immediate())}};

    case text::InstructionKind::Let:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->let_immediate())}};

    case text::InstructionKind::MemArg:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(ctx, value->mem_arg_immediate(),
                                       GetNaturalAlignment(*value->opcode))}};

    case text::InstructionKind::MemOpt:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->mem_opt_immediate())}};

    case text::InstructionKind::HeapType:
      return At{value.loc(), binary::Instruction{
                                 value->opcode,
                                 ToBinary(ctx, value->heap_type_immediate())}};

    case text::InstructionKind::Select:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              At{value->select_immediate().loc(),
                                 ToBinary(ctx, *value->select_immediate())}}};

    case text::InstructionKind::Shuffle:
      return At{value.loc(),
                binary::Instruction{value->opcode, value->shuffle_immediate()}};

    case text::InstructionKind::SimdLane:
      return At{value.loc(), binary::Instruction{value->opcode,
                                                 value->simd_lane_immediate()}};

    case text::InstructionKind::SimdMemoryLane:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(ctx, value->simd_memory_lane_immediate(),
                                       GetNaturalAlignment(*value->opcode))}};

    case text::InstructionKind::FuncBind:
      return At{value.loc(), binary::Instruction{
                                 value->opcode,
                                 ToBinary(ctx, value->func_bind_immediate())}};

    case text::InstructionKind::BrOnCast:
      return At{value.loc(), binary::Instruction{
                                 value->opcode,
                                 ToBinary(ctx, value->br_on_cast_immediate())}};

    case text::InstructionKind::HeapType2:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(ctx, value->heap_type_2_immediate())}};

    case text::InstructionKind::RttSub:
      return At{value.loc(),
                binary::Instruction{value->opcode,
                                    ToBinary(ctx, value->rtt_sub_immediate())}};

    case text::InstructionKind::StructField:
      return At{
          value.loc(),
          binary::Instruction{value->opcode,
                              ToBinary(ctx, value->struct_field_immediate())}};

    default:
      WASP_UNREACHABLE();
  }
}

auto ToBinary(BinCtx& ctx, const text::InstructionList& value)
    -> binary::InstructionList {
  binary::InstructionList result;
  for (auto&& instr : value) {
    result.push_back(ToBinary(ctx, instr));
  }
  return result;
}

auto ToBinaryUnpackedExpression(BinCtx& ctx,
                                const At<text::InstructionList>& value)
    -> At<binary::UnpackedExpression> {
  return At{value.loc(), binary::UnpackedExpression{ToBinary(ctx, value)}};
}

auto ToBinaryLocalsList(BinCtx& ctx, const At<text::BoundValueTypeList>& value)
    -> At<binary::LocalsList> {
  binary::LocalsList result;
  optional<binary::ValueType> local_type;
  for (auto&& bound_local : *value) {
    auto bound_local_type = ToBinary(ctx, bound_local->type);
    if (local_type && bound_local_type == *local_type) {
      assert(!result.empty());
      // TODO: Combine locations?
      (*result.back()->count)++;
    } else {
      result.push_back(binary::Locals{1, bound_local_type});
      local_type = bound_local_type;
    }
  }
  return At{value.loc(), result};
}

auto ToBinaryCode(BinCtx& ctx, const At<text::Function>& value)
    -> OptAt<binary::UnpackedCode> {
  if (value->import) {
    return nullopt;
  }

  return At{value.loc(),
            binary::UnpackedCode{
                ToBinaryLocalsList(ctx, value->locals),
                ToBinaryUnpackedExpression(ctx, value->instructions)}};
}

// Section 11: Data
auto ToBinary(BinCtx& ctx, const At<text::DataItemList>& value) -> SpanU8 {
  Buffer buffer;
  for (auto&& data_item : *value) {
    data_item->AppendToBuffer(buffer);
  }
  return ctx.Add(buffer);
}

auto ToBinary(BinCtx& ctx, const At<text::DataSegment>& value)
    -> At<binary::DataSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(), binary::DataSegment{ToBinary(ctx, value->memory, 0),
                                               ToBinary(ctx, *value->offset),
                                               ToBinary(ctx, value->data)}};
  } else {
    return At{value.loc(), binary::DataSegment{ToBinary(ctx, value->data)}};
  }
}

// Section 12: DataCount

// Section 13: Tag
auto ToBinary(BinCtx& ctx, const At<text::TagType>& value)
    -> At<binary::TagType> {
  return At{
      value.loc(),
      binary::TagType{value->attribute, ToBinary(ctx, value->type.type_use)}};
}

auto ToBinary(BinCtx& ctx, const At<text::Tag>& value)
    -> OptAt<binary::Tag> {
  if (value->import) {
    return nullopt;
  }
  return At{value.loc(), binary::Tag{ToBinary(ctx, value->desc.type)}};
}

// Module
auto ToBinary(BinCtx& ctx, const At<text::Module>& value)
    -> At<binary::Module> {
  binary::Module result;

  auto push_back_opt = [](auto& vec, auto&& item) {
    if (item) {
      vec.push_back(*item);
    }
  };

  for (auto&& item : *value) {
    switch (item.kind()) {
      case text::ModuleItemKind::DefinedType:
        result.types.push_back(ToBinary(ctx, item.defined_type()));
        break;

      case text::ModuleItemKind::Import:
        result.imports.push_back(ToBinary(ctx, item.import()));
        break;

      case text::ModuleItemKind::Function: {
        auto&& function = item.function();
        push_back_opt(result.functions, ToBinary(ctx, function));
        push_back_opt(result.codes, ToBinaryCode(ctx, function));
        break;
      }

      case text::ModuleItemKind::Table:
        push_back_opt(result.tables, ToBinary(ctx, item.table()));
        break;

      case text::ModuleItemKind::Memory:
        push_back_opt(result.memories, ToBinary(ctx, item.memory()));
        break;

      case text::ModuleItemKind::Global:
        push_back_opt(result.globals, ToBinary(ctx, item.global()));
        break;

      case text::ModuleItemKind::Export:
        result.exports.push_back(ToBinary(ctx, item.export_()));
        break;

      case text::ModuleItemKind::Start:
        // This will overwrite an existing Start section, if any. That
        // shouldn't happen, since reading multiple start sections means the
        // text is malformed.
        result.start = ToBinary(ctx, item.start());
        break;

      case text::ModuleItemKind::ElementSegment:
        result.element_segments.push_back(
            ToBinary(ctx, item.element_segment()));
        break;

      case text::ModuleItemKind::DataSegment:
        result.data_segments.push_back(ToBinary(ctx, item.data_segment()));
        if (ctx.features.bulk_memory_enabled()) {
          result.data_count =
              binary::DataCount{Index(result.data_segments.size())};
        }
        break;

      case text::ModuleItemKind::Tag:
        push_back_opt(result.tags, ToBinary(ctx, item.tag()));
        break;
    }
  }
  return At{value.loc(), result};
}

}  // namespace wasp::convert

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

#include "wasp/convert/to_text.h"

#include <cassert>

#include "wasp/base/enumerate.h"
#include "wasp/base/macros.h"

namespace wasp::convert {

std::string EncodeAsText(string_view str) {
  const char kHexDigit[] = "0123456789abcdef";
  std::string text = "\"";
  for (u8 byte : str) {
    if (byte == '"') {
      text += "\\\"";
    } else if (byte == '\\') {
      text += "\\\\";
    } else if (byte >= 32 && byte <= 127) {
      text += byte;
    } else if (byte == '\t') {
      text += "\\t";
    } else if (byte == '\n') {
      text += "\\n";
    } else if (byte == '\r') {
      text += "\\r";
    } else {
      text += "\\";
      text += kHexDigit[byte >> 4];
      text += kHexDigit[byte & 15];
    }
  }
  text += "\"";
  return text;
}

text::Text TextCtx::Add(string_view str) {
  strings.push_back(std::make_unique<std::string>(EncodeAsText(str)));
  return text::Text{string_view{*strings.back()}, static_cast<u32>(str.size())};
}

// Helpers.
auto ToText(TextCtx& ctx, const At<binary::HeapType>& value)
    -> At<text::HeapType> {
  if (value->is_heap_kind()) {
    return At{value.loc(), text::HeapType{value->heap_kind()}};
  } else {
    assert(value->is_index());
    return At{value.loc(), text::HeapType{ToText(ctx, value->index())}};
  }
}

auto ToText(TextCtx& ctx, const At<binary::RefType>& value)
    -> At<text::RefType> {
  return At{value.loc(),
            text::RefType{ToText(ctx, value->heap_type), value->null}};
}

auto ToText(TextCtx& ctx, const At<binary::ReferenceType>& value)
    -> At<text::ReferenceType> {
  if (value->is_reference_kind()) {
    return At{value.loc(), text::ReferenceType{value->reference_kind()}};
  } else {
    assert(value->is_ref());
    return At{value.loc(), text::ReferenceType{ToText(ctx, value->ref())}};
  }
}

auto ToText(TextCtx& ctx, const At<binary::Rtt>& value) -> At<text::Rtt> {
  return At{value.loc(), text::Rtt{value->depth, ToText(ctx, value->type)}};
}

auto ToText(TextCtx& ctx, const At<binary::ValueType>& value)
    -> At<text::ValueType> {
  if (value->is_numeric_type()) {
    return At{value.loc(), text::ValueType{value->numeric_type()}};
  } else if (value->is_reference_type()) {
    return At{value.loc(),
              text::ValueType{ToText(ctx, value->reference_type())}};
  } else {
    assert(value->is_rtt());
    return At{value.loc(), text::ValueType{ToText(ctx, value->rtt())}};
  }
}

auto ToText(TextCtx& ctx, const binary::ValueTypeList& values)
    -> text::ValueTypeList {
  text::ValueTypeList result;
  for (auto&& value : values) {
    result.push_back(ToText(ctx, value));
  }
  return result;
}

auto ToTextBound(TextCtx& ctx, const binary::ValueTypeList& values)
    -> At<text::BoundValueTypeList> {
  text::BoundValueTypeList result;
  for (auto&& value : values) {
    result.push_back(text::BoundValueType{nullopt,  // TODO name
                                          ToText(ctx, value)});
  }
  return result;
}

auto ToText(TextCtx& ctx, const At<binary::StorageType>& value)
    -> At<text::StorageType> {
  if (value->is_value_type()) {
    return At{value.loc(), text::StorageType{ToText(ctx, value->value_type())}};
  } else {
    assert(value->is_packed_type());
    return At{value.loc(), text::StorageType{value->packed_type()}};
  }
}

auto ToText(TextCtx& ctx, const At<string_view>& value) -> At<text::Text> {
  return At{value.loc(), ctx.Add(*value)};
}

auto ToText(TextCtx& ctx, const At<Index>& value) -> At<text::Var> {
  return At{value.loc(), text::Var{value.value()}};
}

auto ToText(TextCtx& ctx, const OptAt<Index>& value) -> OptAt<text::Var> {
  if (!value) {
    return nullopt;
  }
  return ToText(ctx, *value);
}

auto ToText(TextCtx& ctx, const binary::IndexList& values) -> text::VarList {
  text::VarList result;
  for (auto value : values) {
    result.push_back(ToText(ctx, value));
  }
  return result;
}

auto ToText(TextCtx& ctx, const At<binary::FunctionType>& value)
    -> At<text::FunctionType> {
  return At{value.loc(), text::FunctionType{ToText(ctx, value->param_types),
                                            ToText(ctx, value->result_types)}};
}

// Section 1: Type
auto ToTextBound(TextCtx& ctx, const At<binary::FunctionType>& value)
    -> At<text::BoundFunctionType> {
  return At{value.loc(),
            text::BoundFunctionType{ToTextBound(ctx, value->param_types),
                                    ToText(ctx, value->result_types)}};
}

auto ToText(TextCtx& ctx, const At<binary::FieldType>& value)
    -> At<text::FieldType> {
  return At{value.loc(),
            text::FieldType{nullopt,  // TODO(name)
                            ToText(ctx, value->type), value->mut}};
}

auto ToText(TextCtx& ctx, const binary::FieldTypeList& values)
    -> text::FieldTypeList {
  text::FieldTypeList result;
  for (auto&& value : values) {
    result.push_back(ToText(ctx, value));
  }
  return result;
}

auto ToText(TextCtx& ctx, const At<binary::StructType>& value)
    -> At<text::StructType> {
  return At{value.loc(), text::StructType{ToText(ctx, value->fields)}};
}

auto ToText(TextCtx& ctx, const At<binary::ArrayType>& value)
    -> At<text::ArrayType> {
  return At{value.loc(), text::ArrayType{ToText(ctx, value->field)}};
}

auto ToText(TextCtx& ctx, const At<binary::DefinedType>& value)
    -> At<text::DefinedType> {
  if (value->is_function_type()) {
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToTextBound(ctx, value->function_type())}};
  } else if (value->is_struct_type()) {
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToText(ctx, value->struct_type())}};
  } else {
    assert(value->is_array_type());
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToText(ctx, value->array_type())}};
  }
}

// Section 2: Import
auto ToText(TextCtx& ctx, const At<binary::Import>& value)
    -> At<text::Import> {
  auto module = ToText(ctx, value->module);
  auto name = ToText(ctx, value->name);

  switch (value->kind()) {
    case ExternalKind::Function:
      return At{value.loc(),
                text::Import{module, name,
                             text::FunctionDesc{
                                 nullopt,  // TODO(name)
                                 ToText(ctx, value->index()),
                                 {}  // Unresolved bound function type
                             }}};

    case ExternalKind::Table:
      return At{
          value.loc(),
          text::Import{module, name,
                       text::TableDesc{nullopt,  // TODO(name)
                                       ToText(ctx, value->table_type())}}};

    case ExternalKind::Memory:
      return At{value.loc(),
                text::Import{module, name,
                             text::MemoryDesc{nullopt,  // TODO(name)
                                              value->memory_type()}}};

    case ExternalKind::Global:
      return At{
          value.loc(),
          text::Import{module, name,
                       text::GlobalDesc{nullopt,  // TODO(name)
                                        ToText(ctx, value->global_type())}}};

    case ExternalKind::Event:
      return At{
          value.loc(),
          text::Import{module, name,
                       text::EventDesc{nullopt,  // TODO(name)
                                       ToText(ctx, value->event_type())}}};

    default:
      WASP_UNREACHABLE();
  }
}

// Section 3: Function
auto ToText(TextCtx& ctx, const At<binary::Function>& value)
    -> At<text::Function> {
  return At{value.loc(),
            text::Function{text::FunctionDesc{
                               nullopt,  // TODO(name)
                               ToText(ctx, value->type_index),
                               {}  // Unresolved bound function type
                           },
                           {},
                           {},
                           nullopt,
                           {}}};
}

// Section 4: Table
auto ToText(TextCtx& ctx, const At<binary::TableType>& value)
    -> At<text::TableType> {
  return At{value.loc(),
            text::TableType{value->limits, ToText(ctx, value->elemtype)}};
}

auto ToText(TextCtx& ctx, const At<binary::Table>& value)
    -> At<text::Table> {
  return At{value.loc(),
            text::Table{text::TableDesc{nullopt,  // TODO(name)
                                        ToText(ctx, value->table_type)},
                        {}}};
}

// Section 5: Memory
auto ToText(TextCtx& ctx, const At<binary::Memory>& value)
    -> At<text::Memory> {
  return At{value.loc(),
            text::Memory{text::MemoryDesc{nullopt,  // TODO(name)
                                          value->memory_type},
                         {}}};
}

// Section 6: Global
auto ToText(TextCtx& ctx, const At<binary::ConstantExpression>& value)
    -> At<text::ConstantExpression> {
  return At{value.loc(),
            text::ConstantExpression{ToText(ctx, value->instructions)}};
}

auto ToText(TextCtx& ctx, const At<binary::GlobalType>& value)
    -> At<text::GlobalType> {
  return At{value.loc(),
            text::GlobalType{ToText(ctx, value->valtype), value->mut}};
}

auto ToText(TextCtx& ctx, const At<binary::Global>& value)
    -> At<text::Global> {
  return At{value.loc(),
            text::Global{text::GlobalDesc{nullopt,  // TODO(name)
                                          ToText(ctx, value->global_type)},
                         ToText(ctx, value->init),
                         {}}};
}

// Section 7: Export
auto ToText(TextCtx& ctx, const At<binary::Export>& value) -> At<text::Export> {
  return At{value.loc(), text::Export{value->kind, ToText(ctx, value->name),
                                      ToText(ctx, value->index)}};
}

// Section 8: Start
auto ToText(TextCtx& ctx, const At<binary::Start>& value) -> At<text::Start> {
  return At{value.loc(), text::Start{ToText(ctx, value->func_index)}};
}

// Section 9: Elem
auto ToText(TextCtx& ctx, const At<binary::ElementExpression>& value)
    -> At<text::ElementExpression> {
  return At{value.loc(),
            text::ElementExpression{ToText(ctx, value->instructions)}};
}

auto ToText(TextCtx& ctx, const binary::ElementExpressionList& values)
    -> text::ElementExpressionList {
  text::ElementExpressionList result;
  for (auto&& value : values) {
    result.push_back(ToText(ctx, value));
  }
  return result;
}

auto ToText(TextCtx& ctx, const binary::ElementList& value)
    -> text::ElementList {
  if (holds_alternative<binary::ElementListWithExpressions>(value)) {
    auto&& elements = get<binary::ElementListWithExpressions>(value);
    return text::ElementListWithExpressions{ToText(ctx, elements.elemtype),
                                            ToText(ctx, elements.list)};
  } else {
    auto&& elements = get<binary::ElementListWithIndexes>(value);
    return text::ElementListWithVars{elements.kind, ToText(ctx, elements.list)};
  }
}

auto ToText(TextCtx& ctx, const At<binary::ElementSegment>& value)
    -> At<text::ElementSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(),
              text::ElementSegment{nullopt,  // TODO(name)
                                   ToText(ctx, value->table_index),
                                   ToText(ctx, *value->offset),
                                   ToText(ctx, value->elements)}};
  } else {
    return At{value.loc(),
              text::ElementSegment{nullopt,  // TODO(name)
                                   value->type, ToText(ctx, value->elements)}};
  }
}

// Section 10: Code
auto ToText(TextCtx& ctx, const At<binary::BlockType>& value)
    -> At<text::BlockImmediate> {
  if (value->is_value_type()) {
    return At{value.loc(),
              text::BlockImmediate{
                  nullopt,  // TODO(name)
                  text::FunctionTypeUse{
                      nullopt, text::FunctionType{
                                   {}, {ToText(ctx, value->value_type())}}}}};
  } else if (value->is_void()) {
    return At{value.loc(),
              text::BlockImmediate{
                  nullopt,  // TODO(name)
                  text::FunctionTypeUse{nullopt, text::FunctionType{{}, {}}}}};
  } else {
    assert(value->is_index());
    return At{value.loc(),
              text::BlockImmediate{
                  nullopt,  // TODO(name)
                  text::FunctionTypeUse{ToText(ctx, value->index()), {}}}};
  }
}

auto ToText(TextCtx& ctx, const At<binary::BrOnCastImmediate>& value)
    -> At<text::BrOnCastImmediate> {
  return At{value.loc(), text::BrOnCastImmediate{ToText(ctx, value->target),
                                                 ToText(ctx, value->types)}};
}

auto ToText(TextCtx& ctx, const At<binary::BrOnExnImmediate>& value)
    -> At<text::BrOnExnImmediate> {
  return At{value.loc(),
            text::BrOnExnImmediate{ToText(ctx, value->target),
                                   ToText(ctx, value->event_index)}};
}

auto ToText(TextCtx& ctx, const At<binary::BrTableImmediate>& value)
    -> At<text::BrTableImmediate> {
  return At{value.loc(),
            text::BrTableImmediate{ToText(ctx, value->targets),
                                   ToText(ctx, value->default_target)}};
}

auto ToText(TextCtx& ctx, const At<binary::CallIndirectImmediate>& value)
    -> At<text::CallIndirectImmediate> {
  return At{value.loc(),
            text::CallIndirectImmediate{
                ToText(ctx, value->table_index),
                text::FunctionTypeUse{ToText(ctx, value->index), {}}}};
}

auto ToText(TextCtx& ctx, const At<binary::CopyImmediate>& value)
    -> At<text::CopyImmediate> {
  return At{value.loc(), text::CopyImmediate{ToText(ctx, value->dst_index),
                                             ToText(ctx, value->src_index)}};
}

auto ToText(TextCtx& ctx, const At<binary::FuncBindImmediate>& value)
    -> At<text::FuncBindImmediate> {
  return At{value.loc(),
            text::FuncBindImmediate{ToText(ctx, value->index), {}}};
}

auto ToText(TextCtx& ctx, const At<binary::HeapType2Immediate>& value)
    -> At<text::HeapType2Immediate> {
  return At{value.loc(), text::HeapType2Immediate{ToText(ctx, value->parent),
                                                  ToText(ctx, value->child)}};
}

auto ToText(TextCtx& ctx, const At<binary::InitImmediate>& value)
    -> At<text::InitImmediate> {
  return At{value.loc(), text::InitImmediate{ToText(ctx, value->segment_index),
                                             ToText(ctx, value->dst_index)}};
}

auto ToText(TextCtx& ctx, const At<binary::LetImmediate>& value)
    -> At<text::LetImmediate> {
  return At{value.loc(), text::LetImmediate{ToText(ctx, value->block_type),
                                            ToText(ctx, value->locals)}};
}

auto GetAlignPow2(At<u32> align_log2) -> At<u32> {
  // Must be less than 32.
  assert(*align_log2 < 32);
  return At{align_log2.loc(), 1u << align_log2};
}

auto ToText(TextCtx& ctx, const At<binary::MemArgImmediate>& value)
    -> At<text::MemArgImmediate> {
  return At{value.loc(), text::MemArgImmediate{GetAlignPow2(value->align_log2),
                                               value->offset}};
}

auto ToText(TextCtx& ctx, const At<binary::RttSubImmediate>& value)
    -> At<text::RttSubImmediate> {
  return At{value.loc(),
            text::RttSubImmediate{value->depth, ToText(ctx, value->types)}};
}

auto ToText(TextCtx& ctx, const At<binary::StructFieldImmediate>& value)
    -> At<text::StructFieldImmediate> {
  return At{value.loc(), text::StructFieldImmediate{ToText(ctx, value->struct_),
                                                    ToText(ctx, value->field)}};
}

auto ToText(TextCtx& ctx, const At<binary::SimdMemoryLaneImmediate>& value)
    -> At<text::SimdMemoryLaneImmediate> {
  return At{value.loc(), text::SimdMemoryLaneImmediate{
                             ToText(ctx, value->memarg), value->lane}};
}

auto ToText(TextCtx& ctx, const At<binary::Instruction>& value)
    -> At<text::Instruction> {
  switch (value->kind()) {
    case binary::InstructionKind::None:
      return At{value.loc(), text::Instruction{value->opcode}};

    case binary::InstructionKind::S32:
      return At{value.loc(),
                text::Instruction{value->opcode, value->s32_immediate()}};

    case binary::InstructionKind::S64:
      return At{value.loc(),
                text::Instruction{value->opcode, value->s64_immediate()}};

    case binary::InstructionKind::F32:
      return At{value.loc(),
                text::Instruction{value->opcode, value->f32_immediate()}};

    case binary::InstructionKind::F64:
      return At{value.loc(),
                text::Instruction{value->opcode, value->f64_immediate()}};

    case binary::InstructionKind::V128:
      return At{value.loc(),
                text::Instruction{value->opcode, value->v128_immediate()}};

    case binary::InstructionKind::Index:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->index_immediate())}};

    case binary::InstructionKind::BlockType:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->block_type_immediate())}};

    case binary::InstructionKind::BrOnExn:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->br_on_exn_immediate())}};

    case binary::InstructionKind::BrTable:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->br_table_immediate())}};

    case binary::InstructionKind::CallIndirect:
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(ctx, value->call_indirect_immediate())}};

    case binary::InstructionKind::Copy:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->copy_immediate())}};

    case binary::InstructionKind::Init:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->init_immediate())}};

    case binary::InstructionKind::Let:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->let_immediate())}};

    case binary::InstructionKind::MemArg:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->mem_arg_immediate())}};

    case binary::InstructionKind::HeapType:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->heap_type_immediate())}};

    case binary::InstructionKind::Select:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  At{value->select_immediate().loc(),
                                     ToText(ctx, *value->select_immediate())}}};

    case binary::InstructionKind::Shuffle:
      return At{value.loc(),
                text::Instruction{value->opcode, value->shuffle_immediate()}};

    case binary::InstructionKind::SimdLane:
      return At{value.loc(),
                text::Instruction{value->opcode, value->simd_lane_immediate()}};

    case binary::InstructionKind::SimdMemoryLane:
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(ctx, value->simd_memory_lane_immediate())}};

    case binary::InstructionKind::FuncBind:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->func_bind_immediate())}};

    case binary::InstructionKind::BrOnCast:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->br_on_cast_immediate())}};

    case binary::InstructionKind::HeapType2:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->heap_type_2_immediate())}};

    case binary::InstructionKind::RttSub:
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(ctx, value->rtt_sub_immediate())}};

    case binary::InstructionKind::StructField:
      return At{value.loc(), text::Instruction{
                                 value->opcode,
                                 ToText(ctx, value->struct_field_immediate())}};

    default:
      WASP_UNREACHABLE();
  }
}

auto ToText(TextCtx& ctx, const binary::InstructionList& values)
    -> text::InstructionList {
  text::InstructionList result;
  for (auto&& value : values) {
    result.push_back(ToText(ctx, value));
  }
  return result;
}

auto ToText(TextCtx& ctx, const At<binary::UnpackedExpression>& value)
    -> text::InstructionList {
  return ToText(ctx, value->instructions);
}

auto ToText(TextCtx& ctx, const binary::LocalsList& values)
    -> At<text::BoundValueTypeList> {
  text::BoundValueTypeList result;
  for (auto&& locals : values) {
    auto type = ToText(ctx, locals->type);
    for (Index i = 0; i < *locals->count; ++i) {
      result.push_back(text::BoundValueType{nullopt,  // TODO(name)
                                            type});
    }
  }
  return result;
}

auto ToText(TextCtx& ctx,
            const At<binary::UnpackedCode>& value,
            At<text::Function>& function) -> At<text::Function>& {
  function->locals = ToText(ctx, value->locals);
  function->instructions = ToText(ctx, value->body);
  return function;
}

// Section 11: Data
auto ToText(TextCtx& ctx, const At<SpanU8>& value) -> text::DataItemList {
  return text::DataItemList{text::DataItem{ctx.Add(ToStringView(value))}};
}

auto ToText(TextCtx& ctx, const At<binary::DataSegment>& value)
    -> At<text::DataSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(),
              text::DataSegment{nullopt,  // TODO(name)
                                ToText(ctx, value->memory_index),
                                ToText(ctx, *value->offset),
                                ToText(ctx, value->init)}};
  } else {
    return At{value.loc(),
              text::DataSegment{nullopt,  // TODO(name)
                                ToText(ctx, value->init)}};
  }
}

// Section 12: DataCount

// Section 13: Event
auto ToText(TextCtx& ctx, const At<binary::EventType>& value)
    -> At<text::EventType> {
  return At{value.loc(),
            text::EventType{
                value->attribute,
                text::FunctionTypeUse{ToText(ctx, value->type_index), {}}}};
}

auto ToText(TextCtx& ctx, const At<binary::Event>& value) -> At<text::Event> {
  return At{value.loc(),
            text::Event{text::EventDesc{nullopt,  // TODO(name)
                                        ToText(ctx, value->event_type)},
                        nullopt,
                        {}}};
}

// Module
auto ToText(TextCtx& ctx, const At<binary::Module>& value) -> At<text::Module> {
  text::Module module;

  auto do_vector = [&](const auto& items) {
    for (auto&& item : items) {
      module.push_back(At{item.loc(), text::ModuleItem{ToText(ctx, item)}});
    }
  };
  auto do_optional = [&](const auto& maybe_item) {
    if (maybe_item) {
      module.push_back(
          At{maybe_item->loc(), text::ModuleItem{ToText(ctx, *maybe_item)}});
    }
  };

  do_vector(value->types);
  do_vector(value->imports);
  do_vector(value->tables);
  do_vector(value->memories);
  do_vector(value->globals);
  do_vector(value->events);
  do_vector(value->exports);
  do_optional(value->start);
  do_vector(value->element_segments);
  do_vector(value->data_segments);

  // Combine binary::Function and binary::UnpackedCode into text::Function.
  assert(value->functions.size() == value->codes.size());
  for (size_t i = 0; i < value->functions.size(); ++i) {
    At<text::Function> function = ToText(ctx, value->functions[i]);
    module.push_back(At{function.loc(), text::ModuleItem{ToText(
                                            ctx, value->codes[i], function)}});
  }

  return At{value.loc(), module};
}

}  // namespace wasp::convert

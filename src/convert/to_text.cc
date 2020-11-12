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
  for (auto byte : str) {
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

text::Text TextContext::Add(string_view str) {
  strings.push_back(std::make_unique<std::string>(EncodeAsText(str)));
  return text::Text{string_view{*strings.back()}, static_cast<u32>(str.size())};
}

// Helpers.
auto ToText(TextContext& context, const At<binary::HeapType>& value)
    -> At<text::HeapType> {
  if (value->is_heap_kind()) {
    return At{value.loc(), text::HeapType{value->heap_kind()}};
  } else {
    assert(value->is_index());
    return At{value.loc(), text::HeapType{ToText(context, value->index())}};
  }
}

auto ToText(TextContext& context, const At<binary::RefType>& value)
    -> At<text::RefType> {
  return At{value.loc(),
            text::RefType{ToText(context, value->heap_type), value->null}};
}

auto ToText(TextContext& context, const At<binary::ReferenceType>& value)
    -> At<text::ReferenceType> {
  if (value->is_reference_kind()) {
    return At{value.loc(), text::ReferenceType{value->reference_kind()}};
  } else {
    assert(value->is_ref());
    return At{value.loc(), text::ReferenceType{ToText(context, value->ref())}};
  }
}

auto ToText(TextContext& context, const At<binary::Rtt>& value)
    -> At<text::Rtt> {
  return At{value.loc(), text::Rtt{value->depth, ToText(context, value->type)}};
}

auto ToText(TextContext& context, const At<binary::ValueType>& value)
    -> At<text::ValueType> {
  if (value->is_numeric_type()) {
    return At{value.loc(), text::ValueType{value->numeric_type()}};
  } else if (value->is_reference_type()) {
    return At{value.loc(),
              text::ValueType{ToText(context, value->reference_type())}};
  } else {
    assert(value->is_rtt());
    return At{value.loc(), text::ValueType{ToText(context, value->rtt())}};
  }
}

auto ToText(TextContext& context, const binary::ValueTypeList& values)
    -> text::ValueTypeList {
  text::ValueTypeList result;
  for (auto&& value : values) {
    result.push_back(ToText(context, value));
  }
  return result;
}

auto ToTextBound(TextContext& context, const binary::ValueTypeList& values)
    -> At<text::BoundValueTypeList> {
  text::BoundValueTypeList result;
  for (auto&& value : values) {
    result.push_back(text::BoundValueType{nullopt,  // TODO name
                                          ToText(context, value)});
  }
  return result;
}

auto ToText(TextContext& context, const At<binary::StorageType>& value)
    -> At<text::StorageType> {
  if (value->is_value_type()) {
    return At{value.loc(),
              text::StorageType{ToText(context, value->value_type())}};
  } else {
    assert(value->is_packed_type());
    return At{value.loc(), text::StorageType{value->packed_type()}};
  }
}

auto ToText(TextContext& context, const At<string_view>& value)
    -> At<text::Text> {
  return At{value.loc(), context.Add(*value)};
}

auto ToText(TextContext& context, const At<Index>& value) -> At<text::Var> {
  return At{value.loc(), text::Var{value.value()}};
}

auto ToText(TextContext& context, const OptAt<Index>& value)
    -> OptAt<text::Var> {
  if (!value) {
    return nullopt;
  }
  return ToText(context, *value);
}

auto ToText(TextContext& context, const binary::IndexList& values)
    -> text::VarList {
  text::VarList result;
  for (auto value : values) {
    result.push_back(ToText(context, value));
  }
  return result;
}

auto ToText(TextContext& context, const At<binary::FunctionType>& value)
    -> At<text::FunctionType> {
  return At{value.loc(),
            text::FunctionType{ToText(context, value->param_types),
                               ToText(context, value->result_types)}};
}

// Section 1: Type
auto ToTextBound(TextContext& context, const At<binary::FunctionType>& value)
    -> At<text::BoundFunctionType> {
  return At{value.loc(),
            text::BoundFunctionType{ToTextBound(context, value->param_types),
                                    ToText(context, value->result_types)}};
}

auto ToText(TextContext& context, const At<binary::FieldType>& value)
    -> At<text::FieldType> {
  return At{value.loc(),
            text::FieldType{nullopt,  // TODO(name)
                            ToText(context, value->type), value->mut}};
}

auto ToText(TextContext& context, const binary::FieldTypeList& values)
    -> text::FieldTypeList {
  text::FieldTypeList result;
  for (auto&& value : values) {
    result.push_back(ToText(context, value));
  }
  return result;
}

auto ToText(TextContext& context, const At<binary::StructType>& value)
    -> At<text::StructType> {
  return At{value.loc(), text::StructType{ToText(context, value->fields)}};
}

auto ToText(TextContext& context, const At<binary::ArrayType>& value)
    -> At<text::ArrayType> {
  return At{value.loc(), text::ArrayType{ToText(context, value->field)}};
}

auto ToText(TextContext& context, const At<binary::DefinedType>& value)
    -> At<text::DefinedType> {
  if (value->is_function_type()) {
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToTextBound(context, value->function_type())}};
  } else if (value->is_struct_type()) {
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToText(context, value->struct_type())}};
  } else {
    assert(value->is_array_type());
    return At{value.loc(),
              text::DefinedType{nullopt,  // TODO(name)
                                ToText(context, value->array_type())}};
  }
}

// Section 2: Import
auto ToText(TextContext& context, const At<binary::Import>& value)
    -> At<text::Import> {
  auto module = ToText(context, value->module);
  auto name = ToText(context, value->name);

  switch (value->kind()) {
    case ExternalKind::Function:
      return At{value.loc(),
                text::Import{module, name,
                             text::FunctionDesc{
                                 nullopt,  // TODO(name)
                                 ToText(context, value->index()),
                                 {}  // Unresolved bound function type
                             }}};

    case ExternalKind::Table:
      return At{
          value.loc(),
          text::Import{module, name,
                       text::TableDesc{nullopt,  // TODO(name)
                                       ToText(context, value->table_type())}}};

    case ExternalKind::Memory:
      return At{value.loc(),
                text::Import{module, name,
                             text::MemoryDesc{nullopt,  // TODO(name)
                                              value->memory_type()}}};

    case ExternalKind::Global:
      return At{value.loc(),
                text::Import{
                    module, name,
                    text::GlobalDesc{nullopt,  // TODO(name)
                                     ToText(context, value->global_type())}}};

    case ExternalKind::Event:
      return At{
          value.loc(),
          text::Import{module, name,
                       text::EventDesc{nullopt,  // TODO(name)
                                       ToText(context, value->event_type())}}};

    default:
      WASP_UNREACHABLE();
  }
}

// Section 3: Function
auto ToText(TextContext& context, const At<binary::Function>& value)
    -> At<text::Function> {
  return At{value.loc(),
            text::Function{text::FunctionDesc{
                               nullopt,  // TODO(name)
                               ToText(context, value->type_index),
                               {}  // Unresolved bound function type
                           },
                           {},
                           {},
                           nullopt,
                           {}}};
}

// Section 4: Table
auto ToText(TextContext& context, const At<binary::TableType>& value)
    -> At<text::TableType> {
  return At{value.loc(),
            text::TableType{value->limits, ToText(context, value->elemtype)}};
}

auto ToText(TextContext& context, const At<binary::Table>& value)
    -> At<text::Table> {
  return At{value.loc(),
            text::Table{text::TableDesc{nullopt,  // TODO(name)
                                        ToText(context, value->table_type)},
                        {}}};
}

// Section 5: Memory
auto ToText(TextContext& context, const At<binary::Memory>& value)
    -> At<text::Memory> {
  return At{value.loc(),
            text::Memory{text::MemoryDesc{nullopt,  // TODO(name)
                                          value->memory_type},
                         {}}};
}

// Section 6: Global
auto ToText(TextContext& context, const At<binary::ConstantExpression>& value)
    -> At<text::ConstantExpression> {
  return At{value.loc(),
            text::ConstantExpression{ToText(context, value->instructions)}};
}

auto ToText(TextContext& context, const At<binary::GlobalType>& value)
    -> At<text::GlobalType> {
  return At{value.loc(),
            text::GlobalType{ToText(context, value->valtype), value->mut}};
}

auto ToText(TextContext& context, const At<binary::Global>& value)
    -> At<text::Global> {
  return At{value.loc(),
            text::Global{text::GlobalDesc{nullopt,  // TODO(name)
                                          ToText(context, value->global_type)},
                         ToText(context, value->init),
                         {}}};
}

// Section 7: Export
auto ToText(TextContext& context, const At<binary::Export>& value)
    -> At<text::Export> {
  return At{value.loc(), text::Export{value->kind, ToText(context, value->name),
                                      ToText(context, value->index)}};
}

// Section 8: Start
auto ToText(TextContext& context, const At<binary::Start>& value)
    -> At<text::Start> {
  return At{value.loc(), text::Start{ToText(context, value->func_index)}};
}

// Section 9: Elem
auto ToText(TextContext& context, const At<binary::ElementExpression>& value)
    -> At<text::ElementExpression> {
  return At{value.loc(),
            text::ElementExpression{ToText(context, value->instructions)}};
}

auto ToText(TextContext& context, const binary::ElementExpressionList& values)
    -> text::ElementExpressionList {
  text::ElementExpressionList result;
  for (auto&& value : values) {
    result.push_back(ToText(context, value));
  }
  return result;
}

auto ToText(TextContext& context, const binary::ElementList& value)
    -> text::ElementList {
  if (holds_alternative<binary::ElementListWithExpressions>(value)) {
    auto&& elements = get<binary::ElementListWithExpressions>(value);
    return text::ElementListWithExpressions{ToText(context, elements.elemtype),
                                            ToText(context, elements.list)};
  } else {
    auto&& elements = get<binary::ElementListWithIndexes>(value);
    return text::ElementListWithVars{elements.kind,
                                     ToText(context, elements.list)};
  }
}

auto ToText(TextContext& context, const At<binary::ElementSegment>& value)
    -> At<text::ElementSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(),
              text::ElementSegment{nullopt,  // TODO(name)
                                   ToText(context, value->table_index),
                                   ToText(context, *value->offset),
                                   ToText(context, value->elements)}};
  } else {
    return At{value.loc(), text::ElementSegment{
                               nullopt,  // TODO(name)
                               value->type, ToText(context, value->elements)}};
  }
}

// Section 10: Code
auto ToText(TextContext& context, const At<binary::BlockType>& value)
    -> At<text::BlockImmediate> {
  if (value->is_value_type()) {
    return At{
        value.loc(),
        text::BlockImmediate{
            nullopt,  // TODO(name)
            text::FunctionTypeUse{
                nullopt, text::FunctionType{
                             {}, {ToText(context, value->value_type())}}}}};
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
                  text::FunctionTypeUse{ToText(context, value->index()), {}}}};
  }
}

auto ToText(TextContext& context, const At<binary::BrOnCastImmediate>& value)
    -> At<text::BrOnCastImmediate> {
  return At{value.loc(),
            text::BrOnCastImmediate{ToText(context, value->target),
                                    ToText(context, value->types)}};
}

auto ToText(TextContext& context, const At<binary::BrOnExnImmediate>& value)
    -> At<text::BrOnExnImmediate> {
  return At{value.loc(),
            text::BrOnExnImmediate{ToText(context, value->target),
                                   ToText(context, value->event_index)}};
}

auto ToText(TextContext& context, const At<binary::BrTableImmediate>& value)
    -> At<text::BrTableImmediate> {
  return At{value.loc(),
            text::BrTableImmediate{ToText(context, value->targets),
                                   ToText(context, value->default_target)}};
}

auto ToText(TextContext& context,
            const At<binary::CallIndirectImmediate>& value)
    -> At<text::CallIndirectImmediate> {
  return At{value.loc(),
            text::CallIndirectImmediate{
                ToText(context, value->table_index),
                text::FunctionTypeUse{ToText(context, value->index), {}}}};
}

auto ToText(TextContext& context, const At<binary::CopyImmediate>& value)
    -> At<text::CopyImmediate> {
  return At{value.loc(),
            text::CopyImmediate{ToText(context, value->dst_index),
                                ToText(context, value->src_index)}};
}

auto ToText(TextContext& context, const At<binary::FuncBindImmediate>& value)
    -> At<text::FuncBindImmediate> {
  return At{value.loc(),
            text::FuncBindImmediate{ToText(context, value->index), {}}};
}

auto ToText(TextContext& context, const At<binary::HeapType2Immediate>& value)
    -> At<text::HeapType2Immediate> {
  return At{value.loc(),
            text::HeapType2Immediate{ToText(context, value->parent),
                                     ToText(context, value->child)}};
}

auto ToText(TextContext& context, const At<binary::InitImmediate>& value)
    -> At<text::InitImmediate> {
  return At{value.loc(),
            text::InitImmediate{ToText(context, value->segment_index),
                                ToText(context, value->dst_index)}};
}

auto ToText(TextContext& context, const At<binary::LetImmediate>& value)
    -> At<text::LetImmediate> {
  return At{value.loc(), text::LetImmediate{ToText(context, value->block_type),
                                            ToText(context, value->locals)}};
}

auto GetAlignPow2(At<u32> align_log2) -> At<u32> {
  // Must be less than 32.
  assert(*align_log2 < 32);
  return At{align_log2.loc(), 1u << align_log2};
}

auto ToText(TextContext& context, const At<binary::MemArgImmediate>& value)
    -> At<text::MemArgImmediate> {
  return At{value.loc(), text::MemArgImmediate{GetAlignPow2(value->align_log2),
                                               value->offset}};
}

auto ToText(TextContext& context, const At<binary::RttSubImmediate>& value)
    -> At<text::RttSubImmediate> {
  return At{value.loc(),
            text::RttSubImmediate{value->depth, ToText(context, value->types)}};
}

auto ToText(TextContext& context, const At<binary::StructFieldImmediate>& value)
    -> At<text::StructFieldImmediate> {
  return At{value.loc(),
            text::StructFieldImmediate{ToText(context, value->struct_),
                                       ToText(context, value->field)}};
}

auto ToText(TextContext& context, const At<binary::Instruction>& value)
    -> At<text::Instruction> {
  switch (value->immediate.index()) {
    case 0:  // monostate
      return At{value.loc(), text::Instruction{value->opcode}};

    case 1:  // s32
      return At{value.loc(),
                text::Instruction{value->opcode, value->s32_immediate()}};

    case 2:  // s64
      return At{value.loc(),
                text::Instruction{value->opcode, value->s64_immediate()}};

    case 3:  // f32
      return At{value.loc(),
                text::Instruction{value->opcode, value->f32_immediate()}};

    case 4:  // f64
      return At{value.loc(),
                text::Instruction{value->opcode, value->f64_immediate()}};

    case 5:  // v128
      return At{value.loc(),
                text::Instruction{value->opcode, value->v128_immediate()}};

    case 6:  // Index
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->index_immediate())}};

    case 7:  // BlockType
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->block_type_immediate())}};

    case 8:  // BrOnExnImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->br_on_exn_immediate())}};

    case 9:  // BrTableImmediate
      return At{value.loc(), text::Instruction{
                                 value->opcode,
                                 ToText(context, value->br_table_immediate())}};

    case 10:  // CallIndirectImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->call_indirect_immediate())}};

    case 11:  // CopyImmediate
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->copy_immediate())}};

    case 12:  // InitImmediate
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->init_immediate())}};

    case 13:  // LetImmediate
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->let_immediate())}};

    case 14:  // MemArgImmediate
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->mem_arg_immediate())}};

    case 15:  // HeapType
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->heap_type_immediate())}};

    case 16:  // SelectImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            At{value->select_immediate().loc(),
                               ToText(context, *value->select_immediate())}}};

    case 17:  // ShuffleImmediate
      return At{value.loc(),
                text::Instruction{value->opcode, value->shuffle_immediate()}};

    case 18:  // SimdLaneImmediate
      return At{value.loc(),
                text::Instruction{value->opcode, value->simd_lane_immediate()}};

    case 19:  // FuncBindImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->func_bind_immediate())}};

    case 20:  // BrOnCastImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->br_on_cast_immediate())}};

    case 21:  // HeapType2Immediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->heap_type_2_immediate())}};

    case 22:  // RttSubImmediate
      return At{value.loc(),
                text::Instruction{value->opcode,
                                  ToText(context, value->rtt_sub_immediate())}};

    case 23:  // StructFieldImmediate
      return At{
          value.loc(),
          text::Instruction{value->opcode,
                            ToText(context, value->struct_field_immediate())}};

    default:
      WASP_UNREACHABLE();
  }
}

auto ToText(TextContext& context, const binary::InstructionList& values)
    -> text::InstructionList {
  text::InstructionList result;
  for (auto&& value : values) {
    result.push_back(ToText(context, value));
  }
  return result;
}

auto ToText(TextContext& context, const At<binary::UnpackedExpression>& value)
    -> text::InstructionList {
  return ToText(context, value->instructions);
}

auto ToText(TextContext& context, const binary::LocalsList& values)
    -> At<text::BoundValueTypeList> {
  text::BoundValueTypeList result;
  for (auto&& locals : values) {
    auto type = ToText(context, locals->type);
    for (Index i = 0; i < *locals->count; ++i) {
      result.push_back(text::BoundValueType{nullopt,  // TODO(name)
                                            type});
    }
  }
  return result;
}

auto ToText(TextContext& context,
            const At<binary::UnpackedCode>& value,
            At<text::Function>& function) -> At<text::Function>& {
  function->locals = ToText(context, value->locals);
  function->instructions = ToText(context, value->body);
  return function;
}

// Section 11: Data
auto ToText(TextContext& context, const At<SpanU8>& value)
    -> text::DataItemList {
  return text::DataItemList{text::DataItem{context.Add(ToStringView(value))}};
}

auto ToText(TextContext& context, const At<binary::DataSegment>& value)
    -> At<text::DataSegment> {
  if (value->type == SegmentType::Active) {
    return At{value.loc(),
              text::DataSegment{nullopt,  // TODO(name)
                                ToText(context, value->memory_index),
                                ToText(context, *value->offset),
                                ToText(context, value->init)}};
  } else {
    return At{value.loc(),
              text::DataSegment{nullopt,  // TODO(name)
                                ToText(context, value->init)}};
  }
}

// Section 12: DataCount

// Section 13: Event
auto ToText(TextContext& context, const At<binary::EventType>& value)
    -> At<text::EventType> {
  return At{value.loc(),
            text::EventType{
                value->attribute,
                text::FunctionTypeUse{ToText(context, value->type_index), {}}}};
}

auto ToText(TextContext& context, const At<binary::Event>& value)
    -> At<text::Event> {
  return At{value.loc(),
            text::Event{text::EventDesc{nullopt,  // TODO(name)
                                        ToText(context, value->event_type)},
                        nullopt,
                        {}}};
}

// Module
auto ToText(TextContext& context, const At<binary::Module>& value)
    -> At<text::Module> {
  text::Module module;

  auto do_vector = [&](const auto& items) {
    for (auto&& item : items) {
      module.push_back(At{item.loc(), text::ModuleItem{ToText(context, item)}});
    }
  };
  auto do_optional = [&](const auto& maybe_item) {
    if (maybe_item) {
      module.push_back(At{maybe_item->loc(),
                          text::ModuleItem{ToText(context, *maybe_item)}});
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
    At<text::Function> function = ToText(context, value->functions[i]);
    module.push_back(
        At{function.loc(),
           text::ModuleItem{ToText(context, value->codes[i], function)}});
  }

  return At{value.loc(), module};
}

}  // namespace wasp::convert

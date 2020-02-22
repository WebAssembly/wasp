//
// Copyright 2019 WebAssembly Community Group participants
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

#include "wasp/binary/read.h"

#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/utf8.h"
#include "wasp/binary/encoding.h"  // XXX
#include "wasp/binary/encoding/block_type_encoding.h"
#include "wasp/binary/encoding/comdat_symbol_kind_encoding.h"
#include "wasp/binary/encoding/element_type_encoding.h"
#include "wasp/binary/encoding/event_attribute_encoding.h"
#include "wasp/binary/encoding/external_kind_encoding.h"
#include "wasp/binary/encoding/limits_flags_encoding.h"
#include "wasp/binary/encoding/linking_subsection_id_encoding.h"
#include "wasp/binary/encoding/mutability_encoding.h"
#include "wasp/binary/encoding/name_subsection_id_encoding.h"
#include "wasp/binary/encoding/opcode_encoding.h"
#include "wasp/binary/encoding/relocation_type_encoding.h"
#include "wasp/binary/encoding/section_id_encoding.h"
#include "wasp/binary/encoding/segment_flags_encoding.h"
#include "wasp/binary/encoding/symbol_info_flags_encoding.h"
#include "wasp/binary/encoding/symbol_info_kind_encoding.h"
#include "wasp/binary/encoding/value_type_encoding.h"
#include "wasp/binary/errors.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_var_int.h"
#include "wasp/binary/read/read_vector.h"
#include "wasp/binary/read_linking.h"  // TODO move to read_linking.cc
#include "wasp/binary/read_name.h"     // TODO move to read_name.cc

namespace wasp {
namespace binary {

optional<BlockType> Read(SpanU8* data, Context& context, Tag<BlockType>) {
  ErrorsContextGuard guard{context.errors, *data, "block type"};
  if (context.features.multi_value_enabled()) {
    WASP_TRY_READ(val, Read<s32>(data, context));
    WASP_TRY_DECODE_FEATURES(decoded, val, BlockType, "block type",
                             context.features);
    return decoded;
  } else {
    WASP_TRY_READ(val, Read<u8>(data, context));
    WASP_TRY_DECODE_FEATURES(decoded, val, BlockType, "block type",
                             context.features);
    return decoded;
  }
}

optional<BrOnExnImmediate> Read(SpanU8* data,
                                Context& context,
                                Tag<BrOnExnImmediate>) {
  ErrorsContextGuard guard{context.errors, *data, "br_on_exn"};
  WASP_TRY_READ(target, ReadIndex(data, context, "target"));
  WASP_TRY_READ(event_index, ReadIndex(data, context, "event index"));
  return BrOnExnImmediate{target, event_index};
}

optional<BrTableImmediate> Read(SpanU8* data,
                                Context& context,
                                Tag<BrTableImmediate>) {
  ErrorsContextGuard guard{context.errors, *data, "br_table"};
  WASP_TRY_READ(targets, ReadVector<Index>(data, context, "targets"));
  WASP_TRY_READ(default_target, ReadIndex(data, context, "default target"));
  return BrTableImmediate{std::move(targets), default_target};
}

optional<SpanU8> ReadBytes(SpanU8* data,
                           SpanU8::index_type N,
                           Context& context) {
  if (data->size() < N) {
    context.errors.OnError(*data, format("Unable to read {} bytes", N));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  remove_prefix(data, N);
  return result;
}

optional<SpanU8> ReadBytesExpected(SpanU8* data,
                                   SpanU8 expected,
                                   Context& context,
                                   string_view desc) {
  ErrorsContextGuard guard{context.errors, *data, desc};

  auto actual = ReadBytes(data, expected.size(), context);
  if (actual && actual != expected) {
    context.errors.OnError(
        *data, format("Mismatch: expected {}, got {}", expected, *actual));
  }
  return actual;
}

optional<CallIndirectImmediate> Read(SpanU8* data,
                                     Context& context,
                                     Tag<CallIndirectImmediate>) {
  ErrorsContextGuard guard{context.errors, *data, "call_indirect"};
  WASP_TRY_READ(index, ReadIndex(data, context, "type index"));
  if (context.features.reference_types_enabled()) {
    WASP_TRY_READ(table_index, ReadIndex(data, context, "table index"));
    return CallIndirectImmediate{index, table_index};
  } else {
    WASP_TRY_READ(reserved, ReadReserved(data, context));
    return CallIndirectImmediate{index, reserved};
  }
}

optional<Index> ReadCheckLength(SpanU8* data,
                                Context& context,
                                string_view context_name,
                                string_view error_name) {
  WASP_TRY_READ(count, ReadIndex(data, context, context_name));

  // There should be at least one byte per count, so if the data is smaller
  // than that, the module must be malformed.
  if (count > data->size()) {
    context.errors.OnError(*data, format("{} extends past end: {} > {}",
                                         error_name, count, data->size()));
    return nullopt;
  }

  return count;
}

optional<Code> Read(SpanU8* data, Context& context, Tag<Code>) {
  ErrorsContextGuard guard{context.errors, *data, "code"};
  context.code_count++;
  WASP_TRY_READ(body_size, ReadLength(data, context));
  WASP_TRY_READ(body, ReadBytes(data, body_size, context));
  WASP_TRY_READ(locals, ReadVector<Locals>(&body, context, "locals vector"));
  return Code{std::move(locals), Expression{std::move(body)}};
}

optional<Comdat> Read(SpanU8* data, Context& context, Tag<Comdat>) {
  ErrorsContextGuard guard{context.errors, *data, "comdat"};
  WASP_TRY_READ(name, ReadString(data, context, "name"));
  WASP_TRY_READ(flags, Read<u32>(data, context));
  WASP_TRY_READ(symbols, ReadVector<ComdatSymbol>(data, context,
                                                  "comdat symbols vector"));
  return Comdat{name, flags, std::move(symbols)};
}

optional<ComdatSymbol> Read(SpanU8* data, Context& context, Tag<ComdatSymbol>) {
  ErrorsContextGuard guard{context.errors, *data, "comdat symbol"};
  WASP_TRY_READ(kind, Read<ComdatSymbolKind>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  return ComdatSymbol{kind, index};
}

optional<ComdatSymbolKind> Read(SpanU8* data,
                                Context& context,
                                Tag<ComdatSymbolKind>) {
  ErrorsContextGuard guard{context.errors, *data, "comdat symbol kind"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, ComdatSymbolKind, "comdat symbol kind");
  return decoded;
}

optional<ConstantExpression> Read(SpanU8* data,
                                  Context& context,
                                  Tag<ConstantExpression>) {
  ErrorsContextGuard guard{context.errors, *data, "constant expression"};
  WASP_TRY_READ(instr, Read<Instruction>(data, context));
  switch (instr.opcode) {
    case Opcode::I32Const:
    case Opcode::I64Const:
    case Opcode::F32Const:
    case Opcode::F64Const:
    case Opcode::GlobalGet:
      // OK.
      break;

    case Opcode::RefNull:
    case Opcode::RefFunc:
      if (context.features.reference_types_enabled()) {
        break;
      }
      // Fallthrough.

    default:
      context.errors.OnError(
          *data,
          format("Illegal instruction in constant expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, context));
  if (end.opcode != Opcode::End) {
    context.errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ConstantExpression{instr};
}

optional<CopyImmediate> Read(SpanU8* data,
                             Context& context,
                             Tag<CopyImmediate>,
                             BulkImmediateKind kind) {
  ErrorsContextGuard guard{context.errors, *data, "copy immediate"};
  if (kind == BulkImmediateKind::Table &&
      context.features.reference_types_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, context, "dst index"));
    WASP_TRY_READ(src_index, ReadIndex(data, context, "src index"));
    return CopyImmediate{dst_index, src_index};
  } else {
    WASP_TRY_READ(dst_reserved, ReadReserved(data, context));
    WASP_TRY_READ(src_reserved, ReadReserved(data, context));
    return CopyImmediate{dst_reserved, src_reserved};
  }
}

optional<Index> ReadCount(SpanU8* data, Context& context) {
  return ReadCheckLength(data, context, "count", "Count");
}

optional<DataCount> Read(SpanU8* data, Context& context, Tag<DataCount>) {
  ErrorsContextGuard guard{context.errors, *data, "data count"};
  WASP_TRY_READ(count, ReadIndex(data, context, "count"));
  context.declared_data_count = count;
  return DataCount{count};
}

optional<DataSegment> Read(SpanU8* data, Context& context, Tag<DataSegment>) {
  ErrorsContextGuard guard{context.errors, *data, "data segment"};
  context.data_count++;
  auto decoded = encoding::DecodedDataSegmentFlags::MVP();
  if (context.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, context, "flags"));
    WASP_TRY_DECODE(decoded_opt, flags, DataSegmentFlags, "flags");
    decoded = *decoded_opt;
  }

  Index memory_index = 0;
  if (!context.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(memory_index_, ReadIndex(data, context, "memory index"));
    memory_index = memory_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset, Read<ConstantExpression>(data, context),
                          "offset");
    WASP_TRY_READ(len, ReadLength(data, context));
    WASP_TRY_READ(init, ReadBytes(data, len, context));
    return DataSegment{memory_index, offset, init};
  } else {
    WASP_TRY_READ(len, ReadLength(data, context));
    WASP_TRY_READ(init, ReadBytes(data, len, context));
    return DataSegment{init};
  }
}

optional<ElementExpression> Read(SpanU8* data,
                                 Context& context,
                                 Tag<ElementExpression>) {
  ErrorsContextGuard guard{context.errors, *data, "element expression"};
  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(context.features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types proposal,
  // but their encoding is still used by the bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  Context new_context{new_features, context.errors};
  WASP_TRY_READ(instr, Read<Instruction>(data, new_context));
  switch (instr.opcode) {
    case Opcode::RefNull:
    case Opcode::RefFunc:
      // OK.
      break;

    default:
      context.errors.OnError(
          *data,
          format("Illegal instruction in element expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, context));
  if (end.opcode != Opcode::End) {
    context.errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ElementExpression{instr};
}

optional<ElementSegment> Read(SpanU8* data,
                              Context& context,
                              Tag<ElementSegment>) {
  ErrorsContextGuard guard{context.errors, *data, "element segment"};
  auto decoded = encoding::DecodedElemSegmentFlags::MVP();
  if (context.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, context, "flags"));
    WASP_TRY_DECODE_FEATURES(decoded_opt, flags, ElemSegmentFlags, "flags",
                             context.features);
    decoded = *decoded_opt;
  }

  Index table_index = 0;
  if (!context.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(table_index_, ReadIndex(data, context, "table index"));
    table_index = table_index_;
  }

  optional<ConstantExpression> offset;
  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset_, Read<ConstantExpression>(data, context),
                          "offset");
    offset = offset_;
  }

  if (decoded.has_expressions == encoding::HasExpressions::Yes) {
    ElementType element_type = ElementType::Funcref;
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(element_type_, Read<ElementType>(data, context));
      element_type = element_type_;
    }
    WASP_TRY_READ(init,
                  ReadVector<ElementExpression>(data, context, "initializers"));
    if (decoded.segment_type == SegmentType::Active) {
      return ElementSegment{table_index, *offset, element_type, init};
    } else {
      return ElementSegment{decoded.segment_type, element_type, init};
    }
  } else {
    ExternalKind kind = ExternalKind::Function;
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(kind_, Read<ExternalKind>(data, context));
      kind = kind_;
    }
    WASP_TRY_READ(init, ReadVector<Index>(data, context, "initializers"));

    if (decoded.segment_type == SegmentType::Active) {
      return ElementSegment{table_index, *offset, kind, init};
    } else {
      return ElementSegment{decoded.segment_type, kind, init};
    }
  }
}

optional<ElementType> Read(SpanU8* data, Context& context, Tag<ElementType>) {
  ErrorsContextGuard guard{context.errors, *data, "element type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, ElementType, "element type",
                           context.features);
  return decoded;
}

optional<Event> Read(SpanU8* data, Context& context, Tag<Event>) {
  ErrorsContextGuard guard{context.errors, *data, "event"};
  WASP_TRY_READ(event_type, Read<EventType>(data, context));
  return Event{event_type};
}

optional<EventAttribute> Read(SpanU8* data,
                              Context& context,
                              Tag<EventAttribute>) {
  ErrorsContextGuard guard{context.errors, *data, "event attribute"};
  WASP_TRY_READ(val, Read<u32>(data, context));
  WASP_TRY_DECODE(decoded, val, EventAttribute, "event attribute");
  return decoded;
}

optional<EventType> Read(SpanU8* data, Context& context, Tag<EventType>) {
  ErrorsContextGuard guard{context.errors, *data, "event type"};
  WASP_TRY_READ(attribute, Read<EventAttribute>(data, context));
  WASP_TRY_READ(type_index, ReadIndex(data, context, "type index"));
  return EventType{attribute, type_index};
}

optional<Export> Read(SpanU8* data, Context& context, Tag<Export>) {
  ErrorsContextGuard guard{context.errors, *data, "export"};
  WASP_TRY_READ(name, ReadUtf8String(data, context, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  return Export{kind, name, index};
}

optional<ExternalKind> Read(SpanU8* data, Context& context, Tag<ExternalKind>) {
  ErrorsContextGuard guard{context.errors, *data, "external kind"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, ExternalKind, "external kind",
                           context.features);
  return decoded;
}

optional<f32> Read(SpanU8* data, Context& context, Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard guard{context.errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), context));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

optional<f64> Read(SpanU8* data, Context& context, Tag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  ErrorsContextGuard guard{context.errors, *data, "f64"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), context));
  f64 result;
  memcpy(&result, bytes.data(), sizeof(f64));
  return result;
}

optional<Function> Read(SpanU8* data, Context& context, Tag<Function>) {
  ErrorsContextGuard guard{context.errors, *data, "function"};
  context.defined_function_count++;
  WASP_TRY_READ(type_index, ReadIndex(data, context, "type index"));
  return Function{type_index};
}

optional<FunctionType> Read(SpanU8* data, Context& context, Tag<FunctionType>) {
  ErrorsContextGuard guard{context.errors, *data, "function type"};
  WASP_TRY_READ(param_types,
                ReadVector<ValueType>(data, context, "param types"));
  WASP_TRY_READ(result_types,
                ReadVector<ValueType>(data, context, "result types"));
  return FunctionType{std::move(param_types), std::move(result_types)};
}

optional<Global> Read(SpanU8* data, Context& context, Tag<Global>) {
  ErrorsContextGuard guard{context.errors, *data, "global"};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, context));
  WASP_TRY_READ(init_expr, Read<ConstantExpression>(data, context));
  return Global{global_type, std::move(init_expr)};
}

optional<GlobalType> Read(SpanU8* data, Context& context, Tag<GlobalType>) {
  ErrorsContextGuard guard{context.errors, *data, "global type"};
  WASP_TRY_READ(type, Read<ValueType>(data, context));
  WASP_TRY_READ(mut, Read<Mutability>(data, context));
  return GlobalType{type, mut};
}

optional<Import> Read(SpanU8* data, Context& context, Tag<Import>) {
  ErrorsContextGuard guard{context.errors, *data, "import"};
  WASP_TRY_READ(module, ReadUtf8String(data, context, "module name"));
  WASP_TRY_READ(name, ReadUtf8String(data, context, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, context));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index, ReadIndex(data, context, "function index"));
      return Import{module, name, type_index};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, context));
      return Import{module, name, table_type};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, context));
      return Import{module, name, memory_type};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, context));
      return Import{module, name, global_type};
    }
    case ExternalKind::Event: {
      WASP_TRY_READ(event_type, Read<EventType>(data, context));
      return Import{module, name, event_type};
    }
  }
  WASP_UNREACHABLE();
}

optional<Index> ReadIndex(SpanU8* data, Context& context, string_view desc) {
  return ReadVarInt<Index>(data, context, desc);
}

optional<IndirectNameAssoc> Read(SpanU8* data,
                                 Context& context,
                                 Tag<IndirectNameAssoc>) {
  ErrorsContextGuard guard{context.errors, *data, "indirect name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  WASP_TRY_READ(name_map, ReadVector<NameAssoc>(data, context, "name map"));
  return IndirectNameAssoc{index, std::move(name_map)};
}

optional<InitFunction> Read(SpanU8* data, Context& context, Tag<InitFunction>) {
  ErrorsContextGuard guard{context.errors, *data, "init function"};
  WASP_TRY_READ(priority, Read<u32>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "function index"));
  return InitFunction{priority, index};
}

optional<InitImmediate> Read(SpanU8* data,
                             Context& context,
                             Tag<InitImmediate>,
                             BulkImmediateKind kind) {
  ErrorsContextGuard guard{context.errors, *data, "init immediate"};
  WASP_TRY_READ(segment_index, ReadIndex(data, context, "segment index"));
  if (kind == BulkImmediateKind::Table &&
      context.features.reference_types_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, context, "table index"));
    return InitImmediate{segment_index, dst_index};
  } else {
    WASP_TRY_READ(reserved, ReadReserved(data, context));
    return InitImmediate{segment_index, reserved};
  }
}

optional<Instruction> Read(SpanU8* data, Context& context, Tag<Instruction>) {
  WASP_TRY_READ(opcode, Read<Opcode>(data, context));
  switch (opcode) {
    // No immediates:
    case Opcode::Unreachable:
    case Opcode::Nop:
    case Opcode::Else:
    case Opcode::Catch:
    case Opcode::Rethrow:
    case Opcode::End:
    case Opcode::Return:
    case Opcode::Drop:
    case Opcode::Select:
    case Opcode::I32Eqz:
    case Opcode::I32Eq:
    case Opcode::I32Ne:
    case Opcode::I32LtS:
    case Opcode::I32LeS:
    case Opcode::I32LtU:
    case Opcode::I32LeU:
    case Opcode::I32GtS:
    case Opcode::I32GeS:
    case Opcode::I32GtU:
    case Opcode::I32GeU:
    case Opcode::I64Eqz:
    case Opcode::I64Eq:
    case Opcode::I64Ne:
    case Opcode::I64LtS:
    case Opcode::I64LeS:
    case Opcode::I64LtU:
    case Opcode::I64LeU:
    case Opcode::I64GtS:
    case Opcode::I64GeS:
    case Opcode::I64GtU:
    case Opcode::I64GeU:
    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Le:
    case Opcode::F32Gt:
    case Opcode::F32Ge:
    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Le:
    case Opcode::F64Gt:
    case Opcode::F64Ge:
    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
    case Opcode::I32Add:
    case Opcode::I32Sub:
    case Opcode::I32Mul:
    case Opcode::I32DivS:
    case Opcode::I32DivU:
    case Opcode::I32RemS:
    case Opcode::I32RemU:
    case Opcode::I32And:
    case Opcode::I32Or:
    case Opcode::I32Xor:
    case Opcode::I32Shl:
    case Opcode::I32ShrS:
    case Opcode::I32ShrU:
    case Opcode::I32Rotl:
    case Opcode::I32Rotr:
    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::I64Add:
    case Opcode::I64Sub:
    case Opcode::I64Mul:
    case Opcode::I64DivS:
    case Opcode::I64DivU:
    case Opcode::I64RemS:
    case Opcode::I64RemU:
    case Opcode::I64And:
    case Opcode::I64Or:
    case Opcode::I64Xor:
    case Opcode::I64Shl:
    case Opcode::I64ShrS:
    case Opcode::I64ShrU:
    case Opcode::I64Rotl:
    case Opcode::I64Rotr:
    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
    case Opcode::I32WrapI64:
    case Opcode::I32TruncF32S:
    case Opcode::I32TruncF32U:
    case Opcode::I32TruncF64S:
    case Opcode::I32TruncF64U:
    case Opcode::I64ExtendI32S:
    case Opcode::I64ExtendI32U:
    case Opcode::I64TruncF32S:
    case Opcode::I64TruncF32U:
    case Opcode::I64TruncF64S:
    case Opcode::I64TruncF64U:
    case Opcode::F32ConvertI32S:
    case Opcode::F32ConvertI32U:
    case Opcode::F32ConvertI64S:
    case Opcode::F32ConvertI64U:
    case Opcode::F32DemoteF64:
    case Opcode::F64ConvertI32S:
    case Opcode::F64ConvertI32U:
    case Opcode::F64ConvertI64S:
    case Opcode::F64ConvertI64U:
    case Opcode::F64PromoteF32:
    case Opcode::I32ReinterpretF32:
    case Opcode::I64ReinterpretF64:
    case Opcode::F32ReinterpretI32:
    case Opcode::F64ReinterpretI64:
    case Opcode::I32Extend8S:
    case Opcode::I32Extend16S:
    case Opcode::I64Extend8S:
    case Opcode::I64Extend16S:
    case Opcode::I64Extend32S:
    case Opcode::RefNull:
    case Opcode::RefIsNull:
    case Opcode::I32TruncSatF32S:
    case Opcode::I32TruncSatF32U:
    case Opcode::I32TruncSatF64S:
    case Opcode::I32TruncSatF64U:
    case Opcode::I64TruncSatF32S:
    case Opcode::I64TruncSatF32U:
    case Opcode::I64TruncSatF64S:
    case Opcode::I64TruncSatF64U:
    case Opcode::I8X16Add:
    case Opcode::I16X8Add:
    case Opcode::I32X4Add:
    case Opcode::I64X2Add:
    case Opcode::I8X16Sub:
    case Opcode::I16X8Sub:
    case Opcode::I32X4Sub:
    case Opcode::I64X2Sub:
    case Opcode::I16X8Mul:
    case Opcode::I32X4Mul:
    case Opcode::I64X2Mul:
    case Opcode::I8X16AddSaturateS:
    case Opcode::I8X16AddSaturateU:
    case Opcode::I16X8AddSaturateS:
    case Opcode::I16X8AddSaturateU:
    case Opcode::I8X16SubSaturateS:
    case Opcode::I8X16SubSaturateU:
    case Opcode::I16X8SubSaturateS:
    case Opcode::I16X8SubSaturateU:
    case Opcode::I8X16MinS:
    case Opcode::I8X16MinU:
    case Opcode::I8X16MaxS:
    case Opcode::I8X16MaxU:
    case Opcode::I16X8MinS:
    case Opcode::I16X8MinU:
    case Opcode::I16X8MaxS:
    case Opcode::I16X8MaxU:
    case Opcode::I32X4MinS:
    case Opcode::I32X4MinU:
    case Opcode::I32X4MaxS:
    case Opcode::I32X4MaxU:
    case Opcode::I8X16Shl:
    case Opcode::I16X8Shl:
    case Opcode::I32X4Shl:
    case Opcode::I64X2Shl:
    case Opcode::I8X16ShrS:
    case Opcode::I8X16ShrU:
    case Opcode::I16X8ShrS:
    case Opcode::I16X8ShrU:
    case Opcode::I32X4ShrS:
    case Opcode::I32X4ShrU:
    case Opcode::I64X2ShrS:
    case Opcode::I64X2ShrU:
    case Opcode::V128And:
    case Opcode::V128Or:
    case Opcode::V128Xor:
    case Opcode::F32X4Min:
    case Opcode::F64X2Min:
    case Opcode::F32X4Max:
    case Opcode::F64X2Max:
    case Opcode::F32X4Add:
    case Opcode::F64X2Add:
    case Opcode::F32X4Sub:
    case Opcode::F64X2Sub:
    case Opcode::F32X4Div:
    case Opcode::F64X2Div:
    case Opcode::F32X4Mul:
    case Opcode::F64X2Mul:
    case Opcode::I8X16Eq:
    case Opcode::I16X8Eq:
    case Opcode::I32X4Eq:
    case Opcode::F32X4Eq:
    case Opcode::F64X2Eq:
    case Opcode::I8X16Ne:
    case Opcode::I16X8Ne:
    case Opcode::I32X4Ne:
    case Opcode::F32X4Ne:
    case Opcode::F64X2Ne:
    case Opcode::I8X16LtS:
    case Opcode::I8X16LtU:
    case Opcode::I16X8LtS:
    case Opcode::I16X8LtU:
    case Opcode::I32X4LtS:
    case Opcode::I32X4LtU:
    case Opcode::F32X4Lt:
    case Opcode::F64X2Lt:
    case Opcode::I8X16LeS:
    case Opcode::I8X16LeU:
    case Opcode::I16X8LeS:
    case Opcode::I16X8LeU:
    case Opcode::I32X4LeS:
    case Opcode::I32X4LeU:
    case Opcode::F32X4Le:
    case Opcode::F64X2Le:
    case Opcode::I8X16GtS:
    case Opcode::I8X16GtU:
    case Opcode::I16X8GtS:
    case Opcode::I16X8GtU:
    case Opcode::I32X4GtS:
    case Opcode::I32X4GtU:
    case Opcode::F32X4Gt:
    case Opcode::F64X2Gt:
    case Opcode::I8X16GeS:
    case Opcode::I8X16GeU:
    case Opcode::I16X8GeS:
    case Opcode::I16X8GeU:
    case Opcode::I32X4GeS:
    case Opcode::I32X4GeU:
    case Opcode::F32X4Ge:
    case Opcode::F64X2Ge:
    case Opcode::I8X16Splat:
    case Opcode::I16X8Splat:
    case Opcode::I32X4Splat:
    case Opcode::I64X2Splat:
    case Opcode::F32X4Splat:
    case Opcode::F64X2Splat:
    case Opcode::I8X16Neg:
    case Opcode::I16X8Neg:
    case Opcode::I32X4Neg:
    case Opcode::I64X2Neg:
    case Opcode::V128Not:
    case Opcode::I8X16AnyTrue:
    case Opcode::I16X8AnyTrue:
    case Opcode::I32X4AnyTrue:
    case Opcode::I8X16AllTrue:
    case Opcode::I16X8AllTrue:
    case Opcode::I32X4AllTrue:
    case Opcode::F32X4Neg:
    case Opcode::F64X2Neg:
    case Opcode::F32X4Abs:
    case Opcode::F64X2Abs:
    case Opcode::F32X4Sqrt:
    case Opcode::F64X2Sqrt:
    case Opcode::V128BitSelect:
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::I32X4TruncSatF32X4S:
    case Opcode::I32X4TruncSatF32X4U:
    case Opcode::V8X16Swizzle:
    case Opcode::I8X16NarrowI16X8S:
    case Opcode::I8X16NarrowI16X8U:
    case Opcode::I16X8NarrowI32X4S:
    case Opcode::I16X8NarrowI32X4U:
    case Opcode::I16X8WidenLowI8X16S:
    case Opcode::I16X8WidenHighI8X16S:
    case Opcode::I16X8WidenLowI8X16U:
    case Opcode::I16X8WidenHighI8X16U:
    case Opcode::I32X4WidenLowI16X8S:
    case Opcode::I32X4WidenHighI16X8S:
    case Opcode::I32X4WidenLowI16X8U:
    case Opcode::I32X4WidenHighI16X8U:
    case Opcode::V128Andnot:
    case Opcode::I8X16AvgrU:
    case Opcode::I16X8AvgrU:
      return Instruction{Opcode{opcode}};

    // Type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If:
    case Opcode::Try: {
      WASP_TRY_READ(type, Read<BlockType>(data, context));
      return Instruction{Opcode{opcode}, type};
    }

    // Index immediate.
    case Opcode::Throw:
    case Opcode::Br:
    case Opcode::BrIf:
    case Opcode::Call:
    case Opcode::ReturnCall:
    case Opcode::LocalGet:
    case Opcode::LocalSet:
    case Opcode::LocalTee:
    case Opcode::GlobalGet:
    case Opcode::GlobalSet:
    case Opcode::TableGet:
    case Opcode::TableSet:
    case Opcode::RefFunc:
    case Opcode::DataDrop:
    case Opcode::ElemDrop:
    case Opcode::TableGrow:
    case Opcode::TableSize:
    case Opcode::TableFill: {
      WASP_TRY_READ(index, ReadIndex(data, context, "index"));
      return Instruction{Opcode{opcode}, index};
    }

    // Index, Index immediates.
    case Opcode::BrOnExn: {
      WASP_TRY_READ(immediate, Read<BrOnExnImmediate>(data, context));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, context));
      return Instruction{Opcode{opcode}, std::move(immediate)};
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect: {
      WASP_TRY_READ(immediate, Read<CallIndirectImmediate>(data, context));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Memarg (alignment, offset) immediates.
    case Opcode::I32Load:
    case Opcode::I64Load:
    case Opcode::F32Load:
    case Opcode::F64Load:
    case Opcode::I32Load8S:
    case Opcode::I32Load8U:
    case Opcode::I32Load16S:
    case Opcode::I32Load16U:
    case Opcode::I64Load8S:
    case Opcode::I64Load8U:
    case Opcode::I64Load16S:
    case Opcode::I64Load16U:
    case Opcode::I64Load32S:
    case Opcode::I64Load32U:
    case Opcode::V128Load:
    case Opcode::I32Store:
    case Opcode::I64Store:
    case Opcode::F32Store:
    case Opcode::F64Store:
    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32:
    case Opcode::V128Store:
    case Opcode::V8X16LoadSplat:
    case Opcode::V16X8LoadSplat:
    case Opcode::V32X4LoadSplat:
    case Opcode::V64X2LoadSplat:
    case Opcode::I16X8Load8X8S:
    case Opcode::I16X8Load8X8U:
    case Opcode::I32X4Load16X4S:
    case Opcode::I32X4Load16X4U:
    case Opcode::I64X2Load32X2S:
    case Opcode::I64X2Load32X2U:
    case Opcode::AtomicNotify:
    case Opcode::I32AtomicWait:
    case Opcode::I64AtomicWait:
    case Opcode::I32AtomicLoad:
    case Opcode::I64AtomicLoad:
    case Opcode::I32AtomicLoad8U:
    case Opcode::I32AtomicLoad16U:
    case Opcode::I64AtomicLoad8U:
    case Opcode::I64AtomicLoad16U:
    case Opcode::I64AtomicLoad32U:
    case Opcode::I32AtomicStore:
    case Opcode::I64AtomicStore:
    case Opcode::I32AtomicStore8:
    case Opcode::I32AtomicStore16:
    case Opcode::I64AtomicStore8:
    case Opcode::I64AtomicStore16:
    case Opcode::I64AtomicStore32:
    case Opcode::I32AtomicRmwAdd:
    case Opcode::I64AtomicRmwAdd:
    case Opcode::I32AtomicRmw8AddU:
    case Opcode::I32AtomicRmw16AddU:
    case Opcode::I64AtomicRmw8AddU:
    case Opcode::I64AtomicRmw16AddU:
    case Opcode::I64AtomicRmw32AddU:
    case Opcode::I32AtomicRmwSub:
    case Opcode::I64AtomicRmwSub:
    case Opcode::I32AtomicRmw8SubU:
    case Opcode::I32AtomicRmw16SubU:
    case Opcode::I64AtomicRmw8SubU:
    case Opcode::I64AtomicRmw16SubU:
    case Opcode::I64AtomicRmw32SubU:
    case Opcode::I32AtomicRmwAnd:
    case Opcode::I64AtomicRmwAnd:
    case Opcode::I32AtomicRmw8AndU:
    case Opcode::I32AtomicRmw16AndU:
    case Opcode::I64AtomicRmw8AndU:
    case Opcode::I64AtomicRmw16AndU:
    case Opcode::I64AtomicRmw32AndU:
    case Opcode::I32AtomicRmwOr:
    case Opcode::I64AtomicRmwOr:
    case Opcode::I32AtomicRmw8OrU:
    case Opcode::I32AtomicRmw16OrU:
    case Opcode::I64AtomicRmw8OrU:
    case Opcode::I64AtomicRmw16OrU:
    case Opcode::I64AtomicRmw32OrU:
    case Opcode::I32AtomicRmwXor:
    case Opcode::I64AtomicRmwXor:
    case Opcode::I32AtomicRmw8XorU:
    case Opcode::I32AtomicRmw16XorU:
    case Opcode::I64AtomicRmw8XorU:
    case Opcode::I64AtomicRmw16XorU:
    case Opcode::I64AtomicRmw32XorU:
    case Opcode::I32AtomicRmwXchg:
    case Opcode::I64AtomicRmwXchg:
    case Opcode::I32AtomicRmw8XchgU:
    case Opcode::I32AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw8XchgU:
    case Opcode::I64AtomicRmw16XchgU:
    case Opcode::I64AtomicRmw32XchgU:
    case Opcode::I32AtomicRmwCmpxchg:
    case Opcode::I64AtomicRmwCmpxchg:
    case Opcode::I32AtomicRmw8CmpxchgU:
    case Opcode::I32AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw8CmpxchgU:
    case Opcode::I64AtomicRmw16CmpxchgU:
    case Opcode::I64AtomicRmw32CmpxchgU: {
      WASP_TRY_READ(memarg, Read<MemArgImmediate>(data, context));
      return Instruction{Opcode{opcode}, memarg};
    }

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill: {
      WASP_TRY_READ(reserved, ReadReserved(data, context));
      return Instruction{Opcode{opcode}, reserved};
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, context), "i32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, context), "i64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, context), "f32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, context), "f64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::V128Const: {
      WASP_TRY_READ_CONTEXT(value, Read<v128>(data, context), "v128 constant");
      return Instruction{Opcode{opcode}, value};
    }

    // Reserved, Index immediates.
    case Opcode::MemoryInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, context,
                                                   BulkImmediateKind::Memory));
      return Instruction{Opcode{opcode}, immediate};
    }
    case Opcode::TableInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, context,
                                                   BulkImmediateKind::Table));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Reserved, reserved immediates.
    case Opcode::MemoryCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, context,
                                                   BulkImmediateKind::Memory));
      return Instruction{Opcode{opcode}, immediate};
    }
    case Opcode::TableCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, context,
                                                   BulkImmediateKind::Table));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Shuffle immediate.
    case Opcode::V8X16Shuffle: {
      WASP_TRY_READ(immediate, Read<ShuffleImmediate>(data, context));
      return Instruction{Opcode{opcode}, immediate};
    }

    // ValueTypes immediate.
    case Opcode::SelectT: {
      WASP_TRY_READ(immediate, ReadVector<ValueType>(data, context, "types"));
      return Instruction{Opcode{opcode}, immediate};
    }

    // u8 immediate.
    case Opcode::I8X16ExtractLaneS:
    case Opcode::I8X16ExtractLaneU:
    case Opcode::I16X8ExtractLaneS:
    case Opcode::I16X8ExtractLaneU:
    case Opcode::I32X4ExtractLane:
    case Opcode::I64X2ExtractLane:
    case Opcode::F32X4ExtractLane:
    case Opcode::F64X2ExtractLane:
    case Opcode::I8X16ReplaceLane:
    case Opcode::I16X8ReplaceLane:
    case Opcode::I32X4ReplaceLane:
    case Opcode::I64X2ReplaceLane:
    case Opcode::F32X4ReplaceLane:
    case Opcode::F64X2ReplaceLane: {
      WASP_TRY_READ(lane, Read<u8>(data, context));
      return Instruction{Opcode{opcode}, u8{lane}};
    }
  }
  WASP_UNREACHABLE();
}

optional<Index> ReadLength(SpanU8* data, Context& context) {
  return ReadCheckLength(data, context, "length", "Length");
}

optional<Limits> Read(SpanU8* data, Context& context, Tag<Limits>) {
  ErrorsContextGuard guard{context.errors, *data, "limits"};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, context), "flags");
  WASP_TRY_DECODE_FEATURES(decoded, flags, LimitsFlags, "flags value",
                           context.features);

  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, context), "min");

  if (decoded->has_max == encoding::HasMax::No) {
    return Limits{min};
  } else {
    WASP_TRY_READ_CONTEXT(max, Read<u32>(data, context), "max");
    return Limits{min, max, decoded->shared};
  }
}

optional<LinkingSubsection> Read(SpanU8* data,
                                 Context& context,
                                 Tag<LinkingSubsection>) {
  ErrorsContextGuard guard{context.errors, *data, "linking subsection"};
  WASP_TRY_READ(id, Read<LinkingSubsectionId>(data, context));
  WASP_TRY_READ(length, ReadLength(data, context));
  auto bytes = *ReadBytes(data, length, context);
  return LinkingSubsection{id, bytes};
}

optional<LinkingSubsectionId> Read(SpanU8* data,
                                   Context& context,
                                   Tag<LinkingSubsectionId>) {
  ErrorsContextGuard guard{context.errors, *data, "linking subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, LinkingSubsectionId, "linking subsection id");
  return decoded;
}

optional<Locals> Read(SpanU8* data, Context& context, Tag<Locals>) {
  ErrorsContextGuard guard{context.errors, *data, "locals"};
  WASP_TRY_READ(count, ReadIndex(data, context, "count"));
  WASP_TRY_READ_CONTEXT(type, Read<ValueType>(data, context), "type");
  return Locals{count, type};
}

optional<MemArgImmediate> Read(SpanU8* data,
                               Context& context,
                               Tag<MemArgImmediate>) {
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, context), "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, context), "offset");
  return MemArgImmediate{align_log2, offset};
}

optional<Memory> Read(SpanU8* data, Context& context, Tag<Memory>) {
  ErrorsContextGuard guard{context.errors, *data, "memory"};
  WASP_TRY_READ(memory_type, Read<MemoryType>(data, context));
  return Memory{memory_type};
}

optional<MemoryType> Read(SpanU8* data, Context& context, Tag<MemoryType>) {
  ErrorsContextGuard guard{context.errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, context));
  return MemoryType{limits};
}

optional<Mutability> Read(SpanU8* data, Context& context, Tag<Mutability>) {
  ErrorsContextGuard guard{context.errors, *data, "mutability"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, Mutability, "mutability");
  return decoded;
}

optional<NameAssoc> Read(SpanU8* data, Context& context, Tag<NameAssoc>) {
  ErrorsContextGuard guard{context.errors, *data, "name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  WASP_TRY_READ(name, ReadString(data, context, "name"));
  return NameAssoc{index, name};
}

optional<NameSubsection> Read(SpanU8* data,
                              Context& context,
                              Tag<NameSubsection>) {
  ErrorsContextGuard guard{context.errors, *data, "name subsection"};
  WASP_TRY_READ(id, Read<NameSubsectionId>(data, context));
  WASP_TRY_READ(length, ReadLength(data, context));
  auto bytes = *ReadBytes(data, length, context);
  return NameSubsection{id, bytes};
}

optional<NameSubsectionId> Read(SpanU8* data,
                                Context& context,
                                Tag<NameSubsectionId>) {
  ErrorsContextGuard guard{context.errors, *data, "name subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, NameSubsectionId, "name subsection id");
  return decoded;
}

optional<Opcode> Read(SpanU8* data, Context& context, Tag<Opcode>) {
  ErrorsContextGuard guard{context.errors, *data, "opcode"};
  WASP_TRY_READ(val, Read<u8>(data, context));

  if (encoding::Opcode::IsPrefixByte(val, context.features)) {
    WASP_TRY_READ(code, Read<u32>(data, context));
    auto decoded = encoding::Opcode::Decode(val, code, context.features);
    if (!decoded) {
      context.errors.OnError(*data, format("Unknown opcode: {} {}", val, code));
      return nullopt;
    }
    return decoded;
  } else {
    WASP_TRY_DECODE_FEATURES(decoded, val, Opcode, "opcode", context.features);
    return decoded;
  }
}

optional<RelocationEntry> Read(SpanU8* data,
                               Context& context,
                               Tag<RelocationEntry>) {
  ErrorsContextGuard guard{context.errors, *data, "relocation entry"};
  WASP_TRY_READ(type, Read<RelocationType>(data, context));
  WASP_TRY_READ(offset, Read<u32>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  switch (type) {
    case RelocationType::MemoryAddressLEB:
    case RelocationType::MemoryAddressSLEB:
    case RelocationType::MemoryAddressI32:
    case RelocationType::FunctionOffsetI32:
    case RelocationType::SectionOffsetI32: {
      WASP_TRY_READ(addend, Read<s32>(data, context));
      return RelocationEntry{type, offset, index, addend};
    }

    default:
      return RelocationEntry{type, offset, index, nullopt};
  }
}

optional<RelocationType> Read(SpanU8* data,
                              Context& context,
                              Tag<RelocationType>) {
  ErrorsContextGuard guard{context.errors, *data, "relocation type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, RelocationType, "relocation type");
  return decoded;
}

optional<u8> ReadReserved(SpanU8* data, Context& context) {
  ErrorsContextGuard guard{context.errors, *data, "reserved"};
  WASP_TRY_READ(reserved, Read<u8>(data, context));
  if (reserved != 0) {
    context.errors.OnError(
        *data, format("Expected reserved byte 0, got {}", reserved));
    return nullopt;
  }
  return 0;
}

optional<s32> Read(SpanU8* data, Context& context, Tag<s32>) {
  return ReadVarInt<s32>(data, context, "s32");
}

optional<s64> Read(SpanU8* data, Context& context, Tag<s64>) {
  return ReadVarInt<s64>(data, context, "s64");
}

optional<Section> Read(SpanU8* data, Context& context, Tag<Section>) {
  ErrorsContextGuard guard{context.errors, *data, "section"};
  WASP_TRY_READ(id, Read<SectionId>(data, context));
  WASP_TRY_READ(length, ReadLength(data, context));
  auto bytes = *ReadBytes(data, length, context);

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name, ReadUtf8String(&bytes, context, "custom section name"));
    return Section{CustomSection{name, bytes}};
  } else {
    if (context.last_section_id && *context.last_section_id >= id) {
      context.errors.OnError(
          *data, format("Section out of order: {} cannot occur after {}", id,
                        *context.last_section_id));
    }
    context.last_section_id = id;

    return Section{KnownSection{id, bytes}};
  }
}

optional<SectionId> Read(SpanU8* data, Context& context, Tag<SectionId>) {
  ErrorsContextGuard guard{context.errors, *data, "section id"};
  WASP_TRY_READ(val, Read<u32>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, SectionId, "section id",
                           context.features);
  return decoded;
}

optional<SegmentInfo> Read(SpanU8* data, Context& context, Tag<SegmentInfo>) {
  ErrorsContextGuard guard{context.errors, *data, "segment info"};
  WASP_TRY_READ(name, ReadString(data, context, "name"));
  WASP_TRY_READ(align_log2, Read<u32>(data, context));
  WASP_TRY_READ(flags, Read<u32>(data, context));
  return SegmentInfo{name, align_log2, flags};
}

optional<ShuffleImmediate> Read(SpanU8* data,
                                Context& context,
                                Tag<ShuffleImmediate>) {
  ErrorsContextGuard guard{context.errors, *data, "shuffle immediate"};
  ShuffleImmediate immediate;
  for (int i = 0; i < 16; ++i) {
    WASP_TRY_READ(byte, Read<u8>(data, context));
    immediate[i] = byte;
  }
  return immediate;
}

optional<Start> Read(SpanU8* data, Context& context, Tag<Start>) {
  ErrorsContextGuard guard{context.errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, context, "function index"));
  return Start{index};
}

optional<string_view> ReadString(SpanU8* data,
                                 Context& context,
                                 string_view desc) {
  ErrorsContextGuard guard{context.errors, *data, desc};
  WASP_TRY_READ(len, ReadLength(data, context));
  string_view result{reinterpret_cast<const char*>(data->data()), len};
  remove_prefix(data, len);
  return result;
}

optional<string_view> ReadUtf8String(SpanU8* data,
                                     Context& context,
                                     string_view desc) {
  auto string = ReadString(data, context, desc);
  if (string && !IsValidUtf8(*string)) {
    context.errors.OnError(*data, "Invalid UTF-8 encoding");
    return {};
  }
  return string;
}

optional<SymbolInfo> Read(SpanU8* data, Context& context, Tag<SymbolInfo>) {
  ErrorsContextGuard guard{context.errors, *data, "symbol info"};
  WASP_TRY_READ(kind, Read<SymbolInfoKind>(data, context));
  WASP_TRY_READ(encoded_flags, Read<u32>(data, context));
  WASP_TRY_DECODE(flags_opt, encoded_flags, SymbolInfoFlags,
                  "symbol info flags");
  auto flags = *flags_opt;
  switch (kind) {
    case SymbolInfoKind::Function:
    case SymbolInfoKind::Global:
    case SymbolInfoKind::Event: {
      WASP_TRY_READ(index, ReadIndex(data, context, "index"));
      optional<string_view> name;
      if (flags.undefined == SymbolInfo::Flags::Undefined::No ||
          flags.explicit_name == SymbolInfo::Flags::ExplicitName::Yes) {
        WASP_TRY_READ(name_, ReadString(data, context, "name"));
        name = name_;
      }
      return SymbolInfo{flags, SymbolInfo::Base{kind, index, name}};
    }

    case SymbolInfoKind::Data: {
      WASP_TRY_READ(name, ReadString(data, context, "name"));
      optional<SymbolInfo::Data::Defined> defined;
      if (flags.undefined == SymbolInfo::Flags::Undefined::No) {
        WASP_TRY_READ(index, ReadIndex(data, context, "segment index"));
        WASP_TRY_READ(offset, Read<u32>(data, context));
        WASP_TRY_READ(size, Read<u32>(data, context));
        defined = SymbolInfo::Data::Defined{index, offset, size};
      }
      return SymbolInfo{flags, SymbolInfo::Data{name, defined}};
    }

    case SymbolInfoKind::Section:
      WASP_TRY_READ(section, Read<u32>(data, context));
      return SymbolInfo{flags, SymbolInfo::Section{section}};
  }
  WASP_UNREACHABLE();
}

optional<SymbolInfoKind> Read(SpanU8* data,
                              Context& context,
                              Tag<SymbolInfoKind>) {
  ErrorsContextGuard guard{context.errors, *data, "symbol info kind"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, SymbolInfoKind, "symbol info kind");
  return decoded;
}

optional<Table> Read(SpanU8* data, Context& context, Tag<Table>) {
  ErrorsContextGuard guard{context.errors, *data, "table"};
  WASP_TRY_READ(table_type, Read<TableType>(data, context));
  return Table{table_type};
}

optional<TableType> Read(SpanU8* data, Context& context, Tag<TableType>) {
  ErrorsContextGuard guard{context.errors, *data, "table type"};
  WASP_TRY_READ(elemtype, Read<ElementType>(data, context));
  WASP_TRY_READ(limits, Read<Limits>(data, context));
  return TableType{limits, elemtype};
}

optional<TypeEntry> Read(SpanU8* data, Context& context, Tag<TypeEntry>) {
  ErrorsContextGuard guard{context.errors, *data, "type entry"};
  WASP_TRY_READ_CONTEXT(form, Read<u8>(data, context), "form");

  if (form != encoding::Type::Function) {
    context.errors.OnError(*data, format("Unknown type form: {}", form));
    return nullopt;
  }

  WASP_TRY_READ(function_type, Read<FunctionType>(data, context));
  return TypeEntry{std::move(function_type)};
}

optional<u32> Read(SpanU8* data, Context& context, Tag<u32>) {
  return ReadVarInt<u32>(data, context, "u32");
}

optional<u8> Read(SpanU8* data, Context& context, Tag<u8>) {
  if (data->size() < 1) {
    context.errors.OnError(*data, "Unable to read u8");
    return nullopt;
  }

  u8 result{(*data)[0]};
  remove_prefix(data, 1);
  return result;
}

optional<v128> Read(SpanU8* data, Context& context, Tag<v128>) {
  static_assert(sizeof(v128) == 16, "sizeof(v128) != 16");
  ErrorsContextGuard guard{context.errors, *data, "v128"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(v128), context));
  v128 result;
  memcpy(&result, bytes.data(), sizeof(v128));
  return result;
}

optional<ValueType> Read(SpanU8* data, Context& context, Tag<ValueType>) {
  ErrorsContextGuard guard{context.errors, *data, "value type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  auto decoded = encoding::ValueType::Decode(val, context.features);
  if (!decoded) {
    context.errors.OnError(*data, format("Unknown value type: {}", val));
    return nullopt;
  }
  return decoded;
}

}  // namespace wasp
}  // namespace wasp

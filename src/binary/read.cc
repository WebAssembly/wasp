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

#include <cassert>

#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/format.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/utf8.h"
#include "wasp/binary/encoding.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/read/location_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_var_int.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

OptAt<BlockType> Read(SpanU8* data, Context& context, Tag<BlockType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "block type"};
  LocationGuard guard{data};
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

OptAt<BrOnExnImmediate> Read(SpanU8* data,
                             Context& context,
                             Tag<BrOnExnImmediate>) {
  ErrorsContextGuard error_guard{context.errors, *data, "br_on_exn"};
  LocationGuard guard{data};
  WASP_TRY_READ(target, ReadIndex(data, context, "target"));
  WASP_TRY_READ(event_index, ReadIndex(data, context, "event index"));
  return MakeAt(guard.loc(), BrOnExnImmediate{target, event_index});
}

OptAt<BrTableImmediate> Read(SpanU8* data,
                             Context& context,
                             Tag<BrTableImmediate>) {
  ErrorsContextGuard error_guard{context.errors, *data, "br_table"};
  LocationGuard guard{data};
  WASP_TRY_READ(targets, ReadVector<Index>(data, context, "targets"));
  WASP_TRY_READ(default_target, ReadIndex(data, context, "default target"));
  return MakeAt(guard.loc(),
                BrTableImmediate{std::move(targets), default_target});
}

OptAt<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Context& context) {
  if (data->size() < N) {
    context.errors.OnError(*data, format("Unable to read {} bytes", N));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  remove_prefix(data, N);
  return MakeAt(result, result);
}

OptAt<SpanU8> ReadBytesExpected(SpanU8* data,
                                SpanU8 expected,
                                Context& context,
                                string_view desc) {
  ErrorsContextGuard error_guard{context.errors, *data, desc};
  LocationGuard guard{data};

  auto actual = ReadBytes(data, expected.size(), context);
  if (actual && **actual != expected) {
    context.errors.OnError(
        *data, format("Mismatch: expected {}, got {}", expected, *actual));
  }
  return actual;
}

OptAt<CallIndirectImmediate> Read(SpanU8* data,
                                  Context& context,
                                  Tag<CallIndirectImmediate>) {
  ErrorsContextGuard error_guard{context.errors, *data, "call_indirect"};
  LocationGuard guard{data};
  WASP_TRY_READ(index, ReadIndex(data, context, "type index"));
  if (context.features.reference_types_enabled()) {
    WASP_TRY_READ(table_index, ReadIndex(data, context, "table index"));
    return MakeAt(guard.loc(), CallIndirectImmediate{index, table_index});
  } else {
    WASP_TRY_READ(reserved, ReadReservedIndex(data, context));
    return MakeAt(guard.loc(), CallIndirectImmediate{index, reserved});
  }
}

OptAt<Index> ReadCheckLength(SpanU8* data,
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

OptAt<Code> Read(SpanU8* data, Context& context, Tag<Code>) {
  ErrorsContextGuard error_guard{context.errors, *data, "code"};
  LocationGuard guard{data};
  context.code_count++;
  WASP_TRY_READ(body_size, ReadLength(data, context));
  WASP_TRY_READ(body, ReadBytes(data, body_size, context));
  WASP_TRY_READ(locals, ReadVector<Locals>(&*body, context, "locals vector"));
  // Use updated body as Location (i.e. after reading locals).
  auto expression = MakeAt(body, Expression{*body});
  return MakeAt(guard.loc(), Code{std::move(locals), expression});
}

OptAt<ConstantExpression> Read(SpanU8* data,
                               Context& context,
                               Tag<ConstantExpression>) {
  ErrorsContextGuard error_guard{context.errors, *data, "constant expression"};
  LocationGuard guard{data};
  InstructionList instrs;
  while (true) {
    WASP_TRY_READ(instr, Read<Instruction>(data, context));
    if (instr->opcode == Opcode::End) {
      break;
    }
    instrs.push_back(instr);
  }
  return MakeAt(guard.loc(), ConstantExpression{instrs});
}

OptAt<CopyImmediate> Read(SpanU8* data,
                          Context& context,
                          Tag<CopyImmediate>,
                          BulkImmediateKind kind) {
  ErrorsContextGuard error_guard{context.errors, *data, "copy immediate"};
  LocationGuard guard{data};
  if (kind == BulkImmediateKind::Table &&
      context.features.reference_types_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, context, "dst index"));
    WASP_TRY_READ(src_index, ReadIndex(data, context, "src index"));
    return MakeAt(guard.loc(), CopyImmediate{dst_index, src_index});
  } else {
    WASP_TRY_READ(dst_reserved, ReadReservedIndex(data, context));
    WASP_TRY_READ(src_reserved, ReadReservedIndex(data, context));
    return MakeAt(guard.loc(), CopyImmediate{dst_reserved, src_reserved});
  }
}

OptAt<Index> ReadCount(SpanU8* data, Context& context) {
  return ReadCheckLength(data, context, "count", "Count");
}

OptAt<DataCount> Read(SpanU8* data, Context& context, Tag<DataCount>) {
  ErrorsContextGuard error_guard{context.errors, *data, "data count"};
  LocationGuard guard{data};
  WASP_TRY_READ(count, ReadIndex(data, context, "count"));
  context.declared_data_count = count;
  return MakeAt(guard.loc(), DataCount{count});
}

OptAt<DataSegment> Read(SpanU8* data, Context& context, Tag<DataSegment>) {
  ErrorsContextGuard error_guard{context.errors, *data, "data segment"};
  LocationGuard guard{data};
  context.data_count++;
  auto decoded = encoding::DecodedDataSegmentFlags::MVP();
  if (context.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, context, "flags"));
    WASP_TRY_DECODE(decoded_at, flags, DataSegmentFlags, "flags");
    decoded = *decoded_at;
  }

  At<Index> memory_index = 0u;
  if (!context.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(memory_index_, ReadIndex(data, context, "memory index"));
    memory_index = memory_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset, Read<ConstantExpression>(data, context),
                          "offset");
    WASP_TRY_READ(len, ReadLength(data, context));
    WASP_TRY_READ(init, ReadBytes(data, *len, context));
    return MakeAt(guard.loc(), DataSegment{memory_index, offset, init});
  } else {
    WASP_TRY_READ(len, ReadLength(data, context));
    WASP_TRY_READ(init, ReadBytes(data, len, context));
    return MakeAt(guard.loc(), DataSegment{init});
  }
}

OptAt<ElementExpression> Read(SpanU8* data,
                              Context& context,
                              Tag<ElementExpression>) {
  ErrorsContextGuard error_guard{context.errors, *data, "element expression"};
  LocationGuard guard{data};
  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(context.features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types proposal,
  // but their encoding is still used by the bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  Context new_context{new_features, context.errors};

  InstructionList instrs;
  while (true) {
    WASP_TRY_READ(instr, Read<Instruction>(data, new_context));
    if (instr->opcode == Opcode::End) {
      break;
    }
    instrs.push_back(instr);
  }
  return MakeAt(guard.loc(), ElementExpression{instrs});
}

OptAt<ElementSegment> Read(SpanU8* data,
                           Context& context,
                           Tag<ElementSegment>) {
  ErrorsContextGuard error_guard{context.errors, *data, "element segment"};
  LocationGuard guard{data};
  auto decoded = encoding::DecodedElemSegmentFlags::MVP();
  if (context.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, context, "flags"));
    WASP_TRY_DECODE_FEATURES(decoded_at, flags, ElemSegmentFlags, "flags",
                             context.features);
    decoded = *decoded_at;
  }

  At<Index> table_index{0u};
  if (!context.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(table_index_, ReadIndex(data, context, "table index"));
    table_index = table_index_;
  }

  optional<At<ConstantExpression>> offset;
  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset_, Read<ConstantExpression>(data, context),
                          "offset");
    offset = offset_;
  }

  if (decoded.has_expressions == encoding::HasExpressions::Yes) {
    At<ReferenceType> elemtype{ReferenceType::Funcref};
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(elemtype_, Read<ReferenceType>(data, context));
      elemtype = elemtype_;
    }
    WASP_TRY_READ(
        init, ReadVector<ElementExpression>(data, context, "initializers"));

    ElementListWithExpressions list{elemtype, init};
    if (decoded.segment_type == SegmentType::Active) {
      return MakeAt(guard.loc(), ElementSegment{table_index, *offset, list});
    } else {
      return MakeAt(guard.loc(), ElementSegment{decoded.segment_type, list});
    }
  } else {
    At<ExternalKind> kind{ExternalKind::Function};
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(kind_, Read<ExternalKind>(data, context));
      kind = kind_;
    }
    WASP_TRY_READ(init, ReadVector<Index>(data, context, "initializers"));

    ElementListWithIndexes list{kind, init};
    if (decoded.segment_type == SegmentType::Active) {
      return MakeAt(guard.loc(), ElementSegment{table_index, *offset, list});
    } else {
      return MakeAt(guard.loc(), ElementSegment{decoded.segment_type, list});
    }
  }
}

OptAt<ReferenceType> Read(SpanU8* data, Context& context, Tag<ReferenceType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "element type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, ReferenceType, "element type",
                           context.features);
  return decoded;
}

OptAt<Event> Read(SpanU8* data, Context& context, Tag<Event>) {
  ErrorsContextGuard error_guard{context.errors, *data, "event"};
  LocationGuard guard{data};
  WASP_TRY_READ(event_type, Read<EventType>(data, context));
  return MakeAt(guard.loc(), Event{event_type});
}

OptAt<EventAttribute> Read(SpanU8* data,
                           Context& context,
                           Tag<EventAttribute>) {
  ErrorsContextGuard error_guard{context.errors, *data, "event attribute"};
  WASP_TRY_READ(val, Read<u32>(data, context));
  WASP_TRY_DECODE(decoded, val, EventAttribute, "event attribute");
  return decoded;
}

OptAt<EventType> Read(SpanU8* data, Context& context, Tag<EventType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "event type"};
  LocationGuard guard{data};
  WASP_TRY_READ(attribute, Read<EventAttribute>(data, context));
  WASP_TRY_READ(type_index, ReadIndex(data, context, "type index"));
  return MakeAt(guard.loc(), EventType{attribute, type_index});
}

OptAt<Export> Read(SpanU8* data, Context& context, Tag<Export>) {
  ErrorsContextGuard error_guard{context.errors, *data, "export"};
  LocationGuard guard{data};
  WASP_TRY_READ(name, ReadUtf8String(data, context, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, context));
  WASP_TRY_READ(index, ReadIndex(data, context, "index"));
  return MakeAt(guard.loc(), Export{kind, name, index});
}

OptAt<ExternalKind> Read(SpanU8* data, Context& context, Tag<ExternalKind>) {
  ErrorsContextGuard error_guard{context.errors, *data, "external kind"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, ExternalKind, "external kind",
                           context.features);
  return decoded;
}

OptAt<f32> Read(SpanU8* data, Context& context, Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard error_guard{context.errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), context));
  f32 result;
  memcpy(&result, bytes->data(), sizeof(f32));
  return MakeAt(bytes.loc(), result);

}

OptAt<f64> Read(SpanU8* data, Context& context, Tag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  ErrorsContextGuard error_guard{context.errors, *data, "f64"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), context));
  f64 result;
  memcpy(&result, bytes->data(), sizeof(f64));
  return MakeAt(bytes.loc(), result);
}

OptAt<Function> Read(SpanU8* data, Context& context, Tag<Function>) {
  ErrorsContextGuard error_guard{context.errors, *data, "function"};
  LocationGuard guard{data};
  context.defined_function_count++;
  WASP_TRY_READ(type_index, ReadIndex(data, context, "type index"));
  return MakeAt(guard.loc(), Function{type_index});
}

OptAt<FunctionType> Read(SpanU8* data, Context& context, Tag<FunctionType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "function type"};
  LocationGuard guard{data};
  WASP_TRY_READ(param_types,
                      ReadVector<ValueType>(data, context, "param types"));
  WASP_TRY_READ(result_types,
                      ReadVector<ValueType>(data, context, "result types"));
  return MakeAt(guard.loc(),
                FunctionType{std::move(param_types), std::move(result_types)});
}

OptAt<Global> Read(SpanU8* data, Context& context, Tag<Global>) {
  ErrorsContextGuard error_guard{context.errors, *data, "global"};
  LocationGuard guard{data};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, context));
  WASP_TRY_READ(init_expr, Read<ConstantExpression>(data, context));
  return MakeAt(guard.loc(), Global{global_type, std::move(init_expr)});
}

OptAt<GlobalType> Read(SpanU8* data, Context& context, Tag<GlobalType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "global type"};
  LocationGuard guard{data};
  WASP_TRY_READ(type, Read<ValueType>(data, context));
  WASP_TRY_READ(mut, Read<Mutability>(data, context));
  return MakeAt(guard.loc(), GlobalType{type, mut});
}

OptAt<Import> Read(SpanU8* data, Context& context, Tag<Import>) {
  ErrorsContextGuard error_guard{context.errors, *data, "import"};
  LocationGuard guard{data};
  WASP_TRY_READ(module, ReadUtf8String(data, context, "module name"));
  WASP_TRY_READ(name, ReadUtf8String(data, context, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, context));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index, ReadIndex(data, context, "function index"));
      return MakeAt(guard.loc(), Import{module, name, type_index});
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, context));
      return MakeAt(guard.loc(), Import{module, name, table_type});
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, context));
      return MakeAt(guard.loc(), Import{module, name, memory_type});
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, context));
      return MakeAt(guard.loc(), Import{module, name, global_type});
    }
    case ExternalKind::Event: {
      WASP_TRY_READ(event_type, Read<EventType>(data, context));
      return MakeAt(guard.loc(), Import{module, name, event_type});
    }
  }
  WASP_UNREACHABLE();
}

OptAt<Index> ReadIndex(SpanU8* data, Context& context, string_view desc) {
  return ReadVarInt<Index>(data, context, desc);
}

OptAt<InitImmediate> Read(SpanU8* data,
                          Context& context,
                          Tag<InitImmediate>,
                          BulkImmediateKind kind) {
  ErrorsContextGuard error_guard{context.errors, *data, "init immediate"};
  LocationGuard guard{data};
  WASP_TRY_READ(segment_index, ReadIndex(data, context, "segment index"));
  if (kind == BulkImmediateKind::Table &&
      context.features.reference_types_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, context, "table index"));
    return MakeAt(guard.loc(), InitImmediate{segment_index, dst_index});
  } else {
    WASP_TRY_READ(reserved, ReadReservedIndex(data, context));
    return MakeAt(guard.loc(), InitImmediate{segment_index, reserved});
  }
}

OptAt<Instruction> Read(SpanU8* data, Context& context, Tag<Instruction>) {
  LocationGuard guard{data};
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
    case Opcode::I8X16Abs:
    case Opcode::I16X8Abs:
    case Opcode::I32X4Abs:
      return MakeAt(guard.loc(), Instruction{opcode});

    // Reference type immediate.
    case Opcode::RefNull:
    case Opcode::RefIsNull: {
      WASP_TRY_READ(type, Read<ReferenceType>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, type});
    }

    // Block type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If:
    case Opcode::Try: {
      WASP_TRY_READ(type, Read<BlockType>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, type});
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
      return MakeAt(guard.loc(), Instruction{opcode, index});
    }

    // Index, Index immediates.
    case Opcode::BrOnExn: {
      WASP_TRY_READ(immediate, Read<BrOnExnImmediate>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, std::move(immediate)});
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect: {
      WASP_TRY_READ(immediate, Read<CallIndirectImmediate>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
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
    case Opcode::MemoryAtomicNotify:
    case Opcode::MemoryAtomicWait32:
    case Opcode::MemoryAtomicWait64:
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
      return MakeAt(guard.loc(), Instruction{opcode, memarg});
    }

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill: {
      WASP_TRY_READ(reserved, ReadReserved(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, reserved});
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, context), "i32 constant");
      return MakeAt(guard.loc(), Instruction{opcode, value});
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, context), "i64 constant");
      return MakeAt(guard.loc(), Instruction{opcode, value});
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, context), "f32 constant");
      return MakeAt(guard.loc(), Instruction{opcode, value});
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, context), "f64 constant");
      return MakeAt(guard.loc(), Instruction{opcode, value});
    }

    case Opcode::V128Const: {
      WASP_TRY_READ_CONTEXT(value, Read<v128>(data, context), "v128 constant");
      return MakeAt(guard.loc(), Instruction{opcode, value});
    }

    // Reserved, Index immediates.
    case Opcode::MemoryInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, context,
                                                   BulkImmediateKind::Memory));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }
    case Opcode::TableInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, context,
                                                   BulkImmediateKind::Table));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }

    // Reserved, reserved immediates.
    case Opcode::MemoryCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, context,
                                                   BulkImmediateKind::Memory));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }
    case Opcode::TableCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, context,
                                                   BulkImmediateKind::Table));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }

    // Shuffle immediate.
    case Opcode::V8X16Shuffle: {
      WASP_TRY_READ(immediate, Read<ShuffleImmediate>(data, context));
      return MakeAt(guard.loc(), Instruction{opcode, immediate});
    }

    // Select immediate.
    case Opcode::SelectT: {
      LocationGuard immediate_guard{data};
      WASP_TRY_READ(immediate, ReadVector<ValueType>(data, context, "types"));
      return MakeAt(
          guard.loc(),
          Instruction{opcode, MakeAt(immediate_guard.loc(), immediate)});
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
      return MakeAt(guard.loc(), Instruction{opcode, lane});
    }
  }
  WASP_UNREACHABLE();
}

OptAt<Index> ReadLength(SpanU8* data, Context& context) {
  return ReadCheckLength(data, context, "length", "Length");
}

OptAt<Limits> Read(SpanU8* data, Context& context, Tag<Limits>) {
  ErrorsContextGuard error_guard{context.errors, *data, "limits"};
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, context), "flags");
  WASP_TRY_DECODE_FEATURES(decoded, flags, LimitsFlags, "flags value",
                           context.features);

  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, context), "min");

  OptAt<u32> max;
  if (decoded->has_max == encoding::HasMax::Yes) {
    WASP_TRY_READ_CONTEXT(max_, Read<u32>(data, context), "max");
    max = max_;
  }
  return MakeAt(guard.loc(),
                Limits{min, max, MakeAt(flags.loc(), decoded->shared)});
}

OptAt<Locals> Read(SpanU8* data, Context& context, Tag<Locals>) {
  ErrorsContextGuard error_guard{context.errors, *data, "locals"};
  LocationGuard guard{data};
  WASP_TRY_READ(count, ReadIndex(data, context, "count"));
  WASP_TRY_READ_CONTEXT(type, Read<ValueType>(data, context), "type");
  return MakeAt(guard.loc(), Locals{count, type});
}

OptAt<MemArgImmediate> Read(SpanU8* data,
                            Context& context,
                            Tag<MemArgImmediate>) {
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, context), "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, context), "offset");
  return MakeAt(guard.loc(), MemArgImmediate{align_log2, offset});
}

OptAt<Memory> Read(SpanU8* data, Context& context, Tag<Memory>) {
  ErrorsContextGuard error_guard{context.errors, *data, "memory"};
  WASP_TRY_READ(memory_type, Read<MemoryType>(data, context));
  return MakeAt(memory_type.loc(), Memory{memory_type});
}

OptAt<MemoryType> Read(SpanU8* data, Context& context, Tag<MemoryType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, context));
  return MakeAt(limits.loc(), MemoryType{limits});
}

OptAt<Mutability> Read(SpanU8* data, Context& context, Tag<Mutability>) {
  ErrorsContextGuard error_guard{context.errors, *data, "mutability"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE(decoded, val, Mutability, "mutability");
  return decoded;
}

OptAt<Opcode> Read(SpanU8* data, Context& context, Tag<Opcode>) {
  ErrorsContextGuard error_guard{context.errors, *data, "opcode"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, context));

  if (encoding::Opcode::IsPrefixByte(*val, context.features)) {
    WASP_TRY_READ(code, Read<u32>(data, context));
    auto decoded = encoding::Opcode::Decode(val, code, context.features);
    if (!decoded) {
      context.errors.OnError(*data, format("Unknown opcode: {} {}", val, code));
      return nullopt;
    }
    return MakeAt(guard.loc(), *decoded);
  } else {
    WASP_TRY_DECODE_FEATURES(decoded, val, Opcode, "opcode", context.features);
    return decoded;
  }
}

OptAt<u8> ReadReserved(SpanU8* data, Context& context) {
  ErrorsContextGuard error_guard{context.errors, *data, "reserved"};
  LocationGuard guard{data};
  WASP_TRY_READ(reserved, Read<u8>(data, context));
  if (reserved != 0) {
    context.errors.OnError(
        *data, format("Expected reserved byte 0, got {}", reserved));
    return nullopt;
  }
  return reserved;
}

OptAt<Index> ReadReservedIndex(SpanU8* data, Context& context) {
  auto result_u8 = ReadReserved(data, context);
  if (!result_u8) {
    return nullopt;
  }
  return MakeAt(result_u8->loc(), Index(**result_u8));
}

OptAt<s32> Read(SpanU8* data, Context& context, Tag<s32>) {
  return ReadVarInt<s32>(data, context, "s32");
}

OptAt<s64> Read(SpanU8* data, Context& context, Tag<s64>) {
  return ReadVarInt<s64>(data, context, "s64");
}

OptAt<Section> Read(SpanU8* data, Context& context, Tag<Section>) {
  ErrorsContextGuard error_guard{context.errors, *data, "section"};
  LocationGuard guard{data};
  WASP_TRY_READ(id, Read<SectionId>(data, context));
  WASP_TRY_READ(length, ReadLength(data, context));
  WASP_TRY_READ(bytes, ReadBytes(data, length, context));

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name,
                  ReadUtf8String(&*bytes, context, "custom section name"));
    return MakeAt(guard.loc(),
                  Section{MakeAt(guard.loc(), CustomSection{name, *bytes})});
  } else {
    if (context.last_section_id && *context.last_section_id >= id) {
      context.errors.OnError(
          *data, format("Section out of order: {} cannot occur after {}", id,
                        *context.last_section_id));
    }
    context.last_section_id = id;

    return MakeAt(guard.loc(),
                  Section{MakeAt(guard.loc(), KnownSection{id, *bytes})});
  }
}

OptAt<SectionId> Read(SpanU8* data, Context& context, Tag<SectionId>) {
  ErrorsContextGuard guard{context.errors, *data, "section id"};
  WASP_TRY_READ(val, Read<u32>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, SectionId, "section id",
                           context.features);
  return decoded;
}

OptAt<ShuffleImmediate> Read(SpanU8* data,
                             Context& context,
                             Tag<ShuffleImmediate>) {
  ErrorsContextGuard error_guard{context.errors, *data, "shuffle immediate"};
  LocationGuard guard{data};
  ShuffleImmediate immediate;
  for (int i = 0; i < 16; ++i) {
    WASP_TRY_READ(byte, Read<u8>(data, context));
    immediate[i] = byte;
  }
  return MakeAt(guard.loc(), immediate);
}

OptAt<Start> Read(SpanU8* data, Context& context, Tag<Start>) {
  ErrorsContextGuard guard{context.errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, context, "function index"));
  return MakeAt(index.loc(), Start{index});
}

OptAt<string_view> ReadString(SpanU8* data,
                              Context& context,
                              string_view desc) {
  ErrorsContextGuard error_guard{context.errors, *data, desc};
  LocationGuard guard{data};
  WASP_TRY_READ(len, ReadLength(data, context));
  string_view result{reinterpret_cast<const char*>(data->data()), len};
  remove_prefix(data, len);
  return MakeAt(guard.loc(), result);
}

OptAt<string_view> ReadUtf8String(SpanU8* data,
                                  Context& context,
                                  string_view desc) {
  auto string = ReadString(data, context, desc);
  if (string && !IsValidUtf8(*string)) {
    context.errors.OnError(*data, "Invalid UTF-8 encoding");
    return {};
  }
  return string;
}

OptAt<Table> Read(SpanU8* data, Context& context, Tag<Table>) {
  ErrorsContextGuard error_guard{context.errors, *data, "table"};
  WASP_TRY_READ(table_type, Read<TableType>(data, context));
  return MakeAt(table_type.loc(), Table{table_type});
}

OptAt<TableType> Read(SpanU8* data, Context& context, Tag<TableType>) {
  ErrorsContextGuard error_guard{context.errors, *data, "table type"};
  LocationGuard guard{data};
  WASP_TRY_READ(elemtype, Read<ReferenceType>(data, context));
  WASP_TRY_READ(limits, Read<Limits>(data, context));
  return MakeAt(guard.loc(), TableType{std::move(limits), elemtype});
}

OptAt<TypeEntry> Read(SpanU8* data, Context& context, Tag<TypeEntry>) {
  ErrorsContextGuard error_guard{context.errors, *data, "type entry"};
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(form, Read<u8>(data, context), "form");

  if (form != encoding::Type::Function) {
    context.errors.OnError(*data, format("Unknown type form: {}", form));
    return nullopt;
  }

  WASP_TRY_READ(function_type, Read<FunctionType>(data, context));
  return MakeAt(guard.loc(), TypeEntry{std::move(function_type)});
}

OptAt<u32> Read(SpanU8* data, Context& context, Tag<u32>) {
  return ReadVarInt<u32>(data, context, "u32");
}

OptAt<u8> Read(SpanU8* data, Context& context, Tag<u8>) {
  if (data->size() < 1) {
    context.errors.OnError(*data, "Unable to read u8");
    return nullopt;
  }

  Location loc = data->subspan(0, 1);
  u8 result{(*data)[0]};
  remove_prefix(data, 1);
  return MakeAt(loc, result);
}

OptAt<v128> Read(SpanU8* data, Context& context, Tag<v128>) {
  static_assert(sizeof(v128) == 16, "sizeof(v128) != 16");
  ErrorsContextGuard guard{context.errors, *data, "v128"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(v128), context));
  v128 result;
  memcpy(&result, bytes->data(), sizeof(v128));
  return MakeAt(bytes.loc(), result);
}

OptAt<ValueType> Read(SpanU8* data, Context& context, Tag<ValueType>) {
  ErrorsContextGuard guard{context.errors, *data, "value type"};
  WASP_TRY_READ(val, Read<u8>(data, context));
  WASP_TRY_DECODE_FEATURES(decoded, val, ValueType, "value type",
                           context.features);
  return decoded;
}

}  // namespace binary
}  // namespace wasp

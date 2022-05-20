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
#include <limits>

#include "wasp/base/errors.h"
#include "wasp/base/errors_context_guard.h"
#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/utf8.h"
#include "wasp/binary/encoding.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/read/location_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read_var_int.h"
#include "wasp/binary/read/read_vector.h"

#include "wasp/base/concat.h"

namespace wasp::binary {

OptAt<ArrayType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<ArrayType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "array type"};
  LocationGuard guard{data};
  WASP_TRY_READ(field, Read<FieldType>(data, ctx));
  return At{guard.range(data), ArrayType{field}};
}

OptAt<BlockType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<BlockType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "block type"};
  LocationGuard guard{data};

  WASP_TRY_READ(val, PeekU8(data, ctx));

  if (ctx.features.multi_value_enabled() && encoding::BlockType::IsS32(val)) {
    WASP_TRY_READ(val, Read<s32>(data, ctx));
    WASP_TRY_DECODE_FEATURES(decoded, val, BlockType, "block type",
                             ctx.features);
    return decoded;
  } else if (encoding::BlockType::IsBare(val)) {
    data->remove_prefix(1);
    WASP_TRY_DECODE_FEATURES(decoded, val, BlockType, "block type",
                             ctx.features);
    return decoded;
  } else {
    WASP_TRY_READ(value_type, Read<ValueType>(data, ctx));
    return At{guard.range(data), BlockType{value_type}};
  }
}

OptAt<BrOnCastImmediate> Read(SpanU8* data,
                              ReadCtx& ctx,
                              ReadTag<BrOnCastImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "br_on_cast"};
  LocationGuard guard{data};
  WASP_TRY_READ(target, ReadIndex(data, ctx, "target"));
  WASP_TRY_READ(types, Read<HeapType2Immediate>(data, ctx));
  return At{guard.range(data), BrOnCastImmediate{target, types}};
}

OptAt<BrTableImmediate> Read(SpanU8* data,
                             ReadCtx& ctx,
                             ReadTag<BrTableImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "br_table"};
  LocationGuard guard{data};
  WASP_TRY_READ(targets, ReadVector<Index>(data, ctx, "targets"));
  WASP_TRY_READ(default_target, ReadIndex(data, ctx, "default target"));
  return At{guard.range(data),
            BrTableImmediate{std::move(targets), default_target}};
}

OptAt<SpanU8> ReadBytes(SpanU8* data, span_extent_t N, ReadCtx& ctx) {
  if (data->size() < N) {
    ctx.errors.OnError(*data, concat("Unable to read ", N, " bytes"));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  data->remove_prefix(N);
  return At{result, result};
}

OptAt<SpanU8> ReadBytesExpected(SpanU8* data,
                                SpanU8 expected,
                                ReadCtx& ctx,
                                string_view desc) {
  ErrorsContextGuard error_guard{ctx.errors, *data, desc};
  LocationGuard guard{data};

  auto actual = ReadBytes(data, expected.size(), ctx);
  if (actual && **actual != expected) {
    ctx.errors.OnError(actual->loc(), concat("Mismatch: expected ", expected,
                                             ", got ", *actual));
    return nullopt;
  }
  return actual;
}

OptAt<CallIndirectImmediate> Read(SpanU8* data,
                                  ReadCtx& ctx,
                                  ReadTag<CallIndirectImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "call_indirect"};
  LocationGuard guard{data};
  WASP_TRY_READ(index, ReadIndex(data, ctx, "type index"));
  if (ctx.features.reference_types_enabled()) {
    WASP_TRY_READ(table_index, ReadIndex(data, ctx, "table index"));
    return At{guard.range(data), CallIndirectImmediate{index, table_index}};
  } else {
    WASP_TRY_READ(reserved, ReadReservedIndex(data, ctx));
    return At{guard.range(data), CallIndirectImmediate{index, reserved}};
  }
}

OptAt<Index> ReadCheckLength(SpanU8* data,
                             ReadCtx& ctx,
                             string_view context_name,
                             string_view error_name) {
  WASP_TRY_READ(count, ReadIndex(data, ctx, context_name));

  // There should be at least one byte per count, so if the data is smaller
  // than that, the module must be malformed.
  if (count > data->size()) {
    ctx.errors.OnError(
        count.loc(),
        concat(error_name, " extends past end: ", count, " > ", data->size()));
    return nullopt;
  }

  return count;
}

OptAt<Code> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Code>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "code"};
  LocationGuard guard{data};
  ctx.code_count++;
  ctx.local_count = 0;
  WASP_TRY_READ(body_size, ReadLength(data, ctx));
  WASP_TRY_READ(body, ReadBytes(data, body_size, ctx));
  WASP_TRY_READ(locals, ReadVector<Locals>(&*body, ctx, "locals vector"));
  // Use updated body as Location (i.e. after reading locals).
  auto expression = At{body, Expression{*body}};
  return At{guard.range(data), Code{std::move(locals), expression}};
}

OptAt<ConstantExpression> Read(SpanU8* data,
                               ReadCtx& ctx,
                               ReadTag<ConstantExpression>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "constant expression"};
  WASP_TRY_READ(instrs, Read<InstructionList>(data, ctx));
  return At{instrs.loc(), ConstantExpression{*instrs}};
}

OptAt<CopyImmediate> Read(SpanU8* data,
                          ReadCtx& ctx,
                          ReadTag<CopyImmediate>,
                          BulkImmediateKind kind) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "copy immediate"};
  LocationGuard guard{data};
  if ((kind == BulkImmediateKind::Table &&
       ctx.features.reference_types_enabled()) ||
      (kind == BulkImmediateKind::Memory &&
       ctx.features.multi_memory_enabled())) {
    WASP_TRY_READ(dst_index, ReadIndex(data, ctx, "dst index"));
    WASP_TRY_READ(src_index, ReadIndex(data, ctx, "src index"));
    return At{guard.range(data), CopyImmediate{dst_index, src_index}};
  } else {
    WASP_TRY_READ(dst_reserved, ReadReservedIndex(data, ctx));
    WASP_TRY_READ(src_reserved, ReadReservedIndex(data, ctx));
    return At{guard.range(data), CopyImmediate{dst_reserved, src_reserved}};
  }
}

OptAt<Index> ReadCount(SpanU8* data, ReadCtx& ctx) {
  return ReadCheckLength(data, ctx, "count", "Count");
}

OptAt<DataCount> Read(SpanU8* data, ReadCtx& ctx, ReadTag<DataCount>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "data count"};
  LocationGuard guard{data};
  WASP_TRY_READ(count, ReadIndex(data, ctx, "count"));
  ctx.declared_data_count = count;
  return At{guard.range(data), DataCount{count}};
}

OptAt<DataSegment> Read(SpanU8* data, ReadCtx& ctx, ReadTag<DataSegment>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "data segment"};
  LocationGuard guard{data};
  ctx.data_count++;
  auto decoded = encoding::DecodedDataSegmentFlags::MVP();
  if (ctx.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, ctx, "flags"));
    WASP_TRY_DECODE(decoded_at, flags, DataSegmentFlags, "flags");
    decoded = *decoded_at;
  }

  At<Index> memory_index = 0u;
  if (!ctx.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(memory_index_, ReadIndex(data, ctx, "memory index"));
    memory_index = memory_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset, Read<ConstantExpression>(data, ctx),
                          "offset");
    WASP_TRY_READ(len, ReadLength(data, ctx));
    WASP_TRY_READ(init, ReadBytes(data, *len, ctx));
    return At{guard.range(data), DataSegment{memory_index, offset, init}};
  } else {
    WASP_TRY_READ(len, ReadLength(data, ctx));
    WASP_TRY_READ(init, ReadBytes(data, len, ctx));
    return At{guard.range(data), DataSegment{init}};
  }
}

OptAt<DefinedType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<DefinedType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "defined type"};
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(form, Read<u8>(data, ctx), "form");

  switch (form) {
    case encoding::DefinedType::Function: {
      WASP_TRY_READ(function_type, Read<FunctionType>(data, ctx));
      return At{guard.range(data), DefinedType{std::move(function_type)}};
    }

    case encoding::DefinedType::Struct: {
      WASP_TRY_READ(struct_type, Read<StructType>(data, ctx));
      return At{guard.range(data), DefinedType{std::move(struct_type)}};
    }

    case encoding::DefinedType::Array: {
      WASP_TRY_READ(array_type, Read<ArrayType>(data, ctx));
      return At{guard.range(data), DefinedType{std::move(array_type)}};
    }

    default:
      ctx.errors.OnError(form.loc(), concat("Unknown type form: ", form));
      return nullopt;
  }
}

OptAt<ElementExpression> Read(SpanU8* data,
                              ReadCtx& ctx,
                              ReadTag<ElementExpression>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "element expression"};
  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(ctx.features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types proposal,
  // but their encoding is still used by the bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  ReadCtx new_context{new_features, ctx.errors};

  WASP_TRY_READ(instrs, Read<InstructionList>(data, new_context));
  return At{instrs.loc(), ElementExpression{*instrs}};
}

OptAt<ElementSegment> Read(SpanU8* data, ReadCtx& ctx, ReadTag<ElementSegment>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "element segment"};
  LocationGuard guard{data};
  auto decoded = encoding::DecodedElemSegmentFlags::MVP();
  if (ctx.features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, ctx, "flags"));
    WASP_TRY_DECODE_FEATURES(decoded_at, flags, ElemSegmentFlags, "flags",
                             ctx.features);
    decoded = *decoded_at;
  }

  At<Index> table_index{0u};
  if (!ctx.features.bulk_memory_enabled() ||
      decoded.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    WASP_TRY_READ(table_index_, ReadIndex(data, ctx, "table index"));
    table_index = table_index_;
  }

  optional<At<ConstantExpression>> offset;
  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(offset_, Read<ConstantExpression>(data, ctx),
                          "offset");
    offset = offset_;
  }

  if (decoded.has_expressions == encoding::HasExpressions::Yes) {
    At<ReferenceType> elemtype{ReferenceType{ReferenceKind::Funcref}};
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(elemtype_, Read<ReferenceType>(data, ctx));
      elemtype = elemtype_;
    }
    WASP_TRY_READ(init,
                  ReadVector<ElementExpression>(data, ctx, "initializers"));

    ElementListWithExpressions list{elemtype, init};
    if (decoded.segment_type == SegmentType::Active) {
      return At{guard.range(data), ElementSegment{table_index, *offset, list}};
    } else {
      return At{guard.range(data), ElementSegment{decoded.segment_type, list}};
    }
  } else {
    At<ExternalKind> kind{ExternalKind::Function};
    if (!decoded.is_legacy_active()) {
      WASP_TRY_READ(kind_, Read<ExternalKind>(data, ctx));
      kind = kind_;
    }
    WASP_TRY_READ(init, ReadVector<Index>(data, ctx, "initializers"));

    ElementListWithIndexes list{kind, init};
    if (decoded.segment_type == SegmentType::Active) {
      return At{guard.range(data), ElementSegment{table_index, *offset, list}};
    } else {
      return At{guard.range(data), ElementSegment{decoded.segment_type, list}};
    }
  }
}

OptAt<RefType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<RefType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "ref type"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  WASP_TRY_DECODE_FEATURES(null_, val, RefType, "ref type", ctx.features);
  WASP_TRY_READ(heap_type, Read<HeapType>(data, ctx));
  return At{guard.range(data), RefType{heap_type, null_}};
}

OptAt<ReferenceType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<ReferenceType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "reference type"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, PeekU8(data, ctx));

  if (encoding::RefType::Is(*val)) {
    WASP_TRY_READ(ref_type, Read<RefType>(data, ctx));
    return At{ref_type.loc(), ReferenceType{ref_type}};
  } else {
    data->remove_prefix(1);
    WASP_TRY_DECODE_FEATURES(decoded, val, ReferenceKind, "reference type",
                             ctx.features);
    return At{decoded.loc(), ReferenceType{decoded}};
  }
}

OptAt<Rtt> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Rtt>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "rtt"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  if (encoding::Rtt::Is(val)) {
    WASP_TRY_READ(depth, ReadIndex(data, ctx, "depth"));
    WASP_TRY_READ(type, Read<HeapType>(data, ctx));
    return At{guard.range(data), Rtt{depth, type}};
  } else {
    ctx.errors.OnError(val.loc(), concat("Unknown rtt code: ", val));
    return nullopt;
  }
}

OptAt<RttSubImmediate> Read(SpanU8* data,
                            ReadCtx& ctx,
                            ReadTag<RttSubImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "rtt sub immediate"};
  LocationGuard guard{data};
  WASP_TRY_READ(depth, ReadIndex(data, ctx, "depth"));
  WASP_TRY_READ(types, Read<HeapType2Immediate>(data, ctx));
  return At{guard.range(data), RttSubImmediate{depth, types}};
}

OptAt<Tag> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Tag>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "tag"};
  LocationGuard guard{data};
  WASP_TRY_READ(tag_type, Read<TagType>(data, ctx));
  return At{guard.range(data), Tag{tag_type}};
}

OptAt<TagAttribute> Read(SpanU8* data, ReadCtx& ctx, ReadTag<TagAttribute>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "tag attribute"};
  WASP_TRY_READ(val, Read<u32>(data, ctx));
  WASP_TRY_DECODE(decoded, val, TagAttribute, "tag attribute");
  return decoded;
}

OptAt<TagType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<TagType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "tag type"};
  LocationGuard guard{data};
  WASP_TRY_READ(attribute, Read<TagAttribute>(data, ctx));
  WASP_TRY_READ(type_index, ReadIndex(data, ctx, "type index"));
  return At{guard.range(data), TagType{attribute, type_index}};
}

OptAt<Export> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Export>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "export"};
  LocationGuard guard{data};
  WASP_TRY_READ(name, ReadUtf8String(data, ctx, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, ctx));
  WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
  return At{guard.range(data), Export{kind, name, index}};
}

OptAt<ExternalKind> Read(SpanU8* data, ReadCtx& ctx, ReadTag<ExternalKind>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "external kind"};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  WASP_TRY_DECODE_FEATURES(decoded, val, ExternalKind, "external kind",
                           ctx.features);
  return decoded;
}

OptAt<f32> Read(SpanU8* data, ReadCtx& ctx, ReadTag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard error_guard{ctx.errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), ctx));
  f32 result;
  memcpy(&result, bytes->data(), sizeof(f32));
  return At{bytes.loc(), result};
}

OptAt<f64> Read(SpanU8* data, ReadCtx& ctx, ReadTag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  ErrorsContextGuard error_guard{ctx.errors, *data, "f64"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), ctx));
  f64 result;
  memcpy(&result, bytes->data(), sizeof(f64));
  return At{bytes.loc(), result};
}

OptAt<FieldType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<FieldType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "field_type"};
  LocationGuard guard{data};
  WASP_TRY_READ(type, Read<StorageType>(data, ctx));
  WASP_TRY_READ(mut, Read<Mutability>(data, ctx));
  return At{guard.range(data), FieldType{type, mut}};
}

OptAt<FuncBindImmediate> Read(SpanU8* data,
                              ReadCtx& ctx,
                              ReadTag<FuncBindImmediate>) {
  LocationGuard guard{data};
  WASP_TRY_READ(index, ReadIndex(data, ctx, "func index"));
  return At{guard.range(data), FuncBindImmediate{index}};
}

OptAt<Function> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Function>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "function"};
  LocationGuard guard{data};
  ctx.defined_function_count++;
  WASP_TRY_READ(type_index, ReadIndex(data, ctx, "type index"));
  return At{guard.range(data), Function{type_index}};
}

OptAt<FunctionType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<FunctionType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "function type"};
  LocationGuard guard{data};
  WASP_TRY_READ(param_types, ReadVector<ValueType>(data, ctx, "param types"));
  WASP_TRY_READ(result_types, ReadVector<ValueType>(data, ctx, "result types"));
  return At{guard.range(data),
            FunctionType{std::move(param_types), std::move(result_types)}};
}

OptAt<Global> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Global>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "global"};
  LocationGuard guard{data};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, ctx));
  WASP_TRY_READ(init_expr, Read<ConstantExpression>(data, ctx));
  return At{guard.range(data), Global{global_type, std::move(init_expr)}};
}

OptAt<GlobalType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<GlobalType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "global type"};
  LocationGuard guard{data};
  WASP_TRY_READ(type, Read<ValueType>(data, ctx));
  WASP_TRY_READ(mut, Read<Mutability>(data, ctx));
  return At{guard.range(data), GlobalType{type, mut}};
}

OptAt<HeapType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<HeapType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "heap type"};
  WASP_TRY_READ(val, PeekU8(data, ctx));
  if (encoding::HeapKind::Is(val)) {
    data->remove_prefix(1);
    WASP_TRY_DECODE_FEATURES(decoded, val, HeapKind, "heap kind", ctx.features);
    return At{decoded.loc(), HeapType{decoded}};
  } else {
    WASP_TRY_READ(val, Read<s32>(data, ctx));
    return At{val.loc(), HeapType{At{val.loc(), Index(*val)}}};
  }
}

OptAt<HeapType2Immediate> Read(SpanU8* data,
                               ReadCtx& ctx,
                               ReadTag<HeapType2Immediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "heap type 2"};
  LocationGuard guard{data};
  WASP_TRY_READ(parent, Read<HeapType>(data, ctx));
  WASP_TRY_READ(child, Read<HeapType>(data, ctx));
  return At{guard.range(data), HeapType2Immediate{parent, child}};
}

OptAt<Import> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Import>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "import"};
  LocationGuard guard{data};
  WASP_TRY_READ(module, ReadUtf8String(data, ctx, "module name"));
  WASP_TRY_READ(name, ReadUtf8String(data, ctx, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, ctx));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index, ReadIndex(data, ctx, "function index"));
      return At{guard.range(data), Import{module, name, type_index}};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, ctx));
      return At{guard.range(data), Import{module, name, table_type}};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, ctx));
      return At{guard.range(data), Import{module, name, memory_type}};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, ctx));
      return At{guard.range(data), Import{module, name, global_type}};
    }
    case ExternalKind::Tag: {
      WASP_TRY_READ(tag_type, Read<TagType>(data, ctx));
      return At{guard.range(data), Import{module, name, tag_type}};
    }
  }
  WASP_UNREACHABLE();
}

OptAt<Index> ReadIndex(SpanU8* data, ReadCtx& ctx, string_view desc) {
  return ReadVarInt<Index>(data, ctx, desc);
}

OptAt<InitImmediate> Read(SpanU8* data,
                          ReadCtx& ctx,
                          ReadTag<InitImmediate>,
                          BulkImmediateKind kind) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "init immediate"};
  LocationGuard guard{data};
  WASP_TRY_READ(segment_index, ReadIndex(data, ctx, "segment index"));
  if (kind == BulkImmediateKind::Table &&
      ctx.features.reference_types_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, ctx, "table index"));
    return At{guard.range(data), InitImmediate{segment_index, dst_index}};
  } else if (kind == BulkImmediateKind::Memory &&
             ctx.features.multi_memory_enabled()) {
    WASP_TRY_READ(dst_index, ReadIndex(data, ctx, "memory index"));
    return At{guard.range(data), InitImmediate{segment_index, dst_index}};
  } else {
    WASP_TRY_READ(reserved, ReadReservedIndex(data, ctx));
    return At{guard.range(data), InitImmediate{segment_index, reserved}};
  }
}

bool RequireDataCountSection(ReadCtx& ctx, const At<Opcode>& opcode) {
  if (!ctx.declared_data_count) {
    ctx.errors.OnError(
        opcode.loc(),
        concat(*opcode, " instruction requires a data count section"));
    return false;
  }
  return true;
}

OptAt<Instruction> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Instruction>) {
  LocationGuard guard{data};
  WASP_TRY_READ(opcode, Read<Opcode>(data, ctx));

  if (ctx.seen_final_end) {
    ctx.errors.OnError(opcode.loc(), concat("Unexpected ", *opcode,
                                            " instruction after 'end'"));
    return nullopt;
  }

  switch (opcode) {
    // No immediates:
    case Opcode::Unreachable:
    case Opcode::Nop:
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
    case Opcode::RefIsNull:
    case Opcode::I8X16Swizzle:
    case Opcode::I8X16Splat:
    case Opcode::I16X8Splat:
    case Opcode::I32X4Splat:
    case Opcode::I64X2Splat:
    case Opcode::F32X4Splat:
    case Opcode::F64X2Splat:
    case Opcode::I8X16Eq:
    case Opcode::I8X16Ne:
    case Opcode::I8X16LtS:
    case Opcode::I8X16LtU:
    case Opcode::I8X16GtS:
    case Opcode::I8X16GtU:
    case Opcode::I8X16LeS:
    case Opcode::I8X16LeU:
    case Opcode::I8X16GeS:
    case Opcode::I8X16GeU:
    case Opcode::I16X8Eq:
    case Opcode::I16X8Ne:
    case Opcode::I16X8LtS:
    case Opcode::I16X8LtU:
    case Opcode::I16X8GtS:
    case Opcode::I16X8GtU:
    case Opcode::I16X8LeS:
    case Opcode::I16X8LeU:
    case Opcode::I16X8GeS:
    case Opcode::I16X8GeU:
    case Opcode::I32X4Eq:
    case Opcode::I32X4Ne:
    case Opcode::I32X4LtS:
    case Opcode::I32X4LtU:
    case Opcode::I32X4GtS:
    case Opcode::I32X4GtU:
    case Opcode::I32X4LeS:
    case Opcode::I32X4LeU:
    case Opcode::I32X4GeS:
    case Opcode::I32X4GeU:
    case Opcode::F32X4Eq:
    case Opcode::F32X4Ne:
    case Opcode::F32X4Lt:
    case Opcode::F32X4Gt:
    case Opcode::F32X4Le:
    case Opcode::F32X4Ge:
    case Opcode::F64X2Eq:
    case Opcode::F64X2Ne:
    case Opcode::F64X2Lt:
    case Opcode::F64X2Gt:
    case Opcode::F64X2Le:
    case Opcode::F64X2Ge:
    case Opcode::V128Not:
    case Opcode::V128And:
    case Opcode::V128Andnot:
    case Opcode::V128Or:
    case Opcode::V128Xor:
    case Opcode::V128BitSelect:
    case Opcode::V128AnyTrue:
    case Opcode::F32X4DemoteF64X2Zero:
    case Opcode::F64X2PromoteLowF32X4:
    case Opcode::I8X16Abs:
    case Opcode::I8X16Neg:
    case Opcode::I8X16Popcnt:
    case Opcode::I8X16AllTrue:
    case Opcode::I8X16Bitmask:
    case Opcode::I8X16NarrowI16X8S:
    case Opcode::I8X16NarrowI16X8U:
    case Opcode::F32X4Ceil:
    case Opcode::F32X4Floor:
    case Opcode::F32X4Trunc:
    case Opcode::F32X4Nearest:
    case Opcode::I8X16Shl:
    case Opcode::I8X16ShrS:
    case Opcode::I8X16ShrU:
    case Opcode::I8X16Add:
    case Opcode::I8X16AddSatS:
    case Opcode::I8X16AddSatU:
    case Opcode::I8X16Sub:
    case Opcode::I8X16SubSatS:
    case Opcode::I8X16SubSatU:
    case Opcode::F64X2Ceil:
    case Opcode::F64X2Floor:
    case Opcode::I8X16MinS:
    case Opcode::I8X16MinU:
    case Opcode::I8X16MaxS:
    case Opcode::I8X16MaxU:
    case Opcode::F64X2Trunc:
    case Opcode::I8X16AvgrU:
    case Opcode::I16X8ExtaddPairwiseI8X16S:
    case Opcode::I16X8ExtaddPairwiseI8X16U:
    case Opcode::I32X4ExtaddPairwiseI16X8S:
    case Opcode::I32X4ExtaddPairwiseI16X8U:
    case Opcode::I16X8Abs:
    case Opcode::I16X8Neg:
    case Opcode::I16X8Q15mulrSatS:
    case Opcode::I16X8AllTrue:
    case Opcode::I16X8Bitmask:
    case Opcode::I16X8NarrowI32X4S:
    case Opcode::I16X8NarrowI32X4U:
    case Opcode::I16X8ExtendLowI8X16S:
    case Opcode::I16X8ExtendHighI8X16S:
    case Opcode::I16X8ExtendLowI8X16U:
    case Opcode::I16X8ExtendHighI8X16U:
    case Opcode::I16X8Shl:
    case Opcode::I16X8ShrS:
    case Opcode::I16X8ShrU:
    case Opcode::I16X8Add:
    case Opcode::I16X8AddSatS:
    case Opcode::I16X8AddSatU:
    case Opcode::I16X8Sub:
    case Opcode::I16X8SubSatS:
    case Opcode::I16X8SubSatU:
    case Opcode::F64X2Nearest:
    case Opcode::I16X8Mul:
    case Opcode::I16X8MinS:
    case Opcode::I16X8MinU:
    case Opcode::I16X8MaxS:
    case Opcode::I16X8MaxU:
    case Opcode::I16X8AvgrU:
    case Opcode::I16X8ExtmulLowI8X16S:
    case Opcode::I16X8ExtmulHighI8X16S:
    case Opcode::I16X8ExtmulLowI8X16U:
    case Opcode::I16X8ExtmulHighI8X16U:
    case Opcode::I32X4Abs:
    case Opcode::I32X4Neg:
    case Opcode::I32X4AllTrue:
    case Opcode::I32X4Bitmask:
    case Opcode::I32X4ExtendLowI16X8S:
    case Opcode::I32X4ExtendHighI16X8S:
    case Opcode::I32X4ExtendLowI16X8U:
    case Opcode::I32X4ExtendHighI16X8U:
    case Opcode::I32X4Shl:
    case Opcode::I32X4ShrS:
    case Opcode::I32X4ShrU:
    case Opcode::I32X4Add:
    case Opcode::I32X4Sub:
    case Opcode::I32X4Mul:
    case Opcode::I32X4MinS:
    case Opcode::I32X4MinU:
    case Opcode::I32X4MaxS:
    case Opcode::I32X4MaxU:
    case Opcode::I32X4DotI16X8S:
    case Opcode::I32X4ExtmulLowI16X8S:
    case Opcode::I32X4ExtmulHighI16X8S:
    case Opcode::I32X4ExtmulLowI16X8U:
    case Opcode::I32X4ExtmulHighI16X8U:
    case Opcode::I64X2Abs:
    case Opcode::I64X2Neg:
    case Opcode::I64X2AllTrue:
    case Opcode::I64X2Bitmask:
    case Opcode::I64X2ExtendLowI32X4S:
    case Opcode::I64X2ExtendHighI32X4S:
    case Opcode::I64X2ExtendLowI32X4U:
    case Opcode::I64X2ExtendHighI32X4U:
    case Opcode::I64X2Shl:
    case Opcode::I64X2ShrS:
    case Opcode::I64X2ShrU:
    case Opcode::I64X2Add:
    case Opcode::I64X2Sub:
    case Opcode::I64X2Mul:
    case Opcode::I64X2Eq:
    case Opcode::I64X2Ne:
    case Opcode::I64X2LtS:
    case Opcode::I64X2GtS:
    case Opcode::I64X2LeS:
    case Opcode::I64X2GeS:
    case Opcode::I64X2ExtmulLowI32X4S:
    case Opcode::I64X2ExtmulHighI32X4S:
    case Opcode::I64X2ExtmulLowI32X4U:
    case Opcode::I64X2ExtmulHighI32X4U:
    case Opcode::F32X4Abs:
    case Opcode::F32X4Neg:
    case Opcode::F32X4Sqrt:
    case Opcode::F32X4Add:
    case Opcode::F32X4Sub:
    case Opcode::F32X4Mul:
    case Opcode::F32X4Div:
    case Opcode::F32X4Min:
    case Opcode::F32X4Max:
    case Opcode::F32X4Pmin:
    case Opcode::F32X4Pmax:
    case Opcode::F64X2Abs:
    case Opcode::F64X2Neg:
    case Opcode::F64X2Sqrt:
    case Opcode::F64X2Add:
    case Opcode::F64X2Sub:
    case Opcode::F64X2Mul:
    case Opcode::F64X2Div:
    case Opcode::F64X2Min:
    case Opcode::F64X2Max:
    case Opcode::F64X2Pmin:
    case Opcode::F64X2Pmax:
    case Opcode::I32X4TruncSatF32X4S:
    case Opcode::I32X4TruncSatF32X4U:
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::I32X4TruncSatF64X2SZero:
    case Opcode::I32X4TruncSatF64X2UZero:
    case Opcode::F64X2ConvertLowI32X4S:
    case Opcode::F64X2ConvertLowI32X4U:
    case Opcode::RefAsNonNull:
    case Opcode::CallRef:
    case Opcode::ReturnCallRef:
    case Opcode::RefEq:
    case Opcode::I31New:
    case Opcode::I31GetS:
    case Opcode::I31GetU:
      return At{guard.range(data), Instruction{opcode}};

    // No immediates, but only allowed if there's a matching block/loop/if/try
    // instruction.
    case Opcode::End:
      if (ctx.open_blocks.empty()) {
        ctx.seen_final_end = true;
      } else if (ctx.open_blocks.back() == Opcode::Try) {
        ctx.errors.OnError(
            opcode.loc(),
            "Expected catch or delegate instruction in try block");
        return nullopt;
      } else {
        ctx.open_blocks.pop_back();
      }
      return At{guard.range(data), Instruction{opcode}};

    // No immediates, but only allowed if there's a matching if instruction.
    case Opcode::Else:
      if (ctx.open_blocks.empty() || ctx.open_blocks.back() != Opcode::If) {
        ctx.errors.OnError(opcode.loc(), "Unexpected else instruction");
        return nullopt;
      } else {
        ctx.open_blocks.back() = opcode;
      }
      return At{guard.range(data), Instruction{opcode}};

    // Index immediate. Only allowed if there's a previous try/catch
    // instruction.
    case Opcode::Catch: {
      if (ctx.open_blocks.empty() ||
          (ctx.open_blocks.back().second != Opcode::Try &&
           ctx.open_blocks.back().second != Opcode::Catch)) {
        ctx.errors.OnError(opcode.loc(), concat("Unexpected catch instruction"));
        return nullopt;
      } else {
        ctx.open_blocks.back() = opcode;
      }
      WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
      return At{guard.range(data), Instruction{opcode, index}};
    }

    // Index immediate. Only allowed if there's a previous try instruction.
    case Opcode::Delegate: {
      if (ctx.open_blocks.empty() ||
          ctx.open_blocks.back().second != Opcode::Try) {
        ctx.errors.OnError(opcode.loc(),
                           concat("Unexpected delegate instruction"));
        return nullopt;
      } else {
        ctx.open_blocks.pop_back();
      }
      WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
      return At{guard.range(data), Instruction{opcode, index}};
    }

    // No immediates, but only allowed if there's a previous try/catch
    // instruction.
    case Opcode::CatchAll: {
      if (ctx.open_blocks.empty() ||
          (ctx.open_blocks.back().second != Opcode::Try &&
           ctx.open_blocks.back().second != Opcode::Catch)) {
        ctx.errors.OnError(opcode.loc(), "Unexpected catch_all instruction");
        return nullopt;
      } else {
        ctx.open_blocks.back() = opcode;
      }
      return At{guard.range(data), Instruction{opcode}};
    }

    // HeapType type immediate.
    case Opcode::RefNull:
    case Opcode::RttCanon: {
      WASP_TRY_READ(type, Read<HeapType>(data, ctx));
      return At{guard.range(data), Instruction{opcode, type}};
    }

    // Block type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If:
    case Opcode::Try: {
      WASP_TRY_READ(type, Read<BlockType>(data, ctx));
      ctx.open_blocks.push_back(opcode);
      return At{guard.range(data), Instruction{opcode, type}};
    }

    // Index immediate, w/ additional data count requirement.
    case Opcode::DataDrop:
      if (!RequireDataCountSection(ctx, opcode)) {
        return nullopt;
      }
      // Fallthrough.

    // Index immediate.
    case Opcode::Throw:
    case Opcode::Rethrow:
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
    case Opcode::ElemDrop:
    case Opcode::TableGrow:
    case Opcode::TableSize:
    case Opcode::TableFill:
    case Opcode::BrOnNull:
    case Opcode::BrOnNonNull:
    case Opcode::StructNewWithRtt:
    case Opcode::StructNewDefaultWithRtt:
    case Opcode::ArrayNewWithRtt:
    case Opcode::ArrayNewDefaultWithRtt:
    case Opcode::ArrayGet:
    case Opcode::ArrayGetS:
    case Opcode::ArrayGetU:
    case Opcode::ArraySet:
    case Opcode::ArrayLen: {
      WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
      return At{guard.range(data), Instruction{opcode, index}};
    }

    // FuncBind immediate.
    case Opcode::FuncBind: {
      WASP_TRY_READ(immediate, Read<FuncBindImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, std::move(immediate)}};
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect: {
      WASP_TRY_READ(immediate, Read<CallIndirectImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
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
    case Opcode::V128Load8X8S:
    case Opcode::V128Load8X8U:
    case Opcode::V128Load16X4S:
    case Opcode::V128Load16X4U:
    case Opcode::V128Load32X2S:
    case Opcode::V128Load32X2U:
    case Opcode::V128Load8Splat:
    case Opcode::V128Load16Splat:
    case Opcode::V128Load32Splat:
    case Opcode::V128Load64Splat:
    case Opcode::V128Load32Zero:
    case Opcode::V128Load64Zero:
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
      WASP_TRY_READ(memarg, Read<MemArgImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, memarg}};
    }

    case Opcode::V128Load8Lane:
    case Opcode::V128Load16Lane:
    case Opcode::V128Load32Lane:
    case Opcode::V128Load64Lane:
    case Opcode::V128Store8Lane:
    case Opcode::V128Store16Lane:
    case Opcode::V128Store32Lane:
    case Opcode::V128Store64Lane: {
      WASP_TRY_READ(immediate, Read<SimdMemoryLaneImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // MemOpt immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill: {
      WASP_TRY_READ(immediate, Read<MemOptImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, ctx), "i32 constant");
      return At{guard.range(data), Instruction{opcode, value}};
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, ctx), "i64 constant");
      return At{guard.range(data), Instruction{opcode, value}};
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, ctx), "f32 constant");
      return At{guard.range(data), Instruction{opcode, value}};
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, ctx), "f64 constant");
      return At{guard.range(data), Instruction{opcode, value}};
    }

    case Opcode::V128Const: {
      WASP_TRY_READ_CONTEXT(value, Read<v128>(data, ctx), "v128 constant");
      return At{guard.range(data), Instruction{opcode, value}};
    }

    // Init immediates.
    case Opcode::MemoryInit: {
      WASP_TRY_READ(immediate,
                    Read<InitImmediate>(data, ctx, BulkImmediateKind::Memory));
      if (!RequireDataCountSection(ctx, opcode)) {
        return nullopt;
      }
      return At{guard.range(data), Instruction{opcode, immediate}};
    }
    case Opcode::TableInit: {
      WASP_TRY_READ(immediate,
                    Read<InitImmediate>(data, ctx, BulkImmediateKind::Table));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // Copy immediates.
    case Opcode::MemoryCopy: {
      WASP_TRY_READ(immediate,
                    Read<CopyImmediate>(data, ctx, BulkImmediateKind::Memory));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }
    case Opcode::TableCopy: {
      WASP_TRY_READ(immediate,
                    Read<CopyImmediate>(data, ctx, BulkImmediateKind::Table));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // Shuffle immediate.
    case Opcode::I8X16Shuffle: {
      WASP_TRY_READ(immediate, Read<ShuffleImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // Select immediate.
    case Opcode::SelectT: {
      LocationGuard immediate_guard{data};
      WASP_TRY_READ(immediate, ReadVector<ValueType>(data, ctx, "types"));
      return At{
          guard.range(data),
          Instruction{opcode, At{immediate_guard.range(data), immediate}}};
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
      WASP_TRY_READ(lane, Read<u8>(data, ctx));
      return At{guard.range(data), Instruction{opcode, lane}};
    }

    // Let immediate.
    case Opcode::Let: {
      WASP_TRY_READ(immediate, Read<LetImmediate>(data, ctx));
      ctx.open_blocks.push_back(opcode);
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // StructField immediate.
    case Opcode::StructGet:
    case Opcode::StructGetS:
    case Opcode::StructGetU:
    case Opcode::StructSet: {
      WASP_TRY_READ(immediate, Read<StructFieldImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // RttSub immediate.
    case Opcode::RttSub: {
      // TODO: Determine whether this instruction should have heap type
      // immediates.
#if 0
      WASP_TRY_READ(immediate, Read<RttSubImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
#else
      WASP_TRY_READ(type, Read<HeapType>(data, ctx));
      return At{guard.range(data), Instruction{opcode, type}};
#endif
    }

    // Two HeapType immediate.
    case Opcode::RefTest:
    case Opcode::RefCast: {
      WASP_TRY_READ(immediate, Read<HeapType2Immediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
    }

    // BrOnCast immediate.
    case Opcode::BrOnCast: {
      // TODO: Determine whether this instruction should have heap type
      // immediates.
#if 0
      WASP_TRY_READ(immediate, Read<BrOnCastImmediate>(data, ctx));
      return At{guard.range(data), Instruction{opcode, immediate}};
#else
      WASP_TRY_READ(index, ReadIndex(data, ctx, "index"));
      return At{guard.range(data), Instruction{opcode, index}};
#endif
    }
  }
  WASP_UNREACHABLE();
}

OptAt<InstructionList> Read(SpanU8* data,
                            ReadCtx& ctx,
                            ReadTag<InstructionList>) {
  LocationGuard guard{data};
  InstructionList instrs;
  ctx.seen_final_end = false;
  while (true) {
    WASP_TRY_READ(instr, Read<Instruction>(data, ctx));
    if (ctx.seen_final_end) {
      break;
    }
    instrs.push_back(instr);
  }
  return At{guard.range(data), instrs};
}

OptAt<Index> ReadLength(SpanU8* data, ReadCtx& ctx) {
  return ReadCheckLength(data, ctx, "length", "Length");
}

OptAt<Limits> Read(SpanU8* data,
                   ReadCtx& ctx,
                   ReadTag<Limits>,
                   LimitsKind kind) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "limits"};
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, ctx), "flags");
  WASP_TRY_DECODE_FEATURES(decoded, flags, LimitsFlags, "flags value",
                           ctx.features);

  if (kind == LimitsKind::Table) {
    if (decoded->shared == Shared::Yes) {
      ctx.errors.OnError(flags.loc(), "shared tables are not allowed");
      return nullopt;
    }

    if (decoded->index_type == IndexType::I64) {
      ctx.errors.OnError(flags.loc(), "i64 index type is not allowed");
      return nullopt;
    }
  }

  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, ctx), "min");

  OptAt<u32> max;
  if (decoded->has_max == encoding::HasMax::Yes) {
    WASP_TRY_READ_CONTEXT(max_, Read<u32>(data, ctx), "max");
    max = max_;
  }
  return At{guard.range(data),
            Limits{min, max, At{flags.loc(), decoded->shared},
                   At{flags.loc(), decoded->index_type}}};
}

OptAt<Locals> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Locals>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "locals"};
  LocationGuard guard{data};
  WASP_TRY_READ(count, ReadIndex(data, ctx, "count"));

  ctx.local_count += count;
  if (ctx.local_count > std::numeric_limits<u32>::max()) {
    ctx.errors.OnError(count.loc(),
                       concat("Too many locals: ", ctx.local_count));
    return nullopt;
  }

  WASP_TRY_READ_CONTEXT(type, Read<ValueType>(data, ctx), "type");
  return At{guard.range(data), Locals{count, type}};
}

OptAt<LetImmediate> Read(SpanU8* data, ReadCtx& ctx, ReadTag<LetImmediate>) {
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(block_type, Read<BlockType>(data, ctx), "block_type");
  WASP_TRY_READ(locals, ReadVector<Locals>(data, ctx, "locals vector"));
  return At{guard.range(data), LetImmediate{block_type, locals}};
}

OptAt<MemArgImmediate> Read(SpanU8* data,
                            ReadCtx& ctx,
                            ReadTag<MemArgImmediate>) {
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(value, Read<u32>(data, ctx), "align log2");
  WASP_TRY_DECODE_FEATURES(decoded, value, MemArgAlignment, "flags",
                           ctx.features);
  auto align_log2 = At{decoded.loc(), decoded->align_log2};

  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, ctx), "offset");

  if (decoded->has_memory_index == encoding::HasMemoryIndex::Yes) {
    WASP_TRY_READ(memory_index, ReadIndex(data, ctx, "memory index"));
    return At{guard.range(data),
              MemArgImmediate{align_log2, offset, memory_index}};
  } else {
    return At{guard.range(data), MemArgImmediate{align_log2, offset}};
  }
}

OptAt<MemOptImmediate> Read(SpanU8* data,
                            ReadCtx& ctx,
                            ReadTag<MemOptImmediate>) {
  LocationGuard guard{data};
  if (ctx.features.multi_memory_enabled()) {
    WASP_TRY_READ(index, ReadIndex(data, ctx, "memory index"));
    return At{guard.range(data), MemOptImmediate{index}};
  } else {
    WASP_TRY_READ(reserved, ReadReservedIndex(data, ctx));
    return At{guard.range(data), MemOptImmediate{reserved}};
  }
}

OptAt<Memory> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Memory>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "memory"};
  WASP_TRY_READ(memory_type, Read<MemoryType>(data, ctx));
  return At{memory_type.loc(), Memory{memory_type}};
}

OptAt<MemoryType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<MemoryType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, ctx, LimitsKind::Memory));
  return At{limits.loc(), MemoryType{limits}};
}

OptAt<Mutability> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Mutability>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "mutability"};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  WASP_TRY_DECODE(decoded, val, Mutability, "mutability");
  return decoded;
}

OptAt<Opcode> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Opcode>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "opcode"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, Read<u8>(data, ctx));

  if (encoding::Opcode::IsPrefixByte(*val, ctx.features)) {
    WASP_TRY_READ(code, Read<u32>(data, ctx));
    auto decoded = encoding::Opcode::Decode(val, code, ctx.features);
    if (!decoded) {
      ctx.errors.OnError(guard.range(data),
                         concat("Unknown opcode: ", val, " ", code));
      return nullopt;
    }
    return At{guard.range(data), *decoded};
  } else {
    WASP_TRY_DECODE_FEATURES(decoded, val, Opcode, "opcode", ctx.features);
    return decoded;
  }
}

OptAt<u8> ReadReserved(SpanU8* data, ReadCtx& ctx) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "reserved"};
  LocationGuard guard{data};
  WASP_TRY_READ(reserved, Read<u8>(data, ctx));
  if (reserved != 0) {
    ctx.errors.OnError(reserved.loc(),
                       concat("Expected reserved byte 0, got ", reserved));
    return nullopt;
  }
  return reserved;
}

OptAt<Index> ReadReservedIndex(SpanU8* data, ReadCtx& ctx) {
  auto result_u8 = ReadReserved(data, ctx);
  if (!result_u8) {
    return nullopt;
  }
  return At{result_u8->loc(), Index(**result_u8)};
}

OptAt<s32> Read(SpanU8* data, ReadCtx& ctx, ReadTag<s32>) {
  return ReadVarInt<s32>(data, ctx, "s32");
}

OptAt<s64> Read(SpanU8* data, ReadCtx& ctx, ReadTag<s64>) {
  return ReadVarInt<s64>(data, ctx, "s64");
}

OptAt<Section> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Section>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "section"};
  LocationGuard guard{data};
  WASP_TRY_READ(id, Read<SectionId>(data, ctx));
  WASP_TRY_READ(length, ReadLength(data, ctx));
  WASP_TRY_READ(bytes, ReadBytes(data, length, ctx));

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name,
                  ReadUtf8String(&*bytes, ctx, "custom section name"));
    return At{guard.range(data),
              Section{At{guard.range(data), CustomSection{name, *bytes}}}};
  } else {
    if (ctx.last_section_id && *ctx.last_section_id >= id.value()) {
      ctx.errors.OnError(
          id.loc(), concat("Section out of order: ", id, " cannot occur after ",
                           *ctx.last_section_id));
    }
    ctx.last_section_id = id;

    return At{guard.range(data),
              Section{At{guard.range(data), KnownSection{id, *bytes}}}};
  }
}

OptAt<SectionId> Read(SpanU8* data, ReadCtx& ctx, ReadTag<SectionId>) {
  ErrorsContextGuard guard{ctx.errors, *data, "section id"};
  WASP_TRY_READ(val, Read<u8>(data, ctx));
  WASP_TRY_DECODE_FEATURES(decoded, val, SectionId, "section id", ctx.features);
  return decoded;
}

OptAt<ShuffleImmediate> Read(SpanU8* data,
                             ReadCtx& ctx,
                             ReadTag<ShuffleImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "shuffle immediate"};
  LocationGuard guard{data};
  ShuffleImmediate immediate;
  for (int i = 0; i < 16; ++i) {
    WASP_TRY_READ(byte, Read<u8>(data, ctx));
    immediate[i] = byte;
  }
  return At{guard.range(data), immediate};
}

OptAt<SimdMemoryLaneImmediate> Read(SpanU8* data,
                                    ReadCtx& ctx,
                                    ReadTag<SimdMemoryLaneImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "memory lane immediate"};
  LocationGuard guard{data};
  WASP_TRY_READ_CONTEXT(memarg, Read<MemArgImmediate>(data, ctx),
                        "memory immediate");
  WASP_TRY_READ_CONTEXT(lane, Read<u8>(data, ctx), "lane");
  return At{guard.range(data), SimdMemoryLaneImmediate{memarg, lane}};
}

OptAt<Start> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Start>) {
  ErrorsContextGuard guard{ctx.errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, ctx, "function index"));
  return At{index.loc(), Start{index}};
}

auto Read(SpanU8* data, ReadCtx& ctx, ReadTag<StorageType>)
    -> OptAt<StorageType> {
  ErrorsContextGuard error_guard{ctx.errors, *data, "storage type"};
  LocationGuard guard{data};

  WASP_TRY_READ(val, PeekU8(data, ctx));

  if (encoding::PackedType::Is(val)) {
    data->remove_prefix(1);
    WASP_TRY_DECODE_FEATURES(decoded, val, PackedType, "packed type",
                             ctx.features);
    return At{decoded.loc(), StorageType{decoded}};
  } else {
    WASP_TRY_READ(value_type, Read<ValueType>(data, ctx));
    return At{value_type.loc(), StorageType{value_type}};
  }
}

OptAt<StructType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<StructType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "struct type"};
  LocationGuard guard{data};
  WASP_TRY_READ(fields, ReadVector<FieldType>(data, ctx, "fields"));
  return At{guard.range(data), StructType{fields}};
}

OptAt<StructFieldImmediate> Read(SpanU8* data,
                                 ReadCtx& ctx,
                                 ReadTag<StructFieldImmediate>) {
  ErrorsContextGuard error_guard{ctx.errors, *data,
                                 "struct field immediate"};
  LocationGuard guard{data};
  WASP_TRY_READ(struct_, ReadIndex(data, ctx, "struct"));
  WASP_TRY_READ(field, ReadIndex(data, ctx, "field"));
  return At{guard.range(data), StructFieldImmediate{struct_, field}};
}

OptAt<string_view> ReadString(SpanU8* data, ReadCtx& ctx, string_view desc) {
  ErrorsContextGuard error_guard{ctx.errors, *data, desc};
  LocationGuard guard{data};
  WASP_TRY_READ(len, ReadLength(data, ctx));
  string_view result{reinterpret_cast<const char*>(data->data()), len};
  data->remove_prefix(len);
  return At{guard.range(data), result};
}

OptAt<string_view> ReadUtf8String(SpanU8* data,
                                  ReadCtx& ctx,
                                  string_view desc) {
  auto string = ReadString(data, ctx, desc);
  if (string && !IsValidUtf8(*string)) {
    ctx.errors.OnError(string->loc(), "Invalid UTF-8 encoding");
    return {};
  }
  return string;
}

OptAt<Table> Read(SpanU8* data, ReadCtx& ctx, ReadTag<Table>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "table"};
  WASP_TRY_READ(table_type, Read<TableType>(data, ctx));
  return At{table_type.loc(), Table{table_type}};
}

OptAt<TableType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<TableType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "table type"};
  LocationGuard guard{data};
  WASP_TRY_READ(elemtype, Read<ReferenceType>(data, ctx));
  WASP_TRY_READ(limits, Read<Limits>(data, ctx, LimitsKind::Table));
  return At{guard.range(data), TableType{std::move(limits), elemtype}};
}

OptAt<u32> Read(SpanU8* data, ReadCtx& ctx, ReadTag<u32>) {
  return ReadVarInt<u32>(data, ctx, "u32");
}

auto PeekU8(SpanU8* data, ReadCtx& ctx) -> OptAt<u8> {
  if (data->size() < 1) {
    ctx.errors.OnError(*data, "Unable to read u8");
    return nullopt;
  }

  Location loc = data->subspan(0, 1);
  u8 result{(*data)[0]};
  return At{loc, result};
}

OptAt<u8> Read(SpanU8* data, ReadCtx& ctx, ReadTag<u8>) {
  auto result_opt = PeekU8(data, ctx);
  if (result_opt) {
    data->remove_prefix(1);
  }
  return result_opt;
}

OptAt<v128> Read(SpanU8* data, ReadCtx& ctx, ReadTag<v128>) {
  static_assert(sizeof(v128) == 16, "sizeof(v128) != 16");
  ErrorsContextGuard guard{ctx.errors, *data, "v128"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(v128), ctx));
  v128 result;
  memcpy(&result, bytes->data(), sizeof(v128));
  return At{bytes.loc(), result};
}

OptAt<ValueType> Read(SpanU8* data, ReadCtx& ctx, ReadTag<ValueType>) {
  ErrorsContextGuard error_guard{ctx.errors, *data, "value type"};
  LocationGuard guard{data};
  WASP_TRY_READ(val, PeekU8(data, ctx));

  if (encoding::NumericType::Is(val)) {
    data->remove_prefix(1);
    WASP_TRY_DECODE_FEATURES(decoded, val, NumericType, "value type",
                             ctx.features);
    return At{decoded.loc(), ValueType{decoded}};
  } else if (encoding::Rtt::Is(val)) {
    WASP_TRY_READ(rtt, Read<Rtt>(data, ctx));
    return At{guard.range(data), ValueType{rtt}};
  } else {
    WASP_TRY_READ(reference_type, Read<ReferenceType>(data, ctx));
    // Funcref cannot be used as a value type until the reference types
    // proposal.
    if (reference_type->is_reference_kind() &&
        reference_type->reference_kind() == ReferenceKind::Funcref &&
        !ctx.features.reference_types_enabled()) {
      ctx.errors.OnError(reference_type.loc(),
                         concat(*reference_type, " not allowed"));
      return nullopt;
    }
    return At{reference_type.loc(), ValueType{reference_type}};
  }
}

bool EndCode(SpanU8 data, ReadCtx& ctx) {
  if (!ctx.open_blocks.empty()) {
    for (auto& [loc, op] : ctx.open_blocks) {
      ctx.errors.OnError(loc, concat("Unclosed ", op, " instruction"));
    }
    return false;
  }
  if (!ctx.seen_final_end) {
    ctx.errors.OnError(data, "Expected final end instruction");
    return false;
  }
  return true;
}

bool EndModule(SpanU8 data, ReadCtx& ctx) {
  if (ctx.defined_function_count != ctx.code_count) {
    ctx.errors.OnError(
        data, concat("Expected code count of ", ctx.defined_function_count,
                     ", but got ", ctx.code_count));
    return false;
  }
  if (ctx.declared_data_count && *ctx.declared_data_count != ctx.data_count) {
    ctx.errors.OnError(
        data, concat("Expected data count of ", *ctx.declared_data_count,
                     ", but got ", ctx.data_count));
    return false;
  }
  return true;
}

}  // namespace wasp::binary

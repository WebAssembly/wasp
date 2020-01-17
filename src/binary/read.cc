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
#include "wasp/binary/encoding.h"  // XXX
#include "wasp/binary/encoding/block_type_encoding.h"
#include "wasp/binary/encoding/comdat_symbol_kind_encoding.h"
#include "wasp/binary/encoding/element_type_encoding.h"
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
#include "wasp/binary/read_name.h"  // TODO move to read_name.cc

namespace wasp {
namespace binary {

optional<BlockType> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<BlockType>) {
  ErrorsContextGuard guard{errors, *data, "block type"};
  if (features.multi_value_enabled()) {
    WASP_TRY_READ(val, Read<s32>(data, features, errors));
    auto decoded = encoding::BlockType::Decode(val, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown block type: {}", val));
      return nullopt;
    }
    return decoded;
  } else {
    WASP_TRY_READ(val, Read<u8>(data, features, errors));
    auto decoded = encoding::BlockType::Decode(val, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown block type: {}", val));
      return nullopt;
    }
    return decoded;
  }
}

optional<BrOnExnImmediate> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<BrOnExnImmediate>) {
  ErrorsContextGuard guard{errors, *data, "br_on_exn"};
  WASP_TRY_READ(target, ReadIndex(data, features, errors, "target"));
  WASP_TRY_READ(exception_index,
                ReadIndex(data, features, errors, "exception index"));
  return BrOnExnImmediate{target, exception_index};
}

optional<BrTableImmediate> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<BrTableImmediate>) {
  ErrorsContextGuard guard{errors, *data, "br_table"};
  WASP_TRY_READ(targets, ReadVector<Index>(data, features, errors, "targets"));
  WASP_TRY_READ(default_target,
                ReadIndex(data, features, errors, "default target"));
  return BrTableImmediate{std::move(targets), default_target};
}

optional<SpanU8> ReadBytes(SpanU8* data,
                           SpanU8::index_type N,
                           const Features& features,
                           Errors& errors) {
  if (data->size() < N) {
    errors.OnError(*data, format("Unable to read {} bytes", N));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  remove_prefix(data, N);
  return result;
}

optional<SpanU8> ReadBytesExpected(SpanU8* data,
                                   SpanU8 expected,
                                   const Features& features,
                                   Errors& errors,
                                   string_view desc) {
  ErrorsContextGuard guard{errors, *data, desc};

  auto actual = ReadBytes(data, expected.size(), features, errors);
  if (actual && actual != expected) {
    errors.OnError(*data,
                   format("Mismatch: expected {}, got {}", expected, *actual));
  }
  return actual;
}

optional<CallIndirectImmediate> Read(SpanU8* data,
                                     const Features& features,
                                     Errors& errors,
                                     Tag<CallIndirectImmediate>) {
  ErrorsContextGuard guard{errors, *data, "call_indirect"};
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "type index"));
  WASP_TRY_READ(reserved, ReadReserved(data, features, errors));
  return CallIndirectImmediate{index, reserved};
}

optional<Index> ReadCheckLength(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                string_view context_name,
                                string_view error_name) {
  WASP_TRY_READ(count, ReadIndex(data, features, errors, context_name));

  // There should be at least one byte per count, so if the data is smaller
  // than that, the module must be malformed.
  if (count > data->size()) {
    errors.OnError(*data, format("{} extends past end: {} > {}", error_name,
                                 count, data->size()));
    return nullopt;
  }

  return count;
}

optional<Code> Read(SpanU8* data,
                    const Features& features,
                    Errors& errors,
                    Tag<Code>) {
  ErrorsContextGuard guard{errors, *data, "code"};
  WASP_TRY_READ(body_size, ReadLength(data, features, errors));
  WASP_TRY_READ(body, ReadBytes(data, body_size, features, errors));
  WASP_TRY_READ(locals,
                ReadVector<Locals>(&body, features, errors, "locals vector"));
  return Code{std::move(locals), Expression{std::move(body)}};
}

optional<Comdat> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Comdat>) {
  ErrorsContextGuard guard{errors, *data, "comdat"};
  WASP_TRY_READ(name, ReadString(data, features, errors, "name"));
  WASP_TRY_READ(flags, Read<u32>(data, features, errors));
  WASP_TRY_READ(symbols, ReadVector<ComdatSymbol>(data, features, errors,
                                                  "comdat symbols vector"));
  return Comdat{name, flags, std::move(symbols)};
}

optional<ComdatSymbol> Read(SpanU8* data,
                            const Features& features,
                            Errors& errors,
                            Tag<ComdatSymbol>) {
  ErrorsContextGuard guard{errors, *data, "comdat symbol"};
  WASP_TRY_READ(kind, Read<ComdatSymbolKind>(data, features, errors));
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
  return ComdatSymbol{kind, index};
}

optional<ComdatSymbolKind> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<ComdatSymbolKind>) {
  ErrorsContextGuard guard{errors, *data, "comdat symbol kind"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, ComdatSymbolKind, "comdat symbol kind");
  return decoded;
}

optional<ConstantExpression> Read(SpanU8* data,
                                  const Features& features,
                                  Errors& errors,
                                  Tag<ConstantExpression>) {
  ErrorsContextGuard guard{errors, *data, "constant expression"};
  WASP_TRY_READ(instr, Read<Instruction>(data, features, errors));
  switch (instr.opcode) {
    case Opcode::I32Const:
    case Opcode::I64Const:
    case Opcode::F32Const:
    case Opcode::F64Const:
    case Opcode::GlobalGet:
      // OK.
      break;

    default:
      errors.OnError(
          *data,
          format("Illegal instruction in constant expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, features, errors));
  if (end.opcode != Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ConstantExpression{instr};
}

optional<CopyImmediate> Read(SpanU8* data,
                             const Features& features,
                             Errors& errors,
                             Tag<CopyImmediate>) {
  ErrorsContextGuard guard{errors, *data, "copy immediate"};
  WASP_TRY_READ(src_reserved, ReadReserved(data, features, errors));
  WASP_TRY_READ(dst_reserved, ReadReserved(data, features, errors));
  return CopyImmediate{src_reserved, dst_reserved};
}

optional<Index> ReadCount(SpanU8* data,
                          const Features& features,
                          Errors& errors) {
  return ReadCheckLength(data, features, errors, "count", "Count");
}

optional<DataCount> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<DataCount>) {
  ErrorsContextGuard guard{errors, *data, "data count"};
  WASP_TRY_READ(count, ReadIndex(data, features, errors, "count"));
  return DataCount{count};
}

optional<DataSegment> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<DataSegment>) {
  ErrorsContextGuard guard{errors, *data, "data segment"};
  auto decoded = encoding::DecodedSegmentFlags::MVP();
  if (features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, features, errors, "flags"));
    WASP_TRY_DECODE(decoded_opt, flags, SegmentFlags, "flags");
    decoded = *decoded_opt;
  }

  Index memory_index = 0;
  if (decoded.has_index == encoding::HasIndex::Yes) {
    WASP_TRY_READ(memory_index_,
                  ReadIndex(data, features, errors, "memory index"));
    memory_index = memory_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(
        offset, Read<ConstantExpression>(data, features, errors), "offset");
    WASP_TRY_READ(len, ReadLength(data, features, errors));
    WASP_TRY_READ(init, ReadBytes(data, len, features, errors));
    return DataSegment{memory_index, offset, init};
  } else {
    WASP_TRY_READ(len, ReadLength(data, features, errors));
    WASP_TRY_READ(init, ReadBytes(data, len, features, errors));
    return DataSegment{init};
  }
}

optional<ElementExpression> Read(SpanU8* data,
                                 const Features& features,
                                 Errors& errors,
                                 Tag<ElementExpression>) {
  ErrorsContextGuard guard{errors, *data, "element expression"};
  // Element expressions were first added in the bulk memory proposal, so it
  // shouldn't be read (and this function shouldn't be called) if that feature
  // is not enabled.
  assert(features.bulk_memory_enabled());
  // The only valid instructions are enabled by the reference types and
  // function references proposals, but their encoding is still used by the
  // bulk memory proposal.
  Features new_features;
  new_features.enable_reference_types();
  new_features.enable_function_references();
  WASP_TRY_READ(instr, Read<Instruction>(data, new_features, errors));
  switch (instr.opcode) {
    case Opcode::RefNull:
    case Opcode::RefFunc:
      // OK.
      break;

    default:
      errors.OnError(
          *data,
          format("Illegal instruction in element expression: {}", instr));
      return nullopt;
  }

  WASP_TRY_READ(end, Read<Instruction>(data, features, errors));
  if (end.opcode != Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ElementExpression{instr};
}

optional<ElementSegment> Read(SpanU8* data,
                              const Features& features,
                              Errors& errors,
                              Tag<ElementSegment>) {
  ErrorsContextGuard guard{errors, *data, "element segment"};
  auto decoded = encoding::DecodedSegmentFlags::MVP();
  if (features.bulk_memory_enabled()) {
    WASP_TRY_READ(flags, ReadIndex(data, features, errors, "flags"));
    WASP_TRY_DECODE(decoded_opt, flags, SegmentFlags, "flags");
    decoded = *decoded_opt;
  }

  Index table_index = 0;
  if (decoded.has_index == encoding::HasIndex::Yes) {
    WASP_TRY_READ(table_index_,
                  ReadIndex(data, features, errors, "table index"));
    table_index = table_index_;
  }

  if (decoded.segment_type == SegmentType::Active) {
    WASP_TRY_READ_CONTEXT(
        offset, Read<ConstantExpression>(data, features, errors), "offset");
    WASP_TRY_READ(init,
                  ReadVector<Index>(data, features, errors, "initializers"));
    return ElementSegment{table_index, offset, init};
  } else {
    WASP_TRY_READ(element_type, Read<ElementType>(data, features, errors));
    WASP_TRY_READ(init, ReadVector<ElementExpression>(data, features, errors,
                                                      "initializers"));
    return ElementSegment{element_type, init};
  }
}

optional<ElementType> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<ElementType>) {
  ErrorsContextGuard guard{errors, *data, "element type"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, ElementType, "element type");
  return decoded;
}

optional<Export> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Export>) {
  ErrorsContextGuard guard{errors, *data, "export"};
  WASP_TRY_READ(name, ReadString(data, features, errors, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, features, errors));
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
  return Export{kind, name, index};
}

optional<ExternalKind> Read(SpanU8* data,
                            const Features& features,
                            Errors& errors,
                            Tag<ExternalKind>) {
  ErrorsContextGuard guard{errors, *data, "external kind"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, ExternalKind, "external kind");
  return decoded;
}

optional<f32> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard guard{errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), features, errors));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

optional<f64> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  ErrorsContextGuard guard{errors, *data, "f64"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), features, errors));
  f64 result;
  memcpy(&result, bytes.data(), sizeof(f64));
  return result;
}

optional<Function> Read(SpanU8* data,
                        const Features& features,
                        Errors& errors,
                        Tag<Function>) {
  ErrorsContextGuard guard{errors, *data, "function"};
  WASP_TRY_READ(type_index, ReadIndex(data, features, errors, "type index"));
  return Function{type_index};
}

optional<FunctionType> Read(SpanU8* data,
                            const Features& features,
                            Errors& errors,
                            Tag<FunctionType>) {
  ErrorsContextGuard guard{errors, *data, "function type"};
  WASP_TRY_READ(param_types,
                ReadVector<ValueType>(data, features, errors, "param types"));
  WASP_TRY_READ(result_types,
                ReadVector<ValueType>(data, features, errors, "result types"));
  return FunctionType{std::move(param_types), std::move(result_types)};
}

optional<Global> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Global>) {
  ErrorsContextGuard guard{errors, *data, "global"};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, features, errors));
  WASP_TRY_READ(init_expr, Read<ConstantExpression>(data, features, errors));
  return Global{global_type, std::move(init_expr)};
}

optional<GlobalType> Read(SpanU8* data,
                          const Features& features,
                          Errors& errors,
                          Tag<GlobalType>) {
  ErrorsContextGuard guard{errors, *data, "global type"};
  WASP_TRY_READ(type, Read<ValueType>(data, features, errors));
  WASP_TRY_READ(mut, Read<Mutability>(data, features, errors));
  return GlobalType{type, mut};
}

optional<Import> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Import>) {
  ErrorsContextGuard guard{errors, *data, "import"};
  WASP_TRY_READ(module, ReadString(data, features, errors, "module name"));
  WASP_TRY_READ(name, ReadString(data, features, errors, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, features, errors));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index,
                    ReadIndex(data, features, errors, "function index"));
      return Import{module, name, type_index};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, features, errors));
      return Import{module, name, table_type};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, features, errors));
      return Import{module, name, memory_type};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, features, errors));
      return Import{module, name, global_type};
    }
  }
  WASP_UNREACHABLE();
}

optional<Index> ReadIndex(SpanU8* data,
                          const Features& features,
                          Errors& errors,
                          string_view desc) {
  return ReadVarInt<Index>(data, features, errors, desc);
}

optional<IndirectNameAssoc> Read(SpanU8* data,
                                 const Features& features,
                                 Errors& errors,
                                 Tag<IndirectNameAssoc>) {
  ErrorsContextGuard guard{errors, *data, "indirect name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
  WASP_TRY_READ(name_map,
                ReadVector<NameAssoc>(data, features, errors, "name map"));
  return IndirectNameAssoc{index, std::move(name_map)};
}

optional<InitFunction> Read(SpanU8* data,
                            const Features& features,
                            Errors& errors,
                            Tag<InitFunction>) {
  ErrorsContextGuard guard{errors, *data, "init function"};
  WASP_TRY_READ(priority, Read<u32>(data, features, errors));
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "function index"));
  return InitFunction{priority, index};
}

optional<InitImmediate> Read(SpanU8* data,
                             const Features& features,
                             Errors& errors,
                             Tag<InitImmediate>) {
  ErrorsContextGuard guard{errors, *data, "init immediate"};
  WASP_TRY_READ(segment_index,
                ReadIndex(data, features, errors, "segment index"));
  WASP_TRY_READ(reserved, ReadReserved(data, features, errors));
  return InitImmediate{segment_index, reserved};
}

optional<Instruction> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<Instruction>) {
  WASP_TRY_READ(opcode, Read<Opcode>(data, features, errors));
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
    case Opcode::I8X16Mul:
    case Opcode::I16X8Mul:
    case Opcode::I32X4Mul:
    case Opcode::I8X16AddSaturateS:
    case Opcode::I8X16AddSaturateU:
    case Opcode::I16X8AddSaturateS:
    case Opcode::I16X8AddSaturateU:
    case Opcode::I8X16SubSaturateS:
    case Opcode::I8X16SubSaturateU:
    case Opcode::I16X8SubSaturateS:
    case Opcode::I16X8SubSaturateU:
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
    case Opcode::I64X2AnyTrue:
    case Opcode::I8X16AllTrue:
    case Opcode::I16X8AllTrue:
    case Opcode::I32X4AllTrue:
    case Opcode::I64X2AllTrue:
    case Opcode::F32X4Neg:
    case Opcode::F64X2Neg:
    case Opcode::F32X4Abs:
    case Opcode::F64X2Abs:
    case Opcode::F32X4Sqrt:
    case Opcode::F64X2Sqrt:
    case Opcode::V128BitSelect:
    case Opcode::F32X4ConvertI32X4S:
    case Opcode::F32X4ConvertI32X4U:
    case Opcode::F64X2ConvertI64X2S:
    case Opcode::F64X2ConvertI64X2U:
    case Opcode::I32X4TruncSatF32X4S:
    case Opcode::I32X4TruncSatF32X4U:
    case Opcode::I64X2TruncSatF64X2S:
    case Opcode::I64X2TruncSatF64X2U:
    case Opcode::V8X16Swizzle:
      return Instruction{Opcode{opcode}};

    // Type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If:
    case Opcode::Try: {
      WASP_TRY_READ(type, Read<BlockType>(data, features, errors));
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
    case Opcode::TableSize: {
      WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
      return Instruction{Opcode{opcode}, index};
    }

    // Index, Index immediates.
    case Opcode::BrOnExn: {
      WASP_TRY_READ(immediate, Read<BrOnExnImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, std::move(immediate)};
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect: {
      WASP_TRY_READ(immediate,
                    Read<CallIndirectImmediate>(data, features, errors));
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
    case Opcode::I8X16LoadSplat:
    case Opcode::I16X8LoadSplat:
    case Opcode::I32X4LoadSplat:
    case Opcode::I64X2LoadSplat:
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
      WASP_TRY_READ(memarg, Read<MemArgImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, memarg};
    }

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill: {
      WASP_TRY_READ(reserved, ReadReserved(data, features, errors));
      return Instruction{Opcode{opcode}, reserved};
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, features, errors),
                            "i32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, features, errors),
                            "i64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, features, errors),
                            "f32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, features, errors),
                            "f64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::V128Const: {
      WASP_TRY_READ_CONTEXT(value, Read<v128>(data, features, errors),
                            "v128 constant");
      return Instruction{Opcode{opcode}, value};
    }

    // Reserved, Index immediates.
    case Opcode::MemoryInit:
    case Opcode::TableInit: {
      WASP_TRY_READ(immediate, Read<InitImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Reserved, reserved immediates.
    case Opcode::MemoryCopy:
    case Opcode::TableCopy: {
      WASP_TRY_READ(immediate, Read<CopyImmediate>(data, features, errors));
      return Instruction{Opcode{opcode}, immediate};
    }

    // Shuffle immediate.
    case Opcode::V8X16Shuffle: {
      WASP_TRY_READ(immediate, Read<ShuffleImmediate>(data, features, errors));
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
      WASP_TRY_READ(lane, Read<u8>(data, features, errors));
      return Instruction{Opcode{opcode}, u8{lane}};
    }
  }
  WASP_UNREACHABLE();
}

optional<Index> ReadLength(SpanU8* data,
                           const Features& features,
                           Errors& errors) {
  return ReadCheckLength(data, features, errors, "length", "Length");
}

optional<Limits> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Limits>) {
  ErrorsContextGuard guard{errors, *data, "limits"};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, features, errors), "flags");
  auto decoded = encoding::LimitsFlags::Decode(flags, features);
  if (!decoded) {
    errors.OnError(*data, format("Invalid flags value: {}", flags));
    return nullopt;
  }

  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, features, errors), "min");

  if (decoded->has_max == encoding::HasMax::No) {
    return Limits{min};
  } else {
    WASP_TRY_READ_CONTEXT(max, Read<u32>(data, features, errors), "max");
    return Limits{min, max, decoded->shared};
  }
}

optional<LinkingSubsection> Read(SpanU8* data,
                                 const Features& features,
                                 Errors& errors,
                                 Tag<LinkingSubsection>) {
  ErrorsContextGuard guard{errors, *data, "linking subsection"};
  WASP_TRY_READ(id, Read<LinkingSubsectionId>(data, features, errors));
  WASP_TRY_READ(length, ReadLength(data, features, errors));
  auto bytes = *ReadBytes(data, length, features, errors);
  return LinkingSubsection{id, bytes};
}

optional<LinkingSubsectionId> Read(SpanU8* data,
                                   const Features& features,
                                   Errors& errors,
                                   Tag<LinkingSubsectionId>) {
  ErrorsContextGuard guard{errors, *data, "linking subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, LinkingSubsectionId, "linking subsection id");
  return decoded;
}

optional<Locals> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Locals>) {
  ErrorsContextGuard guard{errors, *data, "locals"};
  WASP_TRY_READ(count, ReadIndex(data, features, errors, "count"));
  WASP_TRY_READ_CONTEXT(type, Read<ValueType>(data, features, errors), "type");
  return Locals{count, type};
}

optional<MemArgImmediate> Read(SpanU8* data,
                               const Features& features,
                               Errors& errors,
                               Tag<MemArgImmediate>) {
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, features, errors),
                        "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, features, errors), "offset");
  return MemArgImmediate{align_log2, offset};
}

optional<Memory> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Memory>) {
  ErrorsContextGuard guard{errors, *data, "memory"};
  WASP_TRY_READ(memory_type, Read<MemoryType>(data, features, errors));
  return Memory{memory_type};
}

optional<MemoryType> Read(SpanU8* data,
                          const Features& features,
                          Errors& errors,
                          Tag<MemoryType>) {
  ErrorsContextGuard guard{errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, features, errors));
  return MemoryType{limits};
}

optional<Mutability> Read(SpanU8* data,
                          const Features& features,
                          Errors& errors,
                          Tag<Mutability>) {
  ErrorsContextGuard guard{errors, *data, "mutability"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, Mutability, "mutability");
  return decoded;
}

optional<NameAssoc> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<NameAssoc>) {
  ErrorsContextGuard guard{errors, *data, "name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
  WASP_TRY_READ(name, ReadString(data, features, errors, "name"));
  return NameAssoc{index, name};
}

optional<NameSubsection> Read(SpanU8* data,
                              const Features& features,
                              Errors& errors,
                              Tag<NameSubsection>) {
  ErrorsContextGuard guard{errors, *data, "name subsection"};
  WASP_TRY_READ(id, Read<NameSubsectionId>(data, features, errors));
  WASP_TRY_READ(length, ReadLength(data, features, errors));
  auto bytes = *ReadBytes(data, length, features, errors);
  return NameSubsection{id, bytes};
}

optional<NameSubsectionId> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<NameSubsectionId>) {
  ErrorsContextGuard guard{errors, *data, "name subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, NameSubsectionId, "name subsection id");
  return decoded;
}

optional<Opcode> Read(SpanU8* data,
                      const Features& features,
                      Errors& errors,
                      Tag<Opcode>) {
  ErrorsContextGuard guard{errors, *data, "opcode"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));

  if (encoding::Opcode::IsPrefixByte(val, features)) {
    WASP_TRY_READ(code, Read<u32>(data, features, errors));
    auto decoded = encoding::Opcode::Decode(val, code, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown opcode: {} {}", val, code));
      return nullopt;
    }
    return decoded;
  } else {
    auto decoded = encoding::Opcode::Decode(val, features);
    if (!decoded) {
      errors.OnError(*data, format("Unknown opcode: {}", val));
      return nullopt;
    }
    return decoded;
  }
}

optional<RelocationEntry> Read(SpanU8* data,
                               const Features& features,
                               Errors& errors,
                               Tag<RelocationEntry>) {
  ErrorsContextGuard guard{errors, *data, "relocation entry"};
  WASP_TRY_READ(type, Read<RelocationType>(data, features, errors));
  WASP_TRY_READ(offset, Read<u32>(data, features, errors));
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
  switch (type) {
    case RelocationType::MemoryAddressLEB:
    case RelocationType::MemoryAddressSLEB:
    case RelocationType::MemoryAddressI32:
    case RelocationType::FunctionOffsetI32:
    case RelocationType::SectionOffsetI32: {
      WASP_TRY_READ(addend, Read<s32>(data, features, errors));
      return RelocationEntry{type, offset, index, addend};
    }

    default:
      return RelocationEntry{type, offset, index, nullopt};
  }
}

optional<RelocationType> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<RelocationType>) {
  ErrorsContextGuard guard{errors, *data, "relocation type"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, RelocationType, "relocation type");
  return decoded;
}

optional<u8> ReadReserved(SpanU8* data,
                          const Features& features,
                          Errors& errors) {
  ErrorsContextGuard guard{errors, *data, "reserved"};
  WASP_TRY_READ(reserved, Read<u8>(data, features, errors));
  if (reserved != 0) {
    errors.OnError(*data, format("Expected reserved byte 0, got {}", reserved));
    return nullopt;
  }
  return 0;
}

optional<s32> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<s32>) {
  return ReadVarInt<s32>(data, features, errors, "s32");
}

optional<s64> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<s64>) {
  return ReadVarInt<s64>(data, features, errors, "s64");
}

optional<Section> Read(SpanU8* data,
                       const Features& features,
                       Errors& errors,
                       Tag<Section>) {
  ErrorsContextGuard guard{errors, *data, "section"};
  WASP_TRY_READ(id, Read<SectionId>(data, features, errors));
  WASP_TRY_READ(length, ReadLength(data, features, errors));
  auto bytes = *ReadBytes(data, length, features, errors);

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name,
                  ReadString(&bytes, features, errors, "custom section name"));
    return Section{CustomSection{name, bytes}};
  } else {
    return Section{KnownSection{id, bytes}};
  }
}

optional<SectionId> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<SectionId>) {
  ErrorsContextGuard guard{errors, *data, "section id"};
  WASP_TRY_READ(val, Read<u32>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, SectionId, "section id");
  return decoded;
}

optional<SegmentInfo> Read(SpanU8* data,
                           const Features& features,
                           Errors& errors,
                           Tag<SegmentInfo>) {
  ErrorsContextGuard guard{errors, *data, "segment info"};
  WASP_TRY_READ(name, ReadString(data, features, errors, "name"));
  WASP_TRY_READ(align_log2, Read<u32>(data, features, errors));
  WASP_TRY_READ(flags, Read<u32>(data, features, errors));
  return SegmentInfo{name, align_log2, flags};
}

optional<ShuffleImmediate> Read(SpanU8* data,
                                const Features& features,
                                Errors& errors,
                                Tag<ShuffleImmediate>) {
  ErrorsContextGuard guard{errors, *data, "shuffle immediate"};
  ShuffleImmediate immediate;
  for (int i = 0; i < 16; ++i) {
    WASP_TRY_READ(byte, Read<u8>(data, features, errors));
    immediate[i] = byte;
  }
  return immediate;
}

optional<Start> Read(SpanU8* data,
                     const Features& features,
                     Errors& errors,
                     Tag<Start>) {
  ErrorsContextGuard guard{errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, features, errors, "function index"));
  return Start{index};
}

optional<string_view> ReadString(SpanU8* data,
                                 const Features& features,
                                 Errors& errors,
                                 string_view desc) {
  ErrorsContextGuard guard{errors, *data, desc};
  WASP_TRY_READ(len, ReadLength(data, features, errors));
  string_view result{reinterpret_cast<const char*>(data->data()), len};
  remove_prefix(data, len);
  return result;
}

optional<SymbolInfo> Read(SpanU8* data,
                          const Features& features,
                          Errors& errors,
                          Tag<SymbolInfo>) {
  ErrorsContextGuard guard{errors, *data, "symbol info"};
  WASP_TRY_READ(kind, Read<SymbolInfoKind>(data, features, errors));
  WASP_TRY_READ(encoded_flags, Read<u32>(data, features, errors));
  WASP_TRY_DECODE(flags_opt, encoded_flags, SymbolInfoFlags,
                  "symbol info flags");
  auto flags = *flags_opt;
  switch (kind) {
    case SymbolInfoKind::Function:
    case SymbolInfoKind::Global:
    case SymbolInfoKind::Event: {
      WASP_TRY_READ(index, ReadIndex(data, features, errors, "index"));
      optional<string_view> name;
      if (flags.undefined == SymbolInfo::Flags::Undefined::No ||
          flags.explicit_name == SymbolInfo::Flags::ExplicitName::Yes) {
        WASP_TRY_READ(name_, ReadString(data, features, errors, "name"));
        name = name_;
      }
      return SymbolInfo{flags, SymbolInfo::Base{kind, index, name}};
    }

    case SymbolInfoKind::Data: {
      WASP_TRY_READ(name, ReadString(data, features, errors, "name"));
      optional<SymbolInfo::Data::Defined> defined;
      if (flags.undefined == SymbolInfo::Flags::Undefined::No) {
        WASP_TRY_READ(index,
                      ReadIndex(data, features, errors, "segment index"));
        WASP_TRY_READ(offset, Read<u32>(data, features, errors));
        WASP_TRY_READ(size, Read<u32>(data, features, errors));
        defined = SymbolInfo::Data::Defined{index, offset, size};
      }
      return SymbolInfo{flags, SymbolInfo::Data{name, defined}};
    }

    case SymbolInfoKind::Section:
      WASP_TRY_READ(section, Read<u32>(data, features, errors));
      return SymbolInfo{flags, SymbolInfo::Section{section}};
  }
  WASP_UNREACHABLE();
}

optional<SymbolInfoKind> Read(SpanU8* data,
                              const Features& features,
                              Errors& errors,
                              Tag<SymbolInfoKind>) {
  ErrorsContextGuard guard{errors, *data, "symbol info kind"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  WASP_TRY_DECODE(decoded, val, SymbolInfoKind, "symbol info kind");
  return decoded;
}

optional<Table> Read(SpanU8* data,
                     const Features& features,
                     Errors& errors,
                     Tag<Table>) {
  ErrorsContextGuard guard{errors, *data, "table"};
  WASP_TRY_READ(table_type, Read<TableType>(data, features, errors));
  return Table{table_type};
}

optional<TableType> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<TableType>) {
  ErrorsContextGuard guard{errors, *data, "table type"};
  WASP_TRY_READ(elemtype, Read<ElementType>(data, features, errors));
  WASP_TRY_READ(limits, Read<Limits>(data, features, errors));
  return TableType{limits, elemtype};
}

optional<TypeEntry> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<TypeEntry>) {
  ErrorsContextGuard guard{errors, *data, "type entry"};
  WASP_TRY_READ_CONTEXT(form, Read<u8>(data, features, errors), "form");

  if (form != encoding::Type::Function) {
    errors.OnError(*data, format("Unknown type form: {}", form));
    return nullopt;
  }

  WASP_TRY_READ(function_type, Read<FunctionType>(data, features, errors));
  return TypeEntry{std::move(function_type)};
}

optional<u32> Read(SpanU8* data,
                   const Features& features,
                   Errors& errors,
                   Tag<u32>) {
  return ReadVarInt<u32>(data, features, errors, "u32");
}

optional<u8> Read(SpanU8* data,
                  const Features& features,
                  Errors& errors,
                  Tag<u8>) {
  if (data->size() < 1) {
    errors.OnError(*data, "Unable to read u8");
    return nullopt;
  }

  u8 result{(*data)[0]};
  remove_prefix(data, 1);
  return result;
}

optional<v128> Read(SpanU8* data,
                    const Features& features,
                    Errors& errors,
                    Tag<v128>) {
  static_assert(sizeof(v128) == 16, "sizeof(v128) != 16");
  ErrorsContextGuard guard{errors, *data, "v128"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(v128), features, errors));
  v128 result;
  memcpy(&result, bytes.data(), sizeof(v128));
  return result;
}

optional<ValueType> Read(SpanU8* data,
                         const Features& features,
                         Errors& errors,
                         Tag<ValueType>) {
  ErrorsContextGuard guard{errors, *data, "value type"};
  WASP_TRY_READ(val, Read<u8>(data, features, errors));
  auto decoded = encoding::ValueType::Decode(val, features);
  if (!decoded) {
    errors.OnError(*data, format("Unknown value type: {}", val));
    return nullopt;
  }
  return decoded;
}

}  // namespace binary
}  // namespace wasp

//
// Copyright 2018 WebAssembly Community Group participants
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

#include <type_traits>

#include "src/base/formatters.h"
#include "src/base/macros.h"
#include "src/binary/encoding.h"
#include "src/binary/errors_context_guard.h"
#include "src/binary/formatters.h"
#include "src/binary/types.h"

#define WASP_TRY_READ(var, call) \
  auto opt_##var = call;         \
  if (!opt_##var) {              \
    return nullopt;              \
  }                              \
  auto var = *opt_##var /* No semicolon. */

#define WASP_TRY_READ_CONTEXT(var, call, desc)                 \
  ErrorsContextGuard<Errors> guard_##var(errors, *data, desc); \
  WASP_TRY_READ(var, call);                                    \
  guard_##var.PopContext() /* No semicolon. */

namespace wasp {
namespace binary {

template <typename T>
struct Tag {};

template <typename T, typename Errors>
optional<T> Read(SpanU8* data, Errors&, Tag<T>);

template <typename T, typename Errors>
optional<T> Read(SpanU8* data, Errors& errors) {
  return Read(data, errors, Tag<T>{});
}

template <typename Errors>
optional<u8> Read(SpanU8* data, Errors& errors, Tag<u8>) {
  if (data->size() < 1) {
    errors.OnError(*data, "Unable to read u8");
    return nullopt;
  }

  u8 result{(*data)[0]};
  remove_prefix(data, 1);
  return result;
}

template <typename Errors>
optional<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Errors& errors) {
  if (data->size() < N) {
    errors.OnError(*data, format("Unable to read {} bytes", N));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  remove_prefix(data, N);
  return result;
}

template <typename S>
S SignExtend(typename std::make_unsigned<S>::type x, int N) {
  constexpr size_t kNumBits = sizeof(S) * 8;
  return static_cast<S>(x << (kNumBits - N - 1)) >> (kNumBits - N - 1);
}

template <typename T, typename Errors>
optional<T> ReadVarInt(SpanU8* data, Errors& errors, string_view desc) {
  using U = typename std::make_unsigned<T>::type;
  constexpr bool is_signed = std::is_signed<T>::value;
  constexpr int kMaxBytes = (sizeof(T) * 8 + 6) / 7;
  constexpr int kUsedBitsInLastByte = sizeof(T) * 8 - 7 * (kMaxBytes - 1);
  constexpr int kMaskBits = kUsedBitsInLastByte - (is_signed ? 1 : 0);
  constexpr u8 kMask = ~((1 << kMaskBits) - 1);
  constexpr u8 kOnes = kMask & 0x7f;

  ErrorsContextGuard<Errors> guard{errors, *data, desc};

  U result{};
  for (int i = 0;;) {
    WASP_TRY_READ(byte, Read<u8>(data, errors));

    const int shift = i * 7;
    result |= U(byte & 0x7f) << shift;

    if (++i == kMaxBytes) {
      if ((byte & kMask) == 0 || (is_signed && (byte & kMask) == kOnes)) {
        return static_cast<T>(result);
      }
      const u8 zero_ext = byte & ~kMask & 0x7f;
      const u8 one_ext = (byte | kOnes) & 0x7f;
      if (is_signed) {
        errors.OnError(
            *data, format("Last byte of {} must be sign "
                          "extension: expected {:#2x} or {:#2x}, got {:#2x}",
                          desc, zero_ext, one_ext, byte));
      } else {
        errors.OnError(*data, format("Last byte of {} must be zero "
                                     "extension: expected {:#2x}, got {:#2x}",
                                     desc, zero_ext, byte));
      }
      return nullopt;
    } else if ((byte & 0x80) == 0) {
      return is_signed ? SignExtend<T>(result, 6 + shift) : result;
    }
  }
}

template <typename Errors>
optional<u32> Read(SpanU8* data, Errors& errors, Tag<u32>) {
  return ReadVarInt<u32>(data, errors, "u32");
}

template <typename Errors>
optional<Index> ReadIndex(SpanU8* data, Errors& errors, string_view desc) {
  return ReadVarInt<Index>(data, errors, desc);
}

template <typename Errors>
optional<s32> Read(SpanU8* data, Errors& errors, Tag<s32>) {
  return ReadVarInt<s32>(data, errors, "s32");
}

template <typename Errors>
optional<s64> Read(SpanU8* data, Errors& errors, Tag<s64>) {
  return ReadVarInt<s64>(data, errors, "s64");
}

template <typename Errors>
optional<f32> Read(SpanU8* data, Errors& errors, Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  ErrorsContextGuard<Errors> guard{errors, *data, "f32"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), errors));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

template <typename Errors>
optional<f64> Read(SpanU8* data, Errors& errors, Tag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  ErrorsContextGuard<Errors> guard{errors, *data, "f64"};
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), errors));
  f64 result;
  memcpy(&result, bytes.data(), sizeof(f64));
  return result;
}

template <typename Errors>
optional<Index> ReadCheckLength(SpanU8* data,
                                Errors& errors,
                                string_view context_name,
                                string_view error_name) {
  WASP_TRY_READ(count, ReadIndex(data, errors, context_name));

  // There should be at least one byte per count, so if the data is smaller
  // than that, the module must be malformed.
  if (count > data->size()) {
    errors.OnError(*data, format("{} extends past end: {} > {}", error_name,
                                 count, data->size()));
    return nullopt;
  }

  return count;
}

template <typename Errors>
optional<Index> ReadCount(SpanU8* data, Errors& errors) {
  return ReadCheckLength(data, errors, "count", "Count");
}

template <typename Errors>
optional<Index> ReadLength(SpanU8* data, Errors& errors) {
  return ReadCheckLength(data, errors, "length", "Length");
}

template <typename Errors>
optional<string_view> ReadString(SpanU8* data,
                                 Errors& errors,
                                 string_view desc) {
  ErrorsContextGuard<Errors> guard{errors, *data, desc};
  WASP_TRY_READ(len, ReadLength(data, errors));
  string_view result{reinterpret_cast<const char*>(data->data()), len};
  remove_prefix(data, len);
  return result;
}

template <typename T, typename Errors>
optional<std::vector<T>> ReadVector(SpanU8* data,
                                    Errors& errors,
                                    string_view desc) {
  ErrorsContextGuard<Errors> guard{errors, *data, desc};
  std::vector<T> result;
  WASP_TRY_READ(len, ReadCount(data, errors));
  result.reserve(len);
  for (u32 i = 0; i < len; ++i) {
    WASP_TRY_READ(elt, Read<T>(data, errors));
    result.emplace_back(std::move(elt));
  }
  return result;
}

// -----------------------------------------------------------------------------

#define WASP_TRY_DECODE(out_var, in_var, Type, name)               \
  auto out_var = encoding::Type::Decode(in_var);                   \
  if (!out_var) {                                                  \
    errors.OnError(*data, format("Unknown " name ": {}", in_var)); \
    return nullopt;                                                \
  }

template <typename Errors>
optional<ValueType> Read(SpanU8* data, Errors& errors, Tag<ValueType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "value type"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, ValueType, "value type");
  return decoded;
}

template <typename Errors>
optional<BlockType> Read(SpanU8* data, Errors& errors, Tag<BlockType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "block type"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, BlockType, "block type");
  return decoded;
}

template <typename Errors>
optional<ElementType> Read(SpanU8* data, Errors& errors, Tag<ElementType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "element type"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, ElementType, "element type");
  return decoded;
}

template <typename Errors>
optional<ExternalKind> Read(SpanU8* data, Errors& errors, Tag<ExternalKind>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "external kind"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, ExternalKind, "external kind");
  return decoded;
}

template <typename Errors>
optional<Mutability> Read(SpanU8* data, Errors& errors, Tag<Mutability>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "mutability"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, Mutability, "mutability");
  return decoded;
}

template <typename Errors>
optional<SectionId> Read(SpanU8* data, Errors& errors, Tag<SectionId>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "section id"};
  WASP_TRY_READ(val, Read<u32>(data, errors));
  WASP_TRY_DECODE(decoded, val, Section, "section id");
  return decoded;
}

template <typename Errors>
optional<Opcode> Read(SpanU8* data, Errors& errors, Tag<Opcode>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "opcode"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, Opcode, "opcode");
  return decoded;
}

template <typename Errors>
optional<NameSubsectionId> Read(SpanU8* data,
                                Errors& errors,
                                Tag<NameSubsectionId>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "name subsection id"};
  WASP_TRY_READ(val, Read<u8>(data, errors));
  WASP_TRY_DECODE(decoded, val, NameSubsectionId, "name subsection id");
  return decoded;
}

#undef WASP_TRY_DECODE

template <typename Errors>
optional<u8> ReadReserved(SpanU8* data, Errors& errors) {
  ErrorsContextGuard<Errors> guard{errors, *data, "reserved"};
  WASP_TRY_READ(reserved, Read<u8>(data, errors));
  if (reserved != 0) {
    errors.OnError(*data, format("Expected reserved byte 0, got {}", reserved));
    return nullopt;
  }
  return 0;
}

template <typename Errors>
optional<MemArg> Read(SpanU8* data, Errors& errors, Tag<MemArg>) {
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, errors), "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, errors), "offset");
  return MemArg{align_log2, offset};
}

template <typename Errors>
optional<Limits> Read(SpanU8* data, Errors& errors, Tag<Limits>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "limits"};
  WASP_TRY_READ_CONTEXT(flags, Read<u8>(data, errors), "flags");
  switch (flags) {
    case encoding::Limits::Flags_NoMax: {
      WASP_TRY_READ_CONTEXT(min, Read<u32>(data, errors), "min");
      return Limits{min};
    }

    case encoding::Limits::Flags_HasMax: {
      WASP_TRY_READ_CONTEXT(min, Read<u32>(data, errors), "min");
      WASP_TRY_READ_CONTEXT(max, Read<u32>(data, errors), "max");
      return Limits{min, max};
    }

    default:
      errors.OnError(*data, format("Invalid flags value: {}", flags));
      return nullopt;
  }
}

template <typename Errors>
optional<Locals> Read(SpanU8* data, Errors& errors, Tag<Locals>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "locals"};
  WASP_TRY_READ(count, ReadIndex(data, errors, "count"));
  WASP_TRY_READ_CONTEXT(type, Read<ValueType>(data, errors), "type");
  return Locals{count, type};
}

template <typename Errors>
optional<Section> Read(SpanU8* data, Errors& errors, Tag<Section>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "section"};
  WASP_TRY_READ(id, Read<SectionId>(data, errors));
  WASP_TRY_READ(length, ReadLength(data, errors));
  auto bytes = *ReadBytes(data, length, errors);

  if (id == SectionId::Custom) {
    WASP_TRY_READ(name, ReadString(&bytes, errors, "custom section name"));
    return Section{CustomSection{name, bytes}};
  } else {
    return Section{KnownSection{id, bytes}};
  }
}

template <typename Errors>
optional<FunctionType> Read(SpanU8* data, Errors& errors, Tag<FunctionType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "function type"};
  WASP_TRY_READ(param_types,
                ReadVector<ValueType>(data, errors, "param types"));
  WASP_TRY_READ(result_types,
                ReadVector<ValueType>(data, errors, "result types"));
  return FunctionType{std::move(param_types), std::move(result_types)};
}

template <typename Errors>
optional<TypeEntry> Read(SpanU8* data, Errors& errors, Tag<TypeEntry>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "type entry"};
  WASP_TRY_READ_CONTEXT(form, Read<u8>(data, errors), "form");

  if (form != encoding::Type::Function) {
    errors.OnError(*data, format("Unknown type form: {}", form));
    return nullopt;
  }

  WASP_TRY_READ(function_type, Read<FunctionType>(data, errors));
  return TypeEntry{std::move(function_type)};
}

template <typename Errors>
optional<TableType> Read(SpanU8* data, Errors& errors, Tag<TableType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "table type"};
  WASP_TRY_READ(elemtype, Read<ElementType>(data, errors));
  WASP_TRY_READ(limits, Read<Limits>(data, errors));
  return TableType{limits, elemtype};
}

template <typename Errors>
optional<MemoryType> Read(SpanU8* data, Errors& errors, Tag<MemoryType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "memory type"};
  WASP_TRY_READ(limits, Read<Limits>(data, errors));
  return MemoryType{limits};
}

template <typename Errors>
optional<GlobalType> Read(SpanU8* data, Errors& errors, Tag<GlobalType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "global type"};
  WASP_TRY_READ(type, Read<ValueType>(data, errors));
  WASP_TRY_READ(mut, Read<Mutability>(data, errors));
  return GlobalType{type, mut};
}

template <typename Errors>
optional<Import> Read(SpanU8* data, Errors& errors, Tag<Import>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "import"};
  WASP_TRY_READ(module, ReadString(data, errors, "module name"));
  WASP_TRY_READ(name, ReadString(data, errors, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, errors));
  switch (kind) {
    case ExternalKind::Function: {
      WASP_TRY_READ(type_index, ReadIndex(data, errors, "function index"));
      return Import{module, name, type_index};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, errors));
      return Import{module, name, table_type};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, errors));
      return Import{module, name, memory_type};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, errors));
      return Import{module, name, global_type};
    }
  }
  WASP_UNREACHABLE();
}

template <typename Errors>
optional<BrTableImmediate> Read(SpanU8* data,
                                Errors& errors,
                                Tag<BrTableImmediate>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "br_table"};
  WASP_TRY_READ(targets, ReadVector<Index>(data, errors, "targets"));
  WASP_TRY_READ(default_target, ReadIndex(data, errors, "default target"));
  return BrTableImmediate{std::move(targets), default_target};
}

template <typename Errors>
optional<CallIndirectImmediate> Read(SpanU8* data,
                                     Errors& errors,
                                     Tag<CallIndirectImmediate>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "call_indirect"};
  WASP_TRY_READ(index, ReadIndex(data, errors, "type index"));
  WASP_TRY_READ(reserved, ReadReserved(data, errors));
  return CallIndirectImmediate{index, reserved};
}

template <typename Errors>
optional<ConstantExpression> Read(SpanU8* data,
                                  Errors& errors,
                                  Tag<ConstantExpression>) {
  SpanU8 orig_data = *data;
  ErrorsContextGuard<Errors> guard{errors, *data, "constant expression"};
  WASP_TRY_READ(instr, Read<Instruction>(data, errors));
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

  WASP_TRY_READ(end, Read<Instruction>(data, errors));
  if (end.opcode != Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }
  return ConstantExpression{
      orig_data.subspan(0, data->begin() - orig_data.begin())};
}

template <typename Errors>
optional<Instruction> Read(SpanU8* data, Errors& errors, Tag<Instruction>) {
  WASP_TRY_READ(opcode, Read<Opcode>(data, errors));
  switch (opcode) {
    // No immediates:
    case Opcode::End:
    case Opcode::Unreachable:
    case Opcode::Nop:
    case Opcode::Else:
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
      return Instruction{Opcode{opcode}};

    // Type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If: {
      WASP_TRY_READ(type, Read<BlockType>(data, errors));
      return Instruction{Opcode{opcode}, type};
    }

    // Index immediate.
    case Opcode::Br:
    case Opcode::BrIf:
    case Opcode::Call:
    case Opcode::LocalGet:
    case Opcode::LocalSet:
    case Opcode::LocalTee:
    case Opcode::GlobalGet:
    case Opcode::GlobalSet: {
      WASP_TRY_READ(index, ReadIndex(data, errors, "index"));
      return Instruction{Opcode{opcode}, index};
    }

    // Index* immediates.
    case Opcode::BrTable: {
      WASP_TRY_READ(immediate, Read<BrTableImmediate>(data, errors));
      return Instruction{Opcode{opcode}, std::move(immediate)};
    }

    // Index, reserved immediates.
    case Opcode::CallIndirect: {
      WASP_TRY_READ(immediate, Read<CallIndirectImmediate>(data, errors));
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
    case Opcode::I32Store:
    case Opcode::I64Store:
    case Opcode::F32Store:
    case Opcode::F64Store:
    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32: {
      WASP_TRY_READ(memarg, Read<MemArg>(data, errors));
      return Instruction{Opcode{opcode}, memarg};
    }

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow: {
      WASP_TRY_READ(reserved, ReadReserved(data, errors));
      return Instruction{Opcode{opcode}, reserved};
    }

    // Const immediates.
    case Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, errors), "i32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, errors), "i64 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, errors), "f32 constant");
      return Instruction{Opcode{opcode}, value};
    }

    case Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, errors), "f64 constant");
      return Instruction{Opcode{opcode}, value};
    }
  }
  WASP_UNREACHABLE();
}

template <typename Errors>
optional<Function> Read(SpanU8* data, Errors& errors, Tag<Function>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "function"};
  WASP_TRY_READ(type_index, ReadIndex(data, errors, "type index"));
  return Function{type_index};
}

template <typename Errors>
optional<Table> Read(SpanU8* data, Errors& errors, Tag<Table>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "table"};
  WASP_TRY_READ(table_type, Read<TableType>(data, errors));
  return Table{table_type};
}

template <typename Errors>
optional<Memory> Read(SpanU8* data, Errors& errors, Tag<Memory>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "memory"};
  WASP_TRY_READ(memory_type, Read<MemoryType>(data, errors));
  return Memory{memory_type};
}

template <typename Errors>
optional<Global> Read(SpanU8* data, Errors& errors, Tag<Global>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "global"};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, errors));
  WASP_TRY_READ(init_expr, Read<ConstantExpression>(data, errors));
  return Global{global_type, std::move(init_expr)};
}

template <typename Errors>
optional<Export> Read(SpanU8* data, Errors& errors, Tag<Export>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "export"};
  WASP_TRY_READ(name, ReadString(data, errors, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, errors));
  WASP_TRY_READ(index, ReadIndex(data, errors, "index"));
  return Export{kind, name, index};
}

template <typename Errors>
optional<Start> Read(SpanU8* data, Errors& errors, Tag<Start>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, errors, "function index"));
  return Start{index};
}

template <typename Errors>
optional<ElementSegment> Read(SpanU8* data,
                              Errors& errors,
                              Tag<ElementSegment>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "element segment"};
  WASP_TRY_READ(table_index, ReadIndex(data, errors, "table index"));
  WASP_TRY_READ_CONTEXT(offset, Read<ConstantExpression>(data, errors),
                        "offset");
  WASP_TRY_READ(init, ReadVector<Index>(data, errors, "initializers"));
  return ElementSegment{table_index, std::move(offset), std::move(init)};
}

template <typename Errors>
optional<Code> Read(SpanU8* data, Errors& errors, Tag<Code>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "code"};
  WASP_TRY_READ(body_size, ReadLength(data, errors));
  WASP_TRY_READ(body, ReadBytes(data, body_size, errors));
  WASP_TRY_READ(locals, ReadVector<Locals>(&body, errors, "locals vector"));
  return Code{std::move(locals), Expression{std::move(body)}};
}

template <typename Errors>
optional<DataSegment> Read(SpanU8* data, Errors& errors, Tag<DataSegment>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "data segment"};
  WASP_TRY_READ(memory_index, ReadIndex(data, errors, "memory index"));
  WASP_TRY_READ_CONTEXT(offset, Read<ConstantExpression>(data, errors),
                        "offset");
  WASP_TRY_READ(len, ReadLength(data, errors));
  WASP_TRY_READ(init, ReadBytes(data, len, errors));
  return DataSegment{memory_index, std::move(offset), init};
}

template <typename Errors>
optional<NameAssoc> Read(SpanU8* data, Errors& errors, Tag<NameAssoc>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, errors, "index"));
  WASP_TRY_READ(name, ReadString(data, errors, "name"));
  return NameAssoc{index, name};
}

template <typename Errors>
optional<IndirectNameAssoc> Read(SpanU8* data,
                                 Errors& errors,
                                 Tag<IndirectNameAssoc>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "indirect name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, errors, "index"));
  WASP_TRY_READ(name_map, ReadVector<NameAssoc>(data, errors, "name map"));
  return IndirectNameAssoc{index, std::move(name_map)};
}

template <typename Errors>
optional<NameSubsection> Read(SpanU8* data,
                              Errors& errors,
                              Tag<NameSubsection>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "name subsection"};
  WASP_TRY_READ(id, Read<NameSubsectionId>(data, errors));
  WASP_TRY_READ(length, ReadLength(data, errors));
  auto bytes = *ReadBytes(data, length, errors);
  return NameSubsection{id, bytes};
}

#undef WASP_TRY_READ
#undef WASP_TRY_READ_CONTEXT

}  // namespace binary
}  // namespace wasp

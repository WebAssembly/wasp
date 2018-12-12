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
#include "src/binary/encoding.h"

#define WASP_TRY_READ(var, call) \
  auto opt_##var = call;         \
  if (!opt_##var) {              \
    return nullopt;              \
  }                              \
  auto var = *opt_##var /* No semicolon. */

#define WASP_TRY_READ_CONTEXT(var, call, desc) \
  errors.PushContext(*data, desc);             \
  WASP_TRY_READ(var, call);                    \
  errors.PopContext();

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
  *data = remove_prefix(*data, 1);
  return result;
}

template <typename Errors>
optional<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Errors& errors) {
  if (data->size() < N) {
    errors.OnError(*data, format("Unable to read {} bytes", N));
    return nullopt;
  }

  SpanU8 result{data->begin(), N};
  *data = remove_prefix(*data, N);
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
      if (is_signed) {
        errors.OnError(*data, format("Last byte of {} must be sign "
                                     "extension: expected {:#x}, got {:#02x}",
                                     desc, kOnes, byte & kMask));
      } else {
        errors.OnError(*data, format("Last byte of %s must be zero "
                                     "extension: expected 0, got {:02x}",
                                     desc, byte & kMask));
      }
      return nullopt;
    } else if ((byte & 0x80) == 0) {
      return is_signed ? SignExtend<T>(result, 6 + shift) : result;
    }
  }
}

template <typename Errors>
optional<u32> Read(SpanU8* data, Errors& errors, Tag<u32>) {
  return ReadVarInt<u32>(data, errors, "vu32");
}

template <typename Errors>
optional<Index> ReadIndex(SpanU8* data, Errors& errors) {
  return ReadVarInt<Index>(data, errors, "index");
}

template <typename Errors>
optional<s32> Read(SpanU8* data, Errors& errors, Tag<s32>) {
  return ReadVarInt<s32>(data, errors, "vs32");
}

template <typename Errors>
optional<s64> Read(SpanU8* data, Errors& errors, Tag<s64>) {
  return ReadVarInt<s64>(data, errors, "vs64");
}

template <typename Errors>
optional<f32> Read(SpanU8* data, Errors& errors, Tag<f32>) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f32), errors));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

template <typename Errors>
optional<f64> Read(SpanU8* data, Errors& errors, Tag<f64>) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  WASP_TRY_READ(bytes, ReadBytes(data, sizeof(f64), errors));
  f64 result;
  memcpy(&result, bytes.data(), sizeof(f64));
  return result;
}

template <typename Errors>
optional<Index> ReadCount(SpanU8* data, Errors& errors) {
  WASP_TRY_READ(count, ReadIndex(data, errors));

  // There should be at least one byte per count, so if the data is smaller
  // than that, the module must be malformed.
  if (count > data->size()) {
    errors.OnError(
        *data, format("Count is longer than the data length: {} > {}", count,
                      data->size()));
    return nullopt;
  }

  return count;
}

template <typename Errors>
optional<string_view> ReadStr(SpanU8* data, Errors& errors, string_view desc) {
  ErrorsContextGuard<Errors> guard{errors, *data, desc};
  WASP_TRY_READ(len, ReadCount(data, errors));
  if (len > data->size()) {
    errors.OnError(*data, format("Unable to read string of length {}", len));
    return nullopt;
  }

  string_view result{reinterpret_cast<const char*>(data->data()), len};
  *data = remove_prefix(*data, len);
  return result;
}

template <typename T, typename Errors>
optional<std::vector<T>> ReadVec(SpanU8* data,
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

template <typename Sequence>
void LazySequenceIteratorBase<Sequence>::Increment() {
  if (!empty()) {
    value_ = Read(&data_, sequence_->errors_, Tag<value_type>{});
    if (!value_) {
      clear();
    }
  } else {
    clear();
  }
}

template <typename Sequence>
LazySequenceIterator<Sequence>::LazySequenceIterator(Sequence* seq, SpanU8 data)
    : base{seq, data} {
  if (!this->empty()) {
    operator++();
  }
}

template <typename Sequence>
auto LazySequenceIterator<Sequence>::operator++() -> LazySequenceIterator& {
  this->Increment();
  return *this;
}

template <typename Sequence>
auto LazySequenceIterator<Sequence>::operator++(int) -> LazySequenceIterator {
  auto temp = *this;
  operator++();
  return temp;
}

// -----------------------------------------------------------------------------

template <typename Errors>
LazyModule<Errors>::LazyModule(SpanU8 data, Errors& errors)
    : magic{ReadBytes(&data, 4, errors)},
      version{ReadBytes(&data, 4, errors)},
      sections{data, errors} {
  const SpanU8 kMagic{encoding::Magic};
  const SpanU8 kVersion{encoding::Version};

  if (magic != kMagic) {
    errors.OnError(
        data, format("Magic mismatch: expected {}, got {}", kMagic, *magic));
  }

  if (version != kVersion) {
    errors.OnError(data, format("Version mismatch: expected {}, got {}",
                                kVersion, *version));
  }
}

// -----------------------------------------------------------------------------

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(SpanU8 data, Errors& errors)
    : count(ReadCount(&data, errors)), sequence(data, errors) {}

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(KnownSection<> section, Errors& errors)
    : LazySection(section.data, errors) {}

template <typename Errors>
StartSection<Errors>::StartSection(SpanU8 data, Errors& errors)
    : errors_(errors), start_(Read<Start>(&data, errors)) {}

template <typename Errors>
StartSection<Errors>::StartSection(KnownSection<> section, Errors& errors)
    : errors_(errors), start_(Read<Start>(&section.data, errors)) {}

template <typename Errors>
optional<Start> StartSection<Errors>::start() {
  return start_;
}

// -----------------------------------------------------------------------------

template <typename Errors>
optional<ValType> Read(SpanU8* data, Errors& errors, Tag<ValType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "value type"};
  WASP_TRY_READ(val_s32, Read<s32>(data, errors));
  switch (val_s32) {
    case encoding::ValType::I32:     return ValType::I32;
    case encoding::ValType::I64:     return ValType::I64;
    case encoding::ValType::F32:     return ValType::F32;
    case encoding::ValType::F64:     return ValType::F64;
    case encoding::ValType::Anyfunc: return ValType::Anyfunc;
    case encoding::ValType::Func:    return ValType::Func;
    case encoding::ValType::Void:    return ValType::Void;
    default:
      errors.OnError(*data, format("Unknown value type {}", val_s32));
      return nullopt;
  }
}

template <typename Errors>
optional<ExternalKind> Read(SpanU8* data, Errors& errors, Tag<ExternalKind>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "external kind"};
  WASP_TRY_READ(byte, Read<u8>(data, errors));
  switch (byte) {
    case encoding::ExternalKind::Func:   return ExternalKind::Func;
    case encoding::ExternalKind::Table:  return ExternalKind::Table;
    case encoding::ExternalKind::Memory: return ExternalKind::Memory;
    case encoding::ExternalKind::Global: return ExternalKind::Global;
    default:
      errors.OnError(*data, format("Unknown external kind {}", byte));
      return nullopt;
  }
}

template <typename Errors>
optional<Mutability> Read(SpanU8* data, Errors& errors, Tag<Mutability>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "mutability"};
  WASP_TRY_READ(byte, Read<u8>(data, errors));
  switch (byte) {
    case encoding::Mutability::Var:   return Mutability::Var;
    case encoding::Mutability::Const: return Mutability::Const;
    default:
      errors.OnError(*data, format("Unknown mutability {}", byte));
      return nullopt;
  }
}

template <typename Errors>
optional<Limits> Read(SpanU8* data, Errors& errors, Tag<Limits>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "limits"};
  const u32 kFlags_HasMax = 1;
  WASP_TRY_READ_CONTEXT(flags, Read<u32>(data, errors), "flags");
  WASP_TRY_READ_CONTEXT(min, Read<u32>(data, errors), "min");

  if (flags & kFlags_HasMax) {
    WASP_TRY_READ_CONTEXT(max, Read<u32>(data, errors), "max");
    return Limits{min, max};
  } else {
    return Limits{min};
  }
}

template <typename Errors>
optional<LocalDecl> Read(SpanU8* data, Errors& errors, Tag<LocalDecl>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "local decl"};
  WASP_TRY_READ_CONTEXT(count, ReadIndex(data, errors), "count");
  WASP_TRY_READ_CONTEXT(type, Read<ValType>(data, errors), "type");
  return LocalDecl{count, type};
}

template <typename Errors>
optional<FuncType> Read(SpanU8* data, Errors& errors, Tag<FuncType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "func type"};
  WASP_TRY_READ(param_types, ReadVec<ValType>(data, errors, "param types"));
  WASP_TRY_READ(result_types, ReadVec<ValType>(data, errors, "result types"));
  return FuncType{std::move(param_types), std::move(result_types)};
}

template <typename Errors>
optional<TypeEntry> Read(SpanU8* data, Errors& errors, Tag<TypeEntry>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "type entry"};
  WASP_TRY_READ_CONTEXT(form, Read<ValType>(data, errors), "form");

  if (form != ValType::Func) {
    errors.OnError(*data, format("Unknown type form: {}", form));
    return nullopt;
  }

  WASP_TRY_READ(func_type, Read<FuncType>(data, errors));
  return TypeEntry{form, std::move(func_type)};
}

template <typename Errors>
optional<TableType> Read(SpanU8* data, Errors& errors, Tag<TableType>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "table type"};
  WASP_TRY_READ_CONTEXT(elemtype, Read<ValType>(data, errors), "element type");
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
  WASP_TRY_READ(type, Read<ValType>(data, errors));
  WASP_TRY_READ(mut, Read<Mutability>(data, errors));
  return GlobalType{type, mut};
}

template <typename Errors>
optional<Section<>> Read(SpanU8* data, Errors& errors, Tag<Section<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "section"};
  WASP_TRY_READ_CONTEXT(id, Read<u32>(data, errors), "id");
  WASP_TRY_READ_CONTEXT(len, Read<u32>(data, errors), "length");
  if (len > data->size()) {
    errors.OnError(*data, format("Section length is too long: {} > {}", len,
                                 data->size()));
    return nullopt;
  }

  SpanU8 section_span = data->subspan(0, len);
  *data = remove_prefix(*data, len);

  if (id == encoding::Section::Custom) {
    WASP_TRY_READ(name, ReadStr(&section_span, errors, "custom section name"));
    return Section<>{CustomSection<>{name, section_span}};
  } else {
    return Section<>{KnownSection<>{id, section_span}};
  }
}

template <typename Errors>
optional<Import<>> Read(SpanU8* data, Errors& errors, Tag<Import<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "import"};
  WASP_TRY_READ(module, ReadStr(data, errors, "module name"));
  WASP_TRY_READ(name, ReadStr(data, errors, "field name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, errors));
  switch (kind) {
    case ExternalKind::Func: {
      WASP_TRY_READ(type_index, ReadIndex(data, errors));
      return Import<>{module, name, type_index};
    }
    case ExternalKind::Table: {
      WASP_TRY_READ(table_type, Read<TableType>(data, errors));
      return Import<>{module, name, table_type};
    }
    case ExternalKind::Memory: {
      WASP_TRY_READ(memory_type, Read<MemoryType>(data, errors));
      return Import<>{module, name, memory_type};
    }
    case ExternalKind::Global: {
      WASP_TRY_READ(global_type, Read<GlobalType>(data, errors));
      return Import<>{module, name, global_type};
    }
  }
}

template <typename Errors>
optional<ConstExpr<>> Read(SpanU8* data, Errors& errors, Tag<ConstExpr<>>) {
  LazyInstrs<Errors> instrs{*data, errors};
  auto iter = instrs.begin(), end = instrs.end();

  // Read instruction.
  if (iter == end) {
    errors.OnError(*data, "Unexpected end of const expr");
    return nullopt;
  }
  auto instr = *iter++;
  switch (instr.opcode.code) {
    case encoding::Opcode::I32Const:
    case encoding::Opcode::I64Const:
    case encoding::Opcode::F32Const:
    case encoding::Opcode::F64Const:
    case encoding::Opcode::GetGlobal:
      // OK.
      break;

    default:
      errors.OnError(*data,
                     format("Illegal instruction in const expr: {}", instr));
      return nullopt;
  }

  // Instruction must be followed by end.
  if (iter == end || iter->opcode.code != encoding::Opcode::End) {
    errors.OnError(*data, "Expected end instruction");
    return nullopt;
  }

  auto len = iter.data().begin() - data->begin();
  ConstExpr<> expr{data->subspan(0, len)};
  *data = remove_prefix(*data, len);
  return expr;
}

template <typename Errors>
optional<Instr> Read(SpanU8* data, Errors& errors, Tag<Instr>) {
  WASP_TRY_READ_CONTEXT(opcode, Read<u8>(data, errors), "opcode");
  switch (opcode) {
    // No immediates:
    case encoding::Opcode::End:
    case encoding::Opcode::Unreachable:
    case encoding::Opcode::Nop:
    case encoding::Opcode::Else:
    case encoding::Opcode::Return:
    case encoding::Opcode::Drop:
    case encoding::Opcode::Select:
    case encoding::Opcode::I32Eqz:
    case encoding::Opcode::I32Eq:
    case encoding::Opcode::I32Ne:
    case encoding::Opcode::I32LtS:
    case encoding::Opcode::I32LeS:
    case encoding::Opcode::I32LtU:
    case encoding::Opcode::I32LeU:
    case encoding::Opcode::I32GtS:
    case encoding::Opcode::I32GeS:
    case encoding::Opcode::I32GtU:
    case encoding::Opcode::I32GeU:
    case encoding::Opcode::I64Eqz:
    case encoding::Opcode::I64Eq:
    case encoding::Opcode::I64Ne:
    case encoding::Opcode::I64LtS:
    case encoding::Opcode::I64LeS:
    case encoding::Opcode::I64LtU:
    case encoding::Opcode::I64LeU:
    case encoding::Opcode::I64GtS:
    case encoding::Opcode::I64GeS:
    case encoding::Opcode::I64GtU:
    case encoding::Opcode::I64GeU:
    case encoding::Opcode::F32Eq:
    case encoding::Opcode::F32Ne:
    case encoding::Opcode::F32Lt:
    case encoding::Opcode::F32Le:
    case encoding::Opcode::F32Gt:
    case encoding::Opcode::F32Ge:
    case encoding::Opcode::F64Eq:
    case encoding::Opcode::F64Ne:
    case encoding::Opcode::F64Lt:
    case encoding::Opcode::F64Le:
    case encoding::Opcode::F64Gt:
    case encoding::Opcode::F64Ge:
    case encoding::Opcode::I32Clz:
    case encoding::Opcode::I32Ctz:
    case encoding::Opcode::I32Popcnt:
    case encoding::Opcode::I32Add:
    case encoding::Opcode::I32Sub:
    case encoding::Opcode::I32Mul:
    case encoding::Opcode::I32DivS:
    case encoding::Opcode::I32DivU:
    case encoding::Opcode::I32RemS:
    case encoding::Opcode::I32RemU:
    case encoding::Opcode::I32And:
    case encoding::Opcode::I32Or:
    case encoding::Opcode::I32Xor:
    case encoding::Opcode::I32Shl:
    case encoding::Opcode::I32ShrS:
    case encoding::Opcode::I32ShrU:
    case encoding::Opcode::I32Rotl:
    case encoding::Opcode::I32Rotr:
    case encoding::Opcode::I64Clz:
    case encoding::Opcode::I64Ctz:
    case encoding::Opcode::I64Popcnt:
    case encoding::Opcode::I64Add:
    case encoding::Opcode::I64Sub:
    case encoding::Opcode::I64Mul:
    case encoding::Opcode::I64DivS:
    case encoding::Opcode::I64DivU:
    case encoding::Opcode::I64RemS:
    case encoding::Opcode::I64RemU:
    case encoding::Opcode::I64And:
    case encoding::Opcode::I64Or:
    case encoding::Opcode::I64Xor:
    case encoding::Opcode::I64Shl:
    case encoding::Opcode::I64ShrS:
    case encoding::Opcode::I64ShrU:
    case encoding::Opcode::I64Rotl:
    case encoding::Opcode::I64Rotr:
    case encoding::Opcode::F32Abs:
    case encoding::Opcode::F32Neg:
    case encoding::Opcode::F32Ceil:
    case encoding::Opcode::F32Floor:
    case encoding::Opcode::F32Trunc:
    case encoding::Opcode::F32Nearest:
    case encoding::Opcode::F32Sqrt:
    case encoding::Opcode::F32Add:
    case encoding::Opcode::F32Sub:
    case encoding::Opcode::F32Mul:
    case encoding::Opcode::F32Div:
    case encoding::Opcode::F32Min:
    case encoding::Opcode::F32Max:
    case encoding::Opcode::F32Copysign:
    case encoding::Opcode::F64Abs:
    case encoding::Opcode::F64Neg:
    case encoding::Opcode::F64Ceil:
    case encoding::Opcode::F64Floor:
    case encoding::Opcode::F64Trunc:
    case encoding::Opcode::F64Nearest:
    case encoding::Opcode::F64Sqrt:
    case encoding::Opcode::F64Add:
    case encoding::Opcode::F64Sub:
    case encoding::Opcode::F64Mul:
    case encoding::Opcode::F64Div:
    case encoding::Opcode::F64Min:
    case encoding::Opcode::F64Max:
    case encoding::Opcode::F64Copysign:
    case encoding::Opcode::I32WrapI64:
    case encoding::Opcode::I32TruncSF32:
    case encoding::Opcode::I32TruncUF32:
    case encoding::Opcode::I32TruncSF64:
    case encoding::Opcode::I32TruncUF64:
    case encoding::Opcode::I64ExtendSI32:
    case encoding::Opcode::I64ExtendUI32:
    case encoding::Opcode::I64TruncSF32:
    case encoding::Opcode::I64TruncUF32:
    case encoding::Opcode::I64TruncSF64:
    case encoding::Opcode::I64TruncUF64:
    case encoding::Opcode::F32ConvertSI32:
    case encoding::Opcode::F32ConvertUI32:
    case encoding::Opcode::F32ConvertSI64:
    case encoding::Opcode::F32ConvertUI64:
    case encoding::Opcode::F32DemoteF64:
    case encoding::Opcode::F64ConvertSI32:
    case encoding::Opcode::F64ConvertUI32:
    case encoding::Opcode::F64ConvertSI64:
    case encoding::Opcode::F64ConvertUI64:
    case encoding::Opcode::F64PromoteF32:
    case encoding::Opcode::I32ReinterpretF32:
    case encoding::Opcode::I64ReinterpretF64:
    case encoding::Opcode::F32ReinterpretI32:
    case encoding::Opcode::F64ReinterpretI64:
      return Instr{Opcode{opcode}};

    // Type immediate.
    case encoding::Opcode::Block:
    case encoding::Opcode::Loop:
    case encoding::Opcode::If: {
      WASP_TRY_READ_CONTEXT(type, Read<ValType>(data, errors), "block type");
      return Instr{Opcode{opcode}, type};
    }

    // Index immediate.
    case encoding::Opcode::Br:
    case encoding::Opcode::BrIf:
    case encoding::Opcode::Call:
    case encoding::Opcode::GetLocal:
    case encoding::Opcode::SetLocal:
    case encoding::Opcode::TeeLocal:
    case encoding::Opcode::GetGlobal:
    case encoding::Opcode::SetGlobal: {
      WASP_TRY_READ(index, ReadIndex(data, errors));
      return Instr{Opcode{opcode}, index};
    }

    // Index* immediates.
    case encoding::Opcode::BrTable: {
      WASP_TRY_READ(targets, ReadVec<Index>(data, errors, "br_table targets"));
      WASP_TRY_READ_CONTEXT(default_target, ReadIndex(data, errors),
                            "br_table default target");
      return Instr{Opcode{opcode},
                   BrTableImmediate{std::move(targets), default_target}};
    }

    // Index, reserved immediates.
    case encoding::Opcode::CallIndirect: {
      WASP_TRY_READ(index, ReadIndex(data, errors));
      WASP_TRY_READ_CONTEXT(reserved, Read<u8>(data, errors), "reserved");
      return Instr{Opcode{opcode}, CallIndirectImmediate{index, reserved}};
    }

    // Memarg (alignment, offset) immediates.
    case encoding::Opcode::I32Load:
    case encoding::Opcode::I64Load:
    case encoding::Opcode::F32Load:
    case encoding::Opcode::F64Load:
    case encoding::Opcode::I32Load8S:
    case encoding::Opcode::I32Load8U:
    case encoding::Opcode::I32Load16S:
    case encoding::Opcode::I32Load16U:
    case encoding::Opcode::I64Load8S:
    case encoding::Opcode::I64Load8U:
    case encoding::Opcode::I64Load16S:
    case encoding::Opcode::I64Load16U:
    case encoding::Opcode::I64Load32S:
    case encoding::Opcode::I64Load32U:
    case encoding::Opcode::I32Store:
    case encoding::Opcode::I64Store:
    case encoding::Opcode::F32Store:
    case encoding::Opcode::F64Store:
    case encoding::Opcode::I32Store8:
    case encoding::Opcode::I32Store16:
    case encoding::Opcode::I64Store8:
    case encoding::Opcode::I64Store16:
    case encoding::Opcode::I64Store32: {
      WASP_TRY_READ(memarg, Read<MemArg>(data, errors));
      return Instr{Opcode{opcode}, memarg};
    }

    // Reserved immediates.
    case encoding::Opcode::MemorySize:
    case encoding::Opcode::MemoryGrow: {
      WASP_TRY_READ_CONTEXT(reserved, Read<u8>(data, errors), "reserved");
      return Instr{Opcode{opcode}, reserved};
    }

    // Const immediates.
    case encoding::Opcode::I32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s32>(data, errors), "i32 constant");
      return Instr{Opcode{opcode}, value};
    }

    case encoding::Opcode::I64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<s64>(data, errors), "i64 constant");
      return Instr{Opcode{opcode}, value};
    }

    case encoding::Opcode::F32Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f32>(data, errors), "f32 constant");
      return Instr{Opcode{opcode}, value};
    }

    case encoding::Opcode::F64Const: {
      WASP_TRY_READ_CONTEXT(value, Read<f64>(data, errors), "f64 constant");
      return Instr{Opcode{opcode}, value};
    }

    default:
      errors.OnError(*data, format("Unknown opcode {:#02x}", opcode));
      return nullopt;
  }
}

template <typename Errors>
optional<Func> Read(SpanU8* data, Errors& errors, Tag<Func>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "func"};
  WASP_TRY_READ(type_index, ReadIndex(data, errors));
  return Func{type_index};
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
optional<Global<>> Read(SpanU8* data, Errors& errors, Tag<Global<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "global"};
  WASP_TRY_READ(global_type, Read<GlobalType>(data, errors));
  WASP_TRY_READ(init_expr, Read<ConstExpr<>>(data, errors));
  return Global<>{global_type, std::move(init_expr)};
}

template <typename Errors>
optional<Export<>> Read(SpanU8* data, Errors& errors, Tag<Export<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "export"};
  WASP_TRY_READ(name, ReadStr(data, errors, "name"));
  WASP_TRY_READ(kind, Read<ExternalKind>(data, errors));
  WASP_TRY_READ(index, ReadIndex(data, errors));
  return Export<>{kind, name, index};
}

template <typename Errors>
optional<MemArg> Read(SpanU8* data, Errors& errors, Tag<MemArg>) {
  WASP_TRY_READ_CONTEXT(align_log2, Read<u32>(data, errors), "align log2");
  WASP_TRY_READ_CONTEXT(offset, Read<u32>(data, errors), "offset");
  return MemArg{align_log2, offset};
}

template <typename Errors>
optional<Start> Read(SpanU8* data, Errors& errors, Tag<Start>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "start"};
  WASP_TRY_READ(index, ReadIndex(data, errors));
  return Start{index};
}

template <typename Errors>
optional<ElementSegment<>> Read(SpanU8* data,
                                Errors& errors,
                                Tag<ElementSegment<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "element segment"};
  WASP_TRY_READ_CONTEXT(table_index, ReadIndex(data, errors), "table index");
  WASP_TRY_READ_CONTEXT(offset, Read<ConstExpr<>>(data, errors), "offset");
  WASP_TRY_READ(init, ReadVec<Index>(data, errors, "initializers"));
  return ElementSegment<>{table_index, std::move(offset), std::move(init)};
}

template <typename Errors>
optional<Code<>> Read(SpanU8* data, Errors& errors, Tag<Code<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "code"};
  WASP_TRY_READ(body_size, ReadCount(data, errors));
  WASP_TRY_READ(body, ReadBytes(data, body_size, errors));
  WASP_TRY_READ(local_decls, ReadVec<LocalDecl>(&body, errors, "local decls"));
  return Code<>{std::move(local_decls), Expr<>{std::move(body)}};
}

template <typename Errors>
optional<DataSegment<>> Read(SpanU8* data, Errors& errors, Tag<DataSegment<>>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "data segment"};
  WASP_TRY_READ_CONTEXT(memory_index, ReadIndex(data, errors), "memory index");
  WASP_TRY_READ_CONTEXT(offset, Read<ConstExpr<>>(data, errors), "offset");
  WASP_TRY_READ(len, ReadCount(data, errors));
  WASP_TRY_READ(init, ReadBytes(data, len, errors));
  return DataSegment<>{memory_index, std::move(offset), init};
}

}  // namespace binary
}  // namespace wasp

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

#ifndef WASP_BINARY_WRITE_H_
#define WASP_BINARY_WRITE_H_

#include <cstring>
#include <limits>
#include <type_traits>

#include "wasp/base/types.h"
#include "wasp/binary/encoding.h"  // XXX
#include "wasp/binary/encoding/block_type_encoding.h"
#include "wasp/binary/encoding/element_type_encoding.h"
#include "wasp/binary/encoding/event_attribute_encoding.h"
#include "wasp/binary/encoding/external_kind_encoding.h"
#include "wasp/binary/encoding/limits_flags_encoding.h"
#include "wasp/binary/encoding/mutability_encoding.h"
#include "wasp/binary/encoding/opcode_encoding.h"
#include "wasp/binary/encoding/section_id_encoding.h"
#include "wasp/binary/encoding/segment_flags_encoding.h"
#include "wasp/binary/encoding/value_type_encoding.h"
#include "wasp/binary/types.h"
#include "wasp/binary/var_int.h"

namespace wasp {
namespace binary {

template <typename Iterator>
Iterator Write(u8 value, Iterator out) {
  *out++ = value;
  return out;
}

template <typename T, typename Iterator, typename Cond>
Iterator WriteVarIntLoop(T value, Iterator out, Cond&& end_cond) {
  do {
    const u8 byte = value & VarInt<T>::kByteMask;
    value >>= 7;
    if (end_cond(value, byte)) {
      out = Write(byte, out);
      break;
    } else {
      out = Write(byte | VarInt<T>::kExtendBit, out);
    }
  } while (true);
  return out;
}

// Unsigned integers.
template <typename T, typename Iterator>
typename std::enable_if<!std::is_signed<T>::value, Iterator>::type WriteVarInt(
    T value,
    Iterator out) {
  return WriteVarIntLoop(value, out,
                         [](T value, u8 byte) { return value == 0; });
}

// Signed integers.
template <typename T, typename Iterator>
typename std::enable_if<std::is_signed<T>::value, Iterator>::type WriteVarInt(
    T value,
    Iterator out) {
  if (value < 0) {
    return WriteVarIntLoop(value, out, [](T value, u8 byte) {
      return value == -1 && (byte & VarInt<T>::kSignBit) != 0;
    });
  } else {
    return WriteVarIntLoop(value, out, [](T value, u8 byte) {
      return value == 0 && (byte & VarInt<T>::kSignBit) == 0;
    });
  }
}

template <typename Iterator>
Iterator Write(u32 value, Iterator out) {
  return WriteVarInt(value, out);
}

template <typename Iterator>
Iterator Write(Opcode value, Iterator out) {
  auto encoded = encoding::Opcode::Encode(value);
  out = Write(encoded.u8_code, out);
  if (encoded.u32_code) {
    out = Write(*encoded.u32_code, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(ValueType value, Iterator out) {
  return Write(encoding::ValueType::Encode(value), out);
}

template <typename Iterator>
Iterator WriteBytes(SpanU8 value, Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator Write(s32 value, Iterator out) {
  return WriteVarInt(value, out);
}

template <typename Iterator>
Iterator Write(s64 value, Iterator out) {
  return WriteVarInt(value, out);
}

template <typename Iterator>
Iterator WriteIndex(Index value, Iterator out) {
  return Write(value, out);
}

template <typename Iterator>
Iterator Write(BlockType value, Iterator out) {
  return Write(encoding::BlockType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const BrOnExnImmediate& value, Iterator out) {
  out = WriteIndex(value.target, out);
  out = WriteIndex(value.event_index, out);
  return out;
}


template <typename InputIterator, typename OutputIterator>
OutputIterator WriteVector(InputIterator in_begin,
                           InputIterator in_end,
                           OutputIterator out) {
  size_t count = std::distance(in_begin, in_end);
  assert(count < std::numeric_limits<u32>::max());
  out = Write(static_cast<u32>(count), out);
  for (auto it = in_begin; it != in_end; ++it) {
    out = Write(*it, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(const BrTableImmediate& immediate, Iterator out) {
  out = WriteVector(immediate.targets.begin(), immediate.targets.end(), out);
  out = WriteIndex(immediate.default_target, out);
  return out;
}

template <typename Iterator>
Iterator Write(string_view value, Iterator out) {
  assert(value.size() < std::numeric_limits<u32>::max());
  u32 value_size = value.size();
  out = Write(value_size, out);
  return WriteBytes(
      SpanU8{reinterpret_cast<const u8*>(value.data()), value_size}, out);
}

template <typename Iterator>
Iterator Write(const CallIndirectImmediate& immediate, Iterator out) {
  out = WriteIndex(immediate.index, out);
  out = Write(immediate.table_index, out);
  return out;
}

template <typename Iterator>
Iterator Write(const Comdat& value, Iterator out) {
  out = Write(value.name, out);
  out = Write(value.flags, out);
  out = WriteVector(value.symbols.begin(), value.symbols.end(), out);
  return out;
}

template <typename Iterator>
Iterator Write(const ComdatSymbol& value, Iterator out) {
  out = Write(value.kind, out);
  out = WriteIndex(value.index, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ConstantExpression& value, Iterator out) {
  out = Write(value.instruction, out);
  out = Write(Opcode::End, out);
  return out;
}

template <typename Iterator>
Iterator Write(const CopyImmediate& immediate, Iterator out) {
  out = Write(immediate.src_index, out);
  out = Write(immediate.dst_index, out);
  return out;
}

template <typename Iterator>
Iterator Write(const DataSegment& value, Iterator out) {
  encoding::DecodedDataSegmentFlags flags = {
      value.type, value.memory_index && *value.memory_index != 0
                      ? encoding::HasNonZeroIndex::Yes
                      : encoding::HasNonZeroIndex::No};

  out = Write(encoding::DataSegmentFlags::Encode(flags), out);
  if (flags.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    out = Write(*value.memory_index, out);
  }
  if (flags.segment_type == SegmentType::Active) {
    out = Write(*value.offset, out);
  }
  assert(value.init.size() < std::numeric_limits<u32>::max());
  out = Write(static_cast<u32>(value.init.size()), out);
  out = WriteBytes(value.init, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ElementExpression& value, Iterator out) {
  out = Write(value.instruction, out);
  out = Write(Opcode::End, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ElementSegment& value, Iterator out) {
  encoding::DecodedElemSegmentFlags flags = {
      value.type,
      value.table_index && *value.table_index != 0
          ? encoding::HasNonZeroIndex::Yes
          : encoding::HasNonZeroIndex::No,
      value.has_expressions() ? encoding::HasExpressions::Yes
                              : encoding::HasExpressions::No};

  out = Write(encoding::ElemSegmentFlags::Encode(flags), out);
  if (flags.has_non_zero_index == encoding::HasNonZeroIndex::Yes) {
    out = Write(*value.table_index, out);
  }
  if (flags.segment_type == SegmentType::Active) {
    out = Write(*value.offset, out);
  }
  if (flags.has_expressions == encoding::HasExpressions::Yes) {
    const auto& desc = value.expressions();
    if (!flags.is_legacy_active()) {
      out = Write(desc.element_type, out);
    }
    out = WriteVector(desc.init.begin(), desc.init.end(), out);
  } else {
    const auto& desc = value.indexes();
    if (!flags.is_legacy_active()) {
      out = Write(desc.kind, out);
    }
    out = WriteVector(desc.init.begin(), desc.init.end(), out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(ElementType value, Iterator out) {
  return Write(encoding::ElementType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const EventAttribute& value, Iterator out) {
  return Write(encoding::EventAttribute::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const EventType& value, Iterator out) {
  out = Write(value.attribute, out);
  return Write(value.type_index, out);
}

template <typename Iterator>
Iterator Write(const Event& value, Iterator out) {
  out = Write(value.event_type, out);
  return out;
}

template <typename Iterator>
Iterator Write(const Export& value, Iterator out) {
  out = Write(value.name, out);
  out = Write(value.kind, out);
  return WriteIndex(value.index, out);
}

template <typename Iterator>
Iterator Write(ExternalKind value, Iterator out) {
  return Write(encoding::ExternalKind::Encode(value), out);
}

template <typename Iterator>
Iterator Write(f32 value, Iterator out) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  u8 bytes[4];
  memcpy(bytes, &value, sizeof(value));
  return WriteBytes(SpanU8(bytes, 4), out);
}

template <typename Iterator>
Iterator Write(f64 value, Iterator out) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  u8 bytes[8];
  memcpy(bytes, &value, sizeof(value));
  return WriteBytes(SpanU8(bytes, 8), out);
}

// Unsigned integers.
template <typename T, typename Iterator>
typename std::enable_if<!std::is_signed<T>::value, Iterator>::type
WriteFixedVarInt(T value, Iterator out, size_t length = VarInt<T>::kMaxBytes) {
  using V = VarInt<T>;
  assert(length <= V::kMaxBytes);
  for (size_t i = 0; i < length - 1; ++i) {
    out = Write(static_cast<u8>((value & V::kByteMask) | V::kExtendBit), out);
    value >>= V::kBitsPerByte;
  }
  out = Write(static_cast<u8>(value & V::kByteMask), out);
  assert((value >> V::kBitsPerByte) == 0);
  return out;
}

// Signed integers.
template <typename T, typename Iterator>
typename std::enable_if<std::is_signed<T>::value, Iterator>::type
WriteFixedVarInt(T value, Iterator out, size_t length = VarInt<T>::kMaxBytes) {
  using V = VarInt<T>;
  assert(length <= V::kMaxBytes);
  for (size_t i = 0; i < length - 1; ++i) {
    out = Write(static_cast<u8>((value & V::kByteMask) | V::kExtendBit), out);
    value >>= V::kBitsPerByte;
  }
  out = Write(static_cast<u8>(value & V::kByteMask), out);
  assert((value >> V::kBitsPerByte) == 0 || (value >> V::kBitsPerByte) == -1);
  return out;
}

template <typename Iterator>
Iterator Write(const Function& value,
                        Iterator out) {
  return Write(value.type_index, out);
}

template <typename Iterator>
Iterator Write(const FunctionType& value, Iterator out) {
  out = WriteVector(value.param_types.begin(), value.param_types.end(), out);
  return WriteVector(value.result_types.begin(), value.result_types.end(), out);
}

template <typename Iterator>
Iterator Write(const Global& value, Iterator out) {
  out = Write(value.global_type, out);
  out = Write(value.init, out);
  return out;
}

template <typename Iterator>
Iterator Write(const GlobalType& value, Iterator out) {
  out = Write(value.valtype, out);
  return Write(value.mut, out);
}

template <typename Iterator>
Iterator Write(const Import& value, Iterator out) {
  out = Write(value.module, out);
  out = Write(value.name, out);
  out = Write(value.kind(), out);
  switch (value.kind()) {
    case ExternalKind::Function:
      return WriteIndex(value.index(), out);

    case ExternalKind::Table:
      return Write(value.table_type(), out);

    case ExternalKind::Memory:
      return Write(value.memory_type(), out);

    case ExternalKind::Global:
      return Write(value.global_type(), out);

    case ExternalKind::Event:
      return Write(value.event_type(), out);

    default:
      WASP_UNREACHABLE();
  }
}

template <typename Iterator>
Iterator Write(const InitImmediate& immediate, Iterator out) {
  out = WriteIndex(immediate.segment_index, out);
  out = Write(immediate.dst_index, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ShuffleImmediate& immediate, Iterator out) {
  for (int i = 0; i < 16; ++i) {
    out = Write(immediate[i], out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(v128 value, Iterator out) {
  static_assert(sizeof(v128) == 16, "sizeof(v128) != 16");
  u8 bytes[16];
  memcpy(bytes, &value, sizeof(value));
  return WriteBytes(SpanU8(bytes, 16), out);
}

template <typename Iterator>
Iterator Write(const Instruction& instr, Iterator out) {
  out = Write(instr.opcode, out);
  switch (instr.opcode) {
    // No immediates:
    case Opcode::End:
    case Opcode::Unreachable:
    case Opcode::Nop:
    case Opcode::Else:
    case Opcode::Catch:
    case Opcode::Rethrow:
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
    case Opcode::I8X16Abs:
    case Opcode::I16X8Abs:
    case Opcode::I32X4Abs:
      return out;

    // Type immediate.
    case Opcode::Block:
    case Opcode::Loop:
    case Opcode::If:
    case Opcode::Try:
      return Write(instr.block_type_immediate(), out);

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
    case Opcode::TableFill:
      return Write(instr.index_immediate(), out);

    // Index, Index immediates.
    case Opcode::BrOnExn:
      return Write(instr.br_on_exn_immediate(), out);

    // Index* immediates.
    case Opcode::BrTable:
      return Write(instr.br_table_immediate(), out);

    // Index, reserved immediates.
    case Opcode::CallIndirect:
    case Opcode::ReturnCallIndirect:
      return Write(instr.call_indirect_immediate(), out);

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
    case Opcode::I64AtomicRmw32CmpxchgU:
      return Write(instr.mem_arg_immediate(), out);

    // Reserved immediates.
    case Opcode::MemorySize:
    case Opcode::MemoryGrow:
    case Opcode::MemoryFill:
      return Write(instr.u8_immediate(), out);

    // Const immediates.
    case Opcode::I32Const:
      return Write(instr.s32_immediate(), out);

    case Opcode::I64Const:
      return Write(instr.s64_immediate(), out);

    case Opcode::F32Const:
      return Write(instr.f32_immediate(), out);

    case Opcode::F64Const:
      return Write(instr.f64_immediate(), out);

    case Opcode::V128Const:
      return Write(instr.v128_immediate(), out);

    // Reserved, Index immediates.
    case Opcode::MemoryInit:
    case Opcode::TableInit:
      return Write(instr.init_immediate(), out);

    // Reserved, reserved immediates.
    case Opcode::MemoryCopy:
    case Opcode::TableCopy:
      return Write(instr.copy_immediate(), out);

    // Shuffle immediate.
    case Opcode::V8X16Shuffle:
      return Write(instr.shuffle_immediate(), out);

    // ValueTypes immediate.
    case Opcode::SelectT:
      return WriteVector(instr.value_types_immediate().begin(),
                         instr.value_types_immediate().end(), out);

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
    case Opcode::F64X2ReplaceLane:
      return Write(instr.u8_immediate(), out);
  }
  WASP_UNREACHABLE();
}

template <typename Iterator>
Iterator Write(const Limits& limits, Iterator out) {
  out = Write(encoding::LimitsFlags::Encode(limits), out);
  out = Write(limits.min, out);
  if (limits.max) {
    out = Write(*limits.max, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(const Locals& value, Iterator out) {
  out = WriteIndex(value.count, out);
  out = Write(value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(const MemArgImmediate& immediate, Iterator out) {
  out = Write(immediate.align_log2, out);
  out = Write(immediate.offset, out);
  return out;
}

template <typename Iterator>
Iterator Write(const Memory& value, Iterator out) {
  return Write(value.memory_type, out);
}

template <typename Iterator>
Iterator Write(const MemoryType& value, Iterator out) {
  return Write(value.limits, out);
}

template <typename Iterator>
Iterator Write(Mutability value, Iterator out) {
  return Write(encoding::Mutability::Encode(value), out);
}

template <typename Iterator>
Iterator Write(SectionId value, Iterator out) {
  return Write(encoding::SectionId::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const Start& value, Iterator out) {
  return WriteIndex(value.func_index, out);
}

template <typename Iterator>
Iterator Write(const Table& value, Iterator out) {
  return Write(value.table_type, out);
}

template <typename Iterator>
Iterator Write(const TableType& value, Iterator out) {
  out = Write(value.elemtype, out);
  return Write(value.limits, out);
}

template <typename Iterator>
Iterator Write(const TypeEntry& value, Iterator out) {
  out = Write(encoding::Type::Function, out);
  return Write(value.type, out);
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_H_

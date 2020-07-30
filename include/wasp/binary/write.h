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

#include <cassert>
#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>

#include "wasp/base/buffer.h"
#include "wasp/base/macros.h"
#include "wasp/base/optional.h"
#include "wasp/base/types.h"
#include "wasp/base/wasm_types.h"
#include "wasp/binary/encoding.h"
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
std::enable_if_t<!std::is_signed_v<T>, Iterator> WriteVarInt(T value,
                                                             Iterator out) {
  return WriteVarIntLoop(value, out,
                         [](T value, u8 byte) { return value == 0; });
}

// Signed integers.
template <typename T, typename Iterator>
std::enable_if_t<std::is_signed_v<T>, Iterator> WriteVarInt(T value,
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
Iterator Write(s32 value, Iterator out) {
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
Iterator Write(encoding::EncodedValueType value, Iterator out) {
  out = Write(value.code, out);
  if (value.immediate) {
    out = Write(*value.immediate, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(ValueType value, Iterator out) {
  return Write(encoding::ValueType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(BlockType value, Iterator out) {
  return Write(encoding::BlockType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(HeapType value, Iterator out) {
  return Write(encoding::HeapType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(ReferenceType value, Iterator out) {
  return Write(encoding::ReferenceType::Encode(value), out);
}

template <typename Iterator>
Iterator Write(ExternalKind value, Iterator out) {
  return Write(encoding::ExternalKind::Encode(value), out);
}

template <typename Iterator>
Iterator Write(const EventAttribute& value, Iterator out) {
  return Write(encoding::EventAttribute::Encode(value), out);
}

template <typename Iterator>
Iterator Write(Mutability value, Iterator out) {
  return Write(encoding::Mutability::Encode(value), out);
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
Iterator WriteBytes(SpanU8 value, Iterator out) {
  return std::copy(value.begin(), value.end(), out);
}

template <typename Iterator>
Iterator WriteLengthAndBytes(SpanU8 value, Iterator out) {
  assert(value.size() < std::numeric_limits<u32>::max());
  out = Write(u32(value.size()), out);
  out = WriteBytes(value, out);
  return out;
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
  return WriteLengthAndBytes(SpanU8{reinterpret_cast<const u8*>(value.data()),
                                    static_cast<span_extent_t>(value.size())},
                             out);
}

template <typename Iterator>
Iterator Write(const CallIndirectImmediate& immediate, Iterator out) {
  out = WriteIndex(immediate.index, out);
  out = Write(immediate.table_index, out);
  return out;
}

template <typename Iterator>
Iterator Write(Code value, Iterator out) {
  // Write Code to a separate buffer, so we know its length.
  Buffer buffer;
  auto code_out = std::back_inserter(buffer);
  code_out = WriteVector(value.locals.begin(), value.locals.end(), code_out);
  code_out = WriteBytes(value.body->data, code_out);

  // Then write that buffer to the real output.
  out = WriteLengthAndBytes(buffer, out);
  return out;
}

template <typename Iterator>
Iterator Write(const InstructionList& value, Iterator out) {
  for (auto&& instr : value) {
    out = Write(instr, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(const UnpackedExpression& value, Iterator out) {
  return Write(value.instructions, out);
}

template <typename Iterator>
Iterator Write(const UnpackedCode& value, Iterator out) {
  // Write Code to a separate buffer, so we know its length.
  Buffer buffer;
  auto code_out = std::back_inserter(buffer);
  code_out = WriteVector(value.locals.begin(), value.locals.end(), code_out);
  code_out = Write(value.body, code_out);

  // Then write that buffer to the real output.
  out = WriteLengthAndBytes(buffer, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ConstantExpression& value, Iterator out) {
  out = Write(value.instructions, out);
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
Iterator Write(const DataCount& value, Iterator out) {
  return Write(value.count, out);
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
  out = WriteLengthAndBytes(value.init, out);
  return out;
}

template <typename Iterator>
Iterator Write(const ElementExpression& value, Iterator out) {
  out = Write(value.instructions, out);
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
    const auto& elements = value.expressions();
    if (!flags.is_legacy_active()) {
      out = Write(elements.elemtype, out);
    }
    out = WriteVector(elements.list.begin(), elements.list.end(), out);
  } else {
    const auto& elements = value.indexes();
    if (!flags.is_legacy_active()) {
      out = Write(elements.kind, out);
    }
    out = WriteVector(elements.list.begin(), elements.list.end(), out);
  }
  return out;
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
std::enable_if_t<!std::is_signed_v<T>, Iterator>
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
std::enable_if_t<std::is_signed_v<T>, Iterator>
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
Iterator Write(const Function& value, Iterator out) {
  return Write(value.type_index, out);
}

template <typename Iterator>
Iterator Write(const FunctionType& value, Iterator out) {
  out = WriteVector(value.param_types.begin(), value.param_types.end(), out);
  return WriteVector(value.result_types.begin(), value.result_types.end(), out);
}

template <typename Iterator>
Iterator Write(const TableType& value, Iterator out) {
  out = Write(value.elemtype, out);
  return Write(value.limits, out);
}

template <typename Iterator>
Iterator Write(const MemoryType& value, Iterator out) {
  return Write(value.limits, out);
}

template <typename Iterator>
Iterator Write(const GlobalType& value, Iterator out) {
  out = Write(value.valtype, out);
  return Write(value.mut, out);
}

template <typename Iterator>
Iterator Write(const Global& value, Iterator out) {
  out = Write(value.global_type, out);
  out = Write(value.init, out);
  return out;
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
  switch (instr.immediate.index()) {
    case 0: // monostate
      if (instr.opcode == Opcode::MemorySize ||
          instr.opcode == Opcode::MemoryGrow ||
          instr.opcode == Opcode::MemoryFill) {
        return Write(u8{0}, out);
      }
      return out;

    case 1: // s32
      return Write(instr.s32_immediate(), out);

    case 2: // s64
      return Write(instr.s64_immediate(), out);

    case 3: // f32
      return Write(instr.f32_immediate(), out);

    case 4: // f64
      return Write(instr.f64_immediate(), out);

    case 5: // v128
      return Write(instr.v128_immediate(), out);

    case 6: // Index
      return Write(instr.index_immediate(), out);

    case 7: // BlockType
      return Write(instr.block_type_immediate(), out);

    case 8: // BrOnExnImmediate
      return Write(instr.br_on_exn_immediate(), out);

    case 9: // BrTableImmediate
      return Write(instr.br_table_immediate(), out);

    case 10: // CallIndirectImmediate
      return Write(instr.call_indirect_immediate(), out);

    case 11: // CopyImmediate
      return Write(instr.copy_immediate(), out);

    case 12: // InitImmediate
      return Write(instr.init_immediate(), out);

    case 13: // LetImmediate
      // TODO
      assert(false);
      return out;

    case 14: // MemArgImmediate
      return Write(instr.mem_arg_immediate(), out);

    case 15: // HeapType
      return Write(instr.heap_type_immediate(), out);

    case 16: // SelectImmediate
      return WriteVector(instr.select_immediate()->begin(),
                         instr.select_immediate()->end(), out);

    case 17: // ShuffleImmediate
      return Write(instr.shuffle_immediate(), out);

    case 18: // SimdLaneImmediate
      return Write(instr.simd_lane_immediate(), out);
  }
  WASP_UNREACHABLE();
}

template <typename Iterator>
Iterator Write(const Locals& value, Iterator out) {
  out = WriteIndex(value.count, out);
  out = Write(value.type, out);
  return out;
}

template <typename Iterator>
Iterator Write(const LetImmediate& immediate, Iterator out) {
  out = Write(immediate.block_type, out);
  out = WriteVector(immediate.locals.begin(), immediate.locals.end(), out);
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
Iterator Write(const TypeEntry& value, Iterator out) {
  out = Write(encoding::Type::Function, out);
  return Write(value.type, out);
}

template <typename InputIterator, typename OutputIterator>
OutputIterator WriteKnownSection(SectionId section_id,
                                 InputIterator in_begin,
                                 InputIterator in_end,
                                 OutputIterator out) {
  // Write to a separate buffer, so we know its length.
  Buffer buffer;
  WriteVector(in_begin, in_end, std::back_inserter(buffer));

  // Then write the section id, followed by the buffer to the real output.
  out = Write(section_id, out);
  out = WriteLengthAndBytes(buffer, out);
  return out;
}

template <typename Container, typename Iterator>
Iterator WriteNonEmptyKnownSection(SectionId section_id,
                                   Container container,
                                   Iterator out) {
  if (!container.empty()) {
    out = WriteKnownSection(section_id, std::begin(container),
                            std::end(container), out);
  }
  return out;
}

template <typename T, typename Iterator>
Iterator WriteNonEmptyKnownSection(SectionId section_id,
                                   const optional<T>& value_opt,
                                   Iterator out) {
  // Only write the section if the value is contained.
  if (value_opt) {
    // Write to a separate buffer, so we know its length.
    Buffer buffer;
    Write(*value_opt, std::back_inserter(buffer));

    // Then write the section id, followed by the buffer to the real output.
    out = Write(section_id, out);
    out = WriteLengthAndBytes(buffer, out);
  }
  return out;
}

template <typename Iterator>
Iterator Write(const Module& value, Iterator out) {
  out = WriteBytes(encoding::Magic, out);
  out = WriteBytes(encoding::Version, out);
  out = WriteNonEmptyKnownSection(SectionId::Type, value.types, out);
  out = WriteNonEmptyKnownSection(SectionId::Import, value.imports, out);
  out = WriteNonEmptyKnownSection(SectionId::Function, value.functions, out);
  out = WriteNonEmptyKnownSection(SectionId::Table, value.tables, out);
  out = WriteNonEmptyKnownSection(SectionId::Memory, value.memories, out);
  out = WriteNonEmptyKnownSection(SectionId::Global, value.globals, out);
  out = WriteNonEmptyKnownSection(SectionId::Event, value.events, out);
  out = WriteNonEmptyKnownSection(SectionId::Export, value.exports, out);
  out = WriteNonEmptyKnownSection(SectionId::Start, value.start, out);
  out = WriteNonEmptyKnownSection(SectionId::Element, value.element_segments, out);
  out = WriteNonEmptyKnownSection(SectionId::DataCount, value.data_count, out);
  out = WriteNonEmptyKnownSection(SectionId::Code, value.codes, out);
  out = WriteNonEmptyKnownSection(SectionId::Data, value.data_segments, out);
  return out;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_H_

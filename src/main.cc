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

#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/container/fixed_array.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"

////////////////////////////////////////////////////////////////////////////////

using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = int64_t;
using f32 = float;
using f64 = double;

using Index = u32;

using SpanU8 = absl::Span<const u8>;

template <typename T>
using optional = absl::optional<T>;

using string_view = absl::string_view;

////////////////////////////////////////////////////////////////////////////////

namespace encoding {

constexpr u8 Magic[] = {0, 'a', 's', 'm'};
constexpr u8 Version[] = {1, 0, 0, 0};

struct ValType {
  static constexpr s32 I32 = -0x01;
  static constexpr s32 I64 = -0x02;
  static constexpr s32 F32 = -0x03;
  static constexpr s32 F64 = -0x04;
  static constexpr s32 Anyfunc = -0x10;
  static constexpr s32 Func = -0x20;
  static constexpr s32 Void = -0x40;
};

struct ExternalKind {
  static constexpr u8 Func = 0;
  static constexpr u8 Table = 1;
  static constexpr u8 Memory = 2;
  static constexpr u8 Global = 3;
};

struct Section {
  static constexpr u32 Custom = 0;
  static constexpr u32 Type = 1;
  static constexpr u32 Import = 2;
  static constexpr u32 Function = 3;
  static constexpr u32 Table = 4;
  static constexpr u32 Memory = 5;
  static constexpr u32 Global = 6;
  static constexpr u32 Export = 7;
  static constexpr u32 Start = 8;
  static constexpr u32 Element = 9;
  static constexpr u32 Code = 10;
  static constexpr u32 Data = 11;
};

struct Opcode {
  static constexpr u8 Unreachable = 0x00;
  static constexpr u8 Nop = 0x01;
  static constexpr u8 Block = 0x02;
  static constexpr u8 Loop = 0x03;
  static constexpr u8 If = 0x04;
  static constexpr u8 Else = 0x05;
  static constexpr u8 Try = 0x06;
  static constexpr u8 Catch = 0x07;
  static constexpr u8 Throw = 0x08;
  static constexpr u8 Rethrow = 0x09;
  static constexpr u8 IfExcept = 0x0a;
  static constexpr u8 End = 0x0b;
  static constexpr u8 Br = 0x0c;
  static constexpr u8 BrIf = 0x0d;
  static constexpr u8 BrTable = 0x0e;
  static constexpr u8 Return = 0x0f;
  static constexpr u8 Call = 0x10;
  static constexpr u8 CallIndirect = 0x11;
  static constexpr u8 ReturnCall = 0x12;
  static constexpr u8 ReturnCallIndirect = 0x13;
  static constexpr u8 Drop = 0x1a;
  static constexpr u8 Select = 0x1b;
  static constexpr u8 GetLocal = 0x20;
  static constexpr u8 SetLocal = 0x21;
  static constexpr u8 TeeLocal = 0x22;
  static constexpr u8 GetGlobal = 0x23;
  static constexpr u8 SetGlobal = 0x24;
  static constexpr u8 I32Load = 0x28;
  static constexpr u8 I64Load = 0x29;
  static constexpr u8 F32Load = 0x2a;
  static constexpr u8 F64Load = 0x2b;
  static constexpr u8 I32Load8S = 0x2c;
  static constexpr u8 I32Load8U = 0x2d;
  static constexpr u8 I32Load16S = 0x2e;
  static constexpr u8 I32Load16U = 0x2f;
  static constexpr u8 I64Load8S = 0x30;
  static constexpr u8 I64Load8U = 0x31;
  static constexpr u8 I64Load16S = 0x32;
  static constexpr u8 I64Load16U = 0x33;
  static constexpr u8 I64Load32S = 0x34;
  static constexpr u8 I64Load32U = 0x35;
  static constexpr u8 I32Store = 0x36;
  static constexpr u8 I64Store = 0x37;
  static constexpr u8 F32Store = 0x38;
  static constexpr u8 F64Store = 0x39;
  static constexpr u8 I32Store8 = 0x3a;
  static constexpr u8 I32Store16 = 0x3b;
  static constexpr u8 I64Store8 = 0x3c;
  static constexpr u8 I64Store16 = 0x3d;
  static constexpr u8 I64Store32 = 0x3e;
  static constexpr u8 MemorySize = 0x3f;
  static constexpr u8 MemoryGrow = 0x40;
  static constexpr u8 I32Const = 0x41;
  static constexpr u8 I64Const = 0x42;
  static constexpr u8 F32Const = 0x43;
  static constexpr u8 F64Const = 0x44;
  static constexpr u8 I32Eqz = 0x45;
  static constexpr u8 I32Eq = 0x46;
  static constexpr u8 I32Ne = 0x47;
  static constexpr u8 I32LtS = 0x48;
  static constexpr u8 I32LtU = 0x49;
  static constexpr u8 I32GtS = 0x4a;
  static constexpr u8 I32GtU = 0x4b;
  static constexpr u8 I32LeS = 0x4c;
  static constexpr u8 I32LeU = 0x4d;
  static constexpr u8 I32GeS = 0x4e;
  static constexpr u8 I32GeU = 0x4f;
  static constexpr u8 I64Eqz = 0x50;
  static constexpr u8 I64Eq = 0x51;
  static constexpr u8 I64Ne = 0x52;
  static constexpr u8 I64LtS = 0x53;
  static constexpr u8 I64LtU = 0x54;
  static constexpr u8 I64GtS = 0x55;
  static constexpr u8 I64GtU = 0x56;
  static constexpr u8 I64LeS = 0x57;
  static constexpr u8 I64LeU = 0x58;
  static constexpr u8 I64GeS = 0x59;
  static constexpr u8 I64GeU = 0x5a;
  static constexpr u8 F32Eq = 0x5b;
  static constexpr u8 F32Ne = 0x5c;
  static constexpr u8 F32Lt = 0x5d;
  static constexpr u8 F32Gt = 0x5e;
  static constexpr u8 F32Le = 0x5f;
  static constexpr u8 F32Ge = 0x60;
  static constexpr u8 F64Eq = 0x61;
  static constexpr u8 F64Ne = 0x62;
  static constexpr u8 F64Lt = 0x63;
  static constexpr u8 F64Gt = 0x64;
  static constexpr u8 F64Le = 0x65;
  static constexpr u8 F64Ge = 0x66;
  static constexpr u8 I32Clz = 0x67;
  static constexpr u8 I32Ctz = 0x68;
  static constexpr u8 I32Popcnt = 0x69;
  static constexpr u8 I32Add = 0x6a;
  static constexpr u8 I32Sub = 0x6b;
  static constexpr u8 I32Mul = 0x6c;
  static constexpr u8 I32DivS = 0x6d;
  static constexpr u8 I32DivU = 0x6e;
  static constexpr u8 I32RemS = 0x6f;
  static constexpr u8 I32RemU = 0x70;
  static constexpr u8 I32And = 0x71;
  static constexpr u8 I32Or = 0x72;
  static constexpr u8 I32Xor = 0x73;
  static constexpr u8 I32Shl = 0x74;
  static constexpr u8 I32ShrS = 0x75;
  static constexpr u8 I32ShrU = 0x76;
  static constexpr u8 I32Rotl = 0x77;
  static constexpr u8 I32Rotr = 0x78;
  static constexpr u8 I64Clz = 0x79;
  static constexpr u8 I64Ctz = 0x7a;
  static constexpr u8 I64Popcnt = 0x7b;
  static constexpr u8 I64Add = 0x7c;
  static constexpr u8 I64Sub = 0x7d;
  static constexpr u8 I64Mul = 0x7e;
  static constexpr u8 I64DivS = 0x7f;
  static constexpr u8 I64DivU = 0x80;
  static constexpr u8 I64RemS = 0x81;
  static constexpr u8 I64RemU = 0x82;
  static constexpr u8 I64And = 0x83;
  static constexpr u8 I64Or = 0x84;
  static constexpr u8 I64Xor = 0x85;
  static constexpr u8 I64Shl = 0x86;
  static constexpr u8 I64ShrS = 0x87;
  static constexpr u8 I64ShrU = 0x88;
  static constexpr u8 I64Rotl = 0x89;
  static constexpr u8 I64Rotr = 0x8a;
  static constexpr u8 F32Abs = 0x8b;
  static constexpr u8 F32Neg = 0x8c;
  static constexpr u8 F32Ceil = 0x8d;
  static constexpr u8 F32Floor = 0x8e;
  static constexpr u8 F32Trunc = 0x8f;
  static constexpr u8 F32Nearest = 0x90;
  static constexpr u8 F32Sqrt = 0x91;
  static constexpr u8 F32Add = 0x92;
  static constexpr u8 F32Sub = 0x93;
  static constexpr u8 F32Mul = 0x94;
  static constexpr u8 F32Div = 0x95;
  static constexpr u8 F32Min = 0x96;
  static constexpr u8 F32Max = 0x97;
  static constexpr u8 F32Copysign = 0x98;
  static constexpr u8 F64Abs = 0x99;
  static constexpr u8 F64Neg = 0x9a;
  static constexpr u8 F64Ceil = 0x9b;
  static constexpr u8 F64Floor = 0x9c;
  static constexpr u8 F64Trunc = 0x9d;
  static constexpr u8 F64Nearest = 0x9e;
  static constexpr u8 F64Sqrt = 0x9f;
  static constexpr u8 F64Add = 0xa0;
  static constexpr u8 F64Sub = 0xa1;
  static constexpr u8 F64Mul = 0xa2;
  static constexpr u8 F64Div = 0xa3;
  static constexpr u8 F64Min = 0xa4;
  static constexpr u8 F64Max = 0xa5;
  static constexpr u8 F64Copysign = 0xa6;
  static constexpr u8 I32WrapI64 = 0xa7;
  static constexpr u8 I32TruncSF32 = 0xa8;
  static constexpr u8 I32TruncUF32 = 0xa9;
  static constexpr u8 I32TruncSF64 = 0xaa;
  static constexpr u8 I32TruncUF64 = 0xab;
  static constexpr u8 I64ExtendSI32 = 0xac;
  static constexpr u8 I64ExtendUI32 = 0xad;
  static constexpr u8 I64TruncSF32 = 0xae;
  static constexpr u8 I64TruncUF32 = 0xaf;
  static constexpr u8 I64TruncSF64 = 0xb0;
  static constexpr u8 I64TruncUF64 = 0xb1;
  static constexpr u8 F32ConvertSI32 = 0xb2;
  static constexpr u8 F32ConvertUI32 = 0xb3;
  static constexpr u8 F32ConvertSI64 = 0xb4;
  static constexpr u8 F32ConvertUI64 = 0xb5;
  static constexpr u8 F32DemoteF64 = 0xb6;
  static constexpr u8 F64ConvertSI32 = 0xb7;
  static constexpr u8 F64ConvertUI32 = 0xb8;
  static constexpr u8 F64ConvertSI64 = 0xb9;
  static constexpr u8 F64ConvertUI64 = 0xba;
  static constexpr u8 F64PromoteF32 = 0xbb;
  static constexpr u8 I32ReinterpretF32 = 0xbc;
  static constexpr u8 I64ReinterpretF64 = 0xbd;
  static constexpr u8 F32ReinterpretI32 = 0xbe;
  static constexpr u8 F64ReinterpretI64 = 0xbf;
};

}  // namespace encoding

////////////////////////////////////////////////////////////////////////////////

enum class ValType {
  I32,
  I64,
  F32,
  F64,
  Anyfunc,
  Func,
  Void,
};

enum class ExternalKind {
  Func,
  Table,
  Memory,
  Global,
};

struct MemArg {
  u32 align_log2;
  u32 offset;
};

struct Limits {
  u32 min;
  optional<u32> max;
};

struct LocalDecl {
  Index count;
  ValType type;
};

struct FuncType {
  std::vector<ValType> param_types;
  std::vector<ValType> result_types;
};

struct TableType {
  Limits limits;
  ValType elemtype;
};

struct MemoryType {
  Limits limits;
};

struct GlobalType {
  ValType valtype;
  bool mut;
};

struct Import {
  string_view module;
  string_view name;
  ExternalKind kind;
};

struct FuncImport : Import {
  FuncImport(string_view module, string_view name, Index type_index)
      : Import{module, name, ExternalKind::Func}, type_index(type_index) {}

  Index type_index;
};

struct TableImport : Import {
  TableImport(string_view module, string_view name, TableType table_type)
      : Import{module, name, ExternalKind::Table}, table_type(table_type) {}

  TableType table_type;
};

struct MemoryImport : Import {
  MemoryImport(string_view module, string_view name, MemoryType memory_type)
      : Import{module, name, ExternalKind::Memory}, memory_type(memory_type) {}

  MemoryType memory_type;
};

struct GlobalImport : Import {
  GlobalImport(string_view module, string_view name, GlobalType global_type)
      : Import{module, name, ExternalKind::Global}, global_type(global_type) {}

  GlobalType global_type;
};

struct Export {
  string_view name;
  ExternalKind kind;
  Index index;
};

struct Expr {
  SpanU8 instrs;
};

struct Global {
  GlobalType global_type;
  Expr init_expr;
};

struct ElementSegment {
  Index table_index;
  Expr offset;
  std::vector<Index> init;
};

struct DataSegment {
  Index memory_index;
  Expr offset;
  SpanU8 init;
};

std::string ToString(u32 x) {
  return absl::StrFormat("%u", x);
}

std::string ToString(const SpanU8& self) {
  std::string result = "\"";
  result += absl::StrJoin(self, "", [](std::string* out, u8 x) {
    absl::StrAppendFormat(out, "\\%02x", x);
  });
  absl::StrAppend(&result, "\"");
  return result;
}

std::string ToString(ValType self) {
  switch (self) {
    case ValType::I32: return "i32";
    case ValType::I64: return "i64";
    case ValType::F32: return "f32";
    case ValType::F64: return "f64";
    case ValType::Anyfunc: return "anyfunc";
    case ValType::Func: return "func";
    case ValType::Void: return "void";
  }
}

std::string ToString(ExternalKind self) {
  switch (self) {
    case ExternalKind::Func: return "func";
    case ExternalKind::Table: return "table";
    case ExternalKind::Memory: return "memory";
    case ExternalKind::Global: return "global";
  }
}

template <typename T>
std::string ToString(const std::vector<T>& self) {
  std::string result = "[";
  result += absl::StrJoin(
      self, " ", [](std::string* out, T x) { return *out += ToString(x); });
  absl::StrAppend(&result, "]");
  return result;
}

std::string ToString(const FuncType& self) {
  return absl::StrFormat("%s -> %s", ToString(self.param_types),
                         ToString(self.result_types));
}

std::string ToString(const MemArg& self) {
  return absl::StrFormat("{align %u, offset %u}", self.align_log2, self.offset);
}

std::string ToString(const Limits& self) {
  if (self.max) {
    return absl::StrFormat("{min %u, max %u}", self.min, *self.max);
  } else {
    return absl::StrFormat("{min %u}", self.min);
  }
}

std::string ToString(const LocalDecl& self) {
  return absl::StrFormat("%s ** %u", ToString(self.type), self.count);
}

std::string ToString(const TableType& self) {
  return absl::StrFormat("%s %s", ToString(self.limits),
                         ToString(self.elemtype));
}

std::string ToString(const MemoryType& self) {
  return absl::StrFormat("%s", ToString(self.limits));
}

std::string ToString(const GlobalType& self) {
  return absl::StrFormat("%s %s", self.mut ? "var" : "const",
                         ToString(self.valtype));
}

std::string ToString(const Import& self) {
  return absl::StrFormat("module \"%s\", name \"%s\", desc %s", self.module,
                         self.name, ToString(self.kind));
}

std::string ToString(const FuncImport& self) {
  return absl::StrFormat("{%s %d}", ToString(static_cast<const Import&>(self)),
                         self.type_index);
}

std::string ToString(const TableImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.table_type));
}

std::string ToString(const MemoryImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.memory_type));
}

std::string ToString(const GlobalImport& self) {
  return absl::StrFormat("{%s %s}", ToString(static_cast<const Import&>(self)),
                         ToString(self.global_type));
}

std::string ToString(const Export& self) {
  return absl::StrFormat("{name \"%s\", desc %s %u}", self.name,
                         ToString(self.kind), self.index);
}

std::string ToString(const Expr& self) {
  return absl::StrJoin(self.instrs, "", [](std::string* out, u8 x) {
    return absl::StrAppendFormat(out, "\\%02x", x);
  });
}

std::string ToString(const Global& self) {
  return absl::StrFormat("{type %s, init %s}", ToString(self.global_type),
                         ToString(self.init_expr));
}

std::string ToString(const ElementSegment& self) {
  return absl::StrFormat("{table %u, offset %s, init %s}", self.table_index,
                         ToString(self.offset), ToString(self.init));
}

std::string ToString(const DataSegment& self) {
  return absl::StrFormat("{table %u, offset %s, init %s}", self.memory_index,
                         ToString(self.offset), ToString(self.init));
}

////////////////////////////////////////////////////////////////////////////////

optional<std::vector<u8>> ReadFile(const std::string& filename) {
  std::ifstream stream{filename, std::ios::in | std::ios::binary};
  if (!stream) {
    return absl::nullopt;
  }

  std::vector<u8> buffer;
  stream.seekg(0, std::ios::end);
  buffer.resize(stream.tellg());
  stream.seekg(0, std::ios::beg);
  stream.read(reinterpret_cast<char*>(&buffer[0]), buffer.size());
  if (stream.fail()) {
    return absl::nullopt;
  }

  return buffer;
}

optional<u8> ReadU8(SpanU8* data) {
  if (data->size() < 1) {
    return absl::nullopt;
  }

  u8 result{(*data)[0]};
  data->remove_prefix(1);
  return result;
}

optional<SpanU8> ReadBytes(SpanU8* data, size_t N) {
  if (data->size() < N) {
    return absl::nullopt;
  }

  SpanU8 result{data->begin(), N};
  data->remove_prefix(N);
  return result;
}

#define READ(var, call)   \
  auto opt_##var = call;  \
  if (!opt_##var) {       \
    return absl::nullopt; \
  }                       \
  auto var = *opt_##var /* No semicolon. */

template <typename S>
S SignExtend(typename std::make_unsigned<S>::type x, int N) {
  constexpr size_t kNumBits = sizeof(S) * 8;
  return static_cast<S>(x << (kNumBits - N - 1)) >> (kNumBits - N - 1);
}

template <typename T>
optional<T> ReadVarInt(SpanU8* data) {
  using U = typename std::make_unsigned<T>::type;
  constexpr bool is_signed = std::is_signed<T>::value;
  constexpr int kMaxBytes = (sizeof(T) * 8 + 6) / 7;
  constexpr int kUsedBitsInLastByte = sizeof(T) * 8 - 7 * (kMaxBytes - 1);
  constexpr int kMaskBits = kUsedBitsInLastByte - (is_signed ? 1 : 0);
  constexpr u8 kMask = ~((1 << kMaskBits) - 1);
  constexpr u8 kOnes = kMask & 0x7f;

  U result{};
  for (int i = 0;;) {
    READ(byte, ReadU8(data));

    const int shift = i * 7;
    result |= U(byte & 0x7f) << shift;

    if (++i == kMaxBytes) {
      if ((byte & kMask) == 0 || (is_signed && (byte & kMask) == kOnes)) {
        return static_cast<T>(result);
      }
      return absl::nullopt;
    } else if ((byte & 0x80) == 0) {
      return is_signed ? SignExtend<T>(result, 6 + shift) : result;
    }
  }
}

optional<u32> ReadVarU32(SpanU8* data) {
  return ReadVarInt<u32>(data);
}

optional<Index> ReadIndex(SpanU8* data) {
  return ReadVarU32(data);
}

optional<s32> ReadVarS32(SpanU8* data) {
  return ReadVarInt<s32>(data);
}

optional<s64> ReadVarS64(SpanU8* data) {
  return ReadVarInt<s64>(data);
}

optional<f32> ReadF32(SpanU8* data) {
  static_assert(sizeof(f32) == 4, "sizeof(f32) != 4");
  READ(bytes, ReadBytes(data, sizeof(f32)));
  f32 result;
  memcpy(&result, bytes.data(), sizeof(f32));
  return result;
}

optional<f64> ReadF64(SpanU8* data) {
  static_assert(sizeof(f64) == 8, "sizeof(f64) != 8");
  READ(bytes, ReadBytes(data, sizeof(f64)));
  f64 result;
  memcpy(&result, bytes.data(), sizeof(f64));
  return result;
}

template <typename T, typename F>
optional<std::vector<T>> ReadVec(SpanU8* data, F&& read_element) {
  std::vector<T> result;
  READ(len, ReadVarU32(data));
  result.resize(len);
  for (u32 i = 0; i < len; ++i) {
    READ(elt, read_element(data));
    result[i] = std::move(elt);
  }
  return result;
}

optional<SpanU8> ReadVecU8(SpanU8* data) {
  READ(len, ReadVarU32(data));
  if (len > data->size()) {
    return absl::nullopt;
  }

  SpanU8 result{data->data(), len};
  data->remove_prefix(len);
  return result;
}

optional<ValType> ReadValType(SpanU8* data) {
  READ(val_s32, ReadVarS32(data));
  switch (val_s32) {
    case encoding::ValType::I32: return ValType::I32;
    case encoding::ValType::I64: return ValType::I64;
    case encoding::ValType::F32: return ValType::F32;
    case encoding::ValType::F64: return ValType::F64;
    case encoding::ValType::Anyfunc: return ValType::Anyfunc;
    case encoding::ValType::Func: return ValType::Func;
    case encoding::ValType::Void: return ValType::Void;
    default:    return absl::nullopt;
  }
}

optional<ExternalKind> ReadExternalKind(SpanU8* data) {
  READ(byte, ReadU8(data));
  switch (byte) {
    case encoding::ExternalKind::Func:   return ExternalKind::Func;
    case encoding::ExternalKind::Table:  return ExternalKind::Table;
    case encoding::ExternalKind::Memory: return ExternalKind::Memory;
    case encoding::ExternalKind::Global: return ExternalKind::Global;
    default: return absl::nullopt;
  }
}

optional<string_view> ReadStr(SpanU8* data) {
  READ(len, ReadVarU32(data));
  if (len > data->size()) {
    return absl::nullopt;
  }

  string_view result{reinterpret_cast<const char*>(data->data()), len};
  data->remove_prefix(len);
  return result;
}

optional<Limits> ReadLimits(SpanU8* data) {
  const u32 kFlags_HasMax = 1;
  READ(flags, ReadVarU32(data));
  READ(min, ReadVarU32(data));

  if (flags & kFlags_HasMax) {
    READ(max, ReadVarU32(data));
    return Limits{min, max};
  } else {
    return Limits{min};
  }
}

optional<TableType> ReadTableType(SpanU8* data) {
  READ(elemtype, ReadValType(data));
  READ(limits, ReadLimits(data));
  return TableType{limits, elemtype};
}

optional<MemoryType> ReadMemoryType(SpanU8* data) {
  READ(limits, ReadLimits(data));
  return MemoryType{limits};
}

optional<GlobalType> ReadGlobalType(SpanU8* data) {
  READ(type, ReadValType(data));
  READ(mut_byte, ReadU8(data));
  if (mut_byte > 1) {
    return absl::nullopt;
  }
  return GlobalType{type, mut_byte != 0};
}

struct BaseHooks {
  void OnError(const std::string&) {}
};

struct ExprHooks : BaseHooks {
  void OnOpcodeBare(u8 opcode) {}
  void OnOpcodeType(u8 opcode, ValType) {}
  void OnOpcodeIndex(u8 opcode, Index) {}
  void OnOpcodeCallIndirect(u8 opcode, Index, u8 reserved) {}
  void OnOpcodeBrTable(u8 opcode,
                       const std::vector<Index>& targets,
                       Index default_target) {}
  void OnOpcodeMemarg(u8 opcode, const MemArg&) {}
  void OnOpcodeI32Const(u8 opcode, s32) {}
  void OnOpcodeI64Const(u8 opcode, s64) {}
  void OnOpcodeF32Const(u8 opcode, f32) {}
  void OnOpcodeF64Const(u8 opcode, f64) {}
};

struct ModuleHooks : BaseHooks {
  void OnSection(u8 code, SpanU8 data) {}
};

struct TypeSectionHooks : BaseHooks {
  void OnTypeCount(Index count) {}
  void OnFuncType(Index type_index, const FuncType&) {}
};

struct ImportSectionHooks : BaseHooks {
  void OnImportCount(Index count) {}
  void OnFuncImport(Index import_index, const FuncImport&) {}
  void OnTableImport(Index import_index, const TableImport&) {}
  void OnMemoryImport(Index import_index, const MemoryImport&) {}
  void OnGlobalImport(Index import_index, const GlobalImport&) {}
};

struct FunctionSectionHooks : BaseHooks {
  void OnFuncCount(Index count) {}
  void OnFunc(Index func_index, Index type_index) {}
};

struct TableSectionHooks : BaseHooks {
  void OnTableCount(Index count) {}
  void OnTable(Index table_index, const TableType&) {}
};

struct MemorySectionHooks : BaseHooks {
  void OnMemoryCount(Index count) {}
  void OnMemory(Index memory_index, const MemoryType&) {}
};

struct GlobalSectionHooks : BaseHooks {
  void OnGlobalCount(Index count) {}
  void OnGlobal(Index global_index, const Global&) {}
};

struct ExportSectionHooks : BaseHooks {
  void OnExportCount(Index count) {}
  void OnExport(Index export_index, const Export&) {}
};

struct StartSectionHooks : BaseHooks {
  void OnStart(Index func_index) {}
};

struct ElementSectionHooks : BaseHooks {
  void OnElementSegmentCount(Index count) {}
  void OnElementSegment(Index segment_index, const ElementSegment&) {}
};

struct CodeSectionHooks : BaseHooks {
  void OnCodeCount(Index count) {}
  void OnCode(Index code_index, SpanU8 code) {}
};

struct CodeHooks : BaseHooks {
  void OnCodeContents(const std::vector<LocalDecl>& locals, const Expr& body) {}
};

struct DataSectionHooks : BaseHooks {
  void OnDataSegmentCount(Index count) {}
  void OnDataSegment(Index segment_index, const DataSegment&) {}
};

#define READ_OR_ERROR(var, call, desc)     \
  auto opt_##var = call;                   \
  if (!opt_##var) {                        \
    hooks.OnError("Unable to read " desc); \
    return {};                             \
  }                                        \
  auto var = *opt_##var /* No semicolon. */

template <typename Hooks = ExprHooks>
optional<Expr> ReadExpr(SpanU8* data, Hooks&& hooks = Hooks{}) {
  const u8* const start = data->data();
  int ends_expected = 1;

  while (ends_expected != 0) {
    READ_OR_ERROR(opcode, ReadU8(data), "opcode");
    switch (opcode) {
      // End has no immediates, but is handled specially; we exit from the loop
      // when we've closed all open blocks (and the implicit outer block).
      case encoding::Opcode::End:
        --ends_expected;
        hooks.OnOpcodeBare(opcode);
        break;

      // No immediates:
      case encoding::Opcode::Unreachable:
      case encoding::Opcode::Nop:
      case encoding::Opcode::Else:
      case encoding::Opcode::Return:
      case encoding::Opcode::Drop:
      case encoding::Opcode::Select:
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
      case encoding::Opcode::I32ShrU:
      case encoding::Opcode::I32ShrS:
      case encoding::Opcode::I32Rotr:
      case encoding::Opcode::I32Rotl:
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
      case encoding::Opcode::I64ShrU:
      case encoding::Opcode::I64ShrS:
      case encoding::Opcode::I64Rotr:
      case encoding::Opcode::I64Rotl:
      case encoding::Opcode::F32Add:
      case encoding::Opcode::F32Sub:
      case encoding::Opcode::F32Mul:
      case encoding::Opcode::F32Div:
      case encoding::Opcode::F32Min:
      case encoding::Opcode::F32Max:
      case encoding::Opcode::F32Copysign:
      case encoding::Opcode::F64Add:
      case encoding::Opcode::F64Sub:
      case encoding::Opcode::F64Mul:
      case encoding::Opcode::F64Div:
      case encoding::Opcode::F64Min:
      case encoding::Opcode::F64Max:
      case encoding::Opcode::F64Copysign:
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
      case encoding::Opcode::I64Clz:
      case encoding::Opcode::I64Ctz:
      case encoding::Opcode::I64Popcnt:
      case encoding::Opcode::F32Abs:
      case encoding::Opcode::F32Neg:
      case encoding::Opcode::F32Ceil:
      case encoding::Opcode::F32Floor:
      case encoding::Opcode::F32Trunc:
      case encoding::Opcode::F32Nearest:
      case encoding::Opcode::F32Sqrt:
      case encoding::Opcode::F64Abs:
      case encoding::Opcode::F64Neg:
      case encoding::Opcode::F64Ceil:
      case encoding::Opcode::F64Floor:
      case encoding::Opcode::F64Trunc:
      case encoding::Opcode::F64Nearest:
      case encoding::Opcode::F64Sqrt:
      case encoding::Opcode::I32TruncSF32:
      case encoding::Opcode::I32TruncSF64:
      case encoding::Opcode::I32TruncUF32:
      case encoding::Opcode::I32TruncUF64:
      case encoding::Opcode::I32WrapI64:
      case encoding::Opcode::I64TruncSF32:
      case encoding::Opcode::I64TruncSF64:
      case encoding::Opcode::I64TruncUF32:
      case encoding::Opcode::I64TruncUF64:
      case encoding::Opcode::I64ExtendSI32:
      case encoding::Opcode::I64ExtendUI32:
      case encoding::Opcode::F32ConvertSI32:
      case encoding::Opcode::F32ConvertUI32:
      case encoding::Opcode::F32ConvertSI64:
      case encoding::Opcode::F32ConvertUI64:
      case encoding::Opcode::F32DemoteF64:
      case encoding::Opcode::F32ReinterpretI32:
      case encoding::Opcode::F64ConvertSI32:
      case encoding::Opcode::F64ConvertUI32:
      case encoding::Opcode::F64ConvertSI64:
      case encoding::Opcode::F64ConvertUI64:
      case encoding::Opcode::F64PromoteF32:
      case encoding::Opcode::F64ReinterpretI64:
      case encoding::Opcode::I32ReinterpretF32:
      case encoding::Opcode::I64ReinterpretF64:
      case encoding::Opcode::I32Eqz:
      case encoding::Opcode::I64Eqz:
        hooks.OnOpcodeBare(opcode);
        break;

      // Type immediate.
      case encoding::Opcode::Block:
      case encoding::Opcode::Loop:
      case encoding::Opcode::If: {
        READ_OR_ERROR(type, ReadValType(data), "type index");
        hooks.OnOpcodeType(opcode, type);
        // Each of these instructions opens a new block which must be closed by
        // an `End`.
        ++ends_expected;
        break;
      }

      // Index immediate.
      case encoding::Opcode::Br:
      case encoding::Opcode::BrIf:
      case encoding::Opcode::GetGlobal:
      case encoding::Opcode::GetLocal:
      case encoding::Opcode::SetGlobal:
      case encoding::Opcode::SetLocal:
      case encoding::Opcode::Call:
      case encoding::Opcode::TeeLocal: {
        READ_OR_ERROR(index, ReadIndex(data), "index");
        hooks.OnOpcodeIndex(opcode, index);
        break;
      }

      // Index, reserved immediates.
      case encoding::Opcode::CallIndirect: {
        READ_OR_ERROR(index, ReadIndex(data), "index");
        READ_OR_ERROR(reserved, ReadU8(data), "reserved");
        hooks.OnOpcodeCallIndirect(opcode, index, reserved);
        break;
      }

      // Index* immediates.
      case encoding::Opcode::BrTable: {
        READ_OR_ERROR(targets, ReadVec<Index>(data, ReadIndex),
                      "br_table targets");
        READ_OR_ERROR(default_target, ReadIndex(data),
                      "br_table default target");
        hooks.OnOpcodeBrTable(opcode, targets, default_target);
        break;
      }

      // Memarg (alignment, offset) immediates.
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
      case encoding::Opcode::I32Load:
      case encoding::Opcode::I64Load:
      case encoding::Opcode::F32Load:
      case encoding::Opcode::F64Load:
      case encoding::Opcode::I32Store8:
      case encoding::Opcode::I32Store16:
      case encoding::Opcode::I64Store8:
      case encoding::Opcode::I64Store16:
      case encoding::Opcode::I64Store32:
      case encoding::Opcode::I32Store:
      case encoding::Opcode::I64Store:
      case encoding::Opcode::F32Store:
      case encoding::Opcode::F64Store: {
        READ_OR_ERROR(align_log2, ReadVarU32(data), "alignment");
        READ_OR_ERROR(offset, ReadVarU32(data), "offset");
        hooks.OnOpcodeMemarg(opcode, MemArg{align_log2, offset});
        break;
      }

      // Const immediates.
      case encoding::Opcode::I32Const: {
        READ_OR_ERROR(value, ReadVarS32(data), "i32 constant");
        hooks.OnOpcodeI32Const(opcode, value);
        break;
      }

      case encoding::Opcode::I64Const: {
        READ_OR_ERROR(value, ReadVarS64(data), "i64 constant");
        hooks.OnOpcodeI64Const(opcode, value);
        break;
      }

      case encoding::Opcode::F32Const: {
        READ_OR_ERROR(value, ReadF32(data), "f32 constant");
        hooks.OnOpcodeF32Const(opcode, value);
        break;
      }

      case encoding::Opcode::F64Const: {
        READ_OR_ERROR(value, ReadF64(data), "f64 constant");
        hooks.OnOpcodeF64Const(opcode, value);
        break;
      }

      default:
        hooks.OnError(absl::StrFormat("Unknown opcode 0x%02x", opcode));
        return absl::nullopt;
    }
  }

  const size_t len = data->data() - start;
  return Expr{SpanU8{start, len}};
}

template <typename Hooks = ModuleHooks>
bool ReadSection(SpanU8* data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(code, ReadVarU32(data), "section code");
  READ_OR_ERROR(len, ReadVarU32(data), "section length");

  if (len > data->size()) {
    hooks.OnError(absl::StrFormat("Section length is too long: %zd > %zd", len,
                                  data->size()));
    return false;
  }

  hooks.OnSection(code, data->subspan(0, len));
  data->remove_prefix(*opt_len);
  return true;
}

template <typename Hooks = ModuleHooks>
bool ReadModule(SpanU8 data, Hooks&& hooks = Hooks{}) {
  const SpanU8 kMagic{encoding::Magic};
  const SpanU8 kVersion{encoding::Version};

  auto opt_magic = ReadBytes(&data, 4);
  if (opt_magic != kMagic) {
    hooks.OnError(absl::StrFormat("Magic mismatch: expected %s, got %s",
                                  ToString(kMagic), ToString(*opt_magic)));
    return false;
  }

  auto opt_version = ReadBytes(&data, 4);
  if (opt_version != kVersion) {
    hooks.OnError(absl::StrFormat("Version mismatch: expected %s, got %s",
                                  ToString(kVersion), ToString(*opt_version)));
    return false;
  }

  while (!data.empty()) {
    if (!ReadSection(&data, hooks)) {
      return false;
    }
  }

  return true;
}

template <typename Hooks>
void ErrorUnlessAtSectionEnd(SpanU8 data, Hooks&& hooks) {
  if (data.size() != 0) {
    hooks.OnError("Expected end of section");
  }
}

template <typename Hooks = TypeSectionHooks>
bool ReadTypeSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "type count");
  hooks.OnTypeCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(form, ReadValType(&data), "type form");

    if (form != ValType::Func) {
      hooks.OnError(absl::StrFormat("Unknown type form: %d", form));
      return false;
    }

    FuncType func_type;
    READ_OR_ERROR(num_params, ReadIndex(&data), "param count");
    func_type.param_types.resize(num_params);
    for (Index j = 0; j < num_params; ++j) {
      READ_OR_ERROR(param_type, ReadValType(&data), "param valtype");
      func_type.param_types[j] = param_type;
    }

    READ_OR_ERROR(num_results, ReadIndex(&data), "result count");
    func_type.result_types.resize(num_results);
    for (Index j = 0; j < num_results; ++j) {
      READ_OR_ERROR(result_type, ReadValType(&data), "result valtype");
      func_type.result_types[j] = result_type;
    }

    hooks.OnFuncType(i, func_type);
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = ImportSectionHooks>
bool ReadImportSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "import count");
  hooks.OnImportCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(module, ReadStr(&data), "module name");
    READ_OR_ERROR(name, ReadStr(&data), "field name");
    READ_OR_ERROR(kind, ReadExternalKind(&data), "import kind");

    switch (kind) {
      case ExternalKind::Func: {
        READ_OR_ERROR(type_index, ReadIndex(&data), "func type index");
        hooks.OnFuncImport(i, FuncImport{module, name, type_index});
        break;
      }

      case ExternalKind::Table: {
        READ_OR_ERROR(table_type, ReadTableType(&data), "table type");
        hooks.OnTableImport(i, TableImport{module, name, table_type});
        break;
      }

      case ExternalKind::Memory: {
        READ_OR_ERROR(memory_type, ReadMemoryType(&data), "memory type");
        hooks.OnMemoryImport(i, MemoryImport{module, name, memory_type});
        break;
      }

      case ExternalKind::Global: {
        READ_OR_ERROR(global_type, ReadGlobalType(&data), "global type");
        hooks.OnGlobalImport(i, GlobalImport{module, name, global_type});
        break;
      }
    }
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = FunctionSectionHooks>
bool ReadFunctionSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "func count");
  hooks.OnFuncCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(type_index, ReadIndex(&data), "func type index");
    hooks.OnFunc(i, type_index);
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = TableSectionHooks>
bool ReadTableSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "table count");
  hooks.OnTableCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(table_type, ReadTableType(&data), "table type");
    hooks.OnTable(i, table_type);
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = MemorySectionHooks>
bool ReadMemorySection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "memory count");
  hooks.OnMemoryCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(memory_type, ReadMemoryType(&data), "memory type");
    hooks.OnMemory(i, memory_type);
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = GlobalSectionHooks>
bool ReadGlobalSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "global count");
  hooks.OnGlobalCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(global_type, ReadGlobalType(&data), "global type");
    READ_OR_ERROR(init_expr, ReadExpr(&data), "global initializer expression");
    hooks.OnGlobal(i, Global{global_type, init_expr});
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = ExportSectionHooks>
bool ReadExportSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "export count");
  hooks.OnExportCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(name, ReadStr(&data), "export name");
    READ_OR_ERROR(kind, ReadExternalKind(&data), "export kind");
    READ_OR_ERROR(index, ReadIndex(&data), "export index");
    hooks.OnExport(i, Export{name, kind, index});
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = StartSectionHooks>
bool ReadStartSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(func_index, ReadIndex(&data), "start function index");
  hooks.OnStart(func_index);
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = ElementSectionHooks>
bool ReadElementSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "element segment count");
  hooks.OnElementSegmentCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(table_index, ReadIndex(&data), "element segment table index");
    READ_OR_ERROR(offset, ReadExpr(&data), "element segment offset");
    READ_OR_ERROR(init, ReadVec<Index>(&data, ReadIndex),
                  "element segment initializers");
    hooks.OnElementSegment(i, ElementSegment{table_index, offset, init});
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = CodeSectionHooks>
bool ReadCodeSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "code count");
  hooks.OnCodeCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(len, ReadIndex(&data), "code length");

    if (len > data.size()) {
      hooks.OnError(absl::StrFormat("Code length is too long: %zd > %zd", len,
                                    data.size()));
      return false;
    }

    hooks.OnCode(i, data.subspan(0, len));
    data.remove_prefix(len);
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

optional<LocalDecl> ReadLocalDecl(SpanU8* data) {
  READ(count, ReadIndex(data));
  READ(type, ReadValType(data));
  return LocalDecl{count, type};
}

template <typename Hooks = CodeHooks>
bool ReadCode(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(local_decls, ReadVec<LocalDecl>(&data, ReadLocalDecl),
                "locals");
  READ_OR_ERROR(body, ReadExpr(&data), "body");
  hooks.OnCodeContents(local_decls, body);
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

template <typename Hooks = DataSectionHooks>
bool ReadDataSection(SpanU8 data, Hooks&& hooks = Hooks{}) {
  READ_OR_ERROR(count, ReadIndex(&data), "data segment count");
  hooks.OnDataSegmentCount(count);

  for (Index i = 0; i < count; ++i) {
    READ_OR_ERROR(table_index, ReadIndex(&data), "data segment table index");
    READ_OR_ERROR(offset, ReadExpr(&data), "data segment offset");
    READ_OR_ERROR(init, ReadVecU8(&data), "data segment initializer");
    hooks.OnDataSegment(i, DataSegment{table_index, offset, init});
  }
  ErrorUnlessAtSectionEnd(data, hooks);
  return true;
}

struct MyHooks {
  void OnError(const std::string& msg) { absl::PrintF("Error: %s\n", msg); }

  void OnSection(u32 code, SpanU8 data) {
    absl::PrintF("Section %d (%zd bytes)\n", code, data.size());
    switch (code) {
      case encoding::Section::Custom:   /*TODO*/ break;
      case encoding::Section::Type:     ReadTypeSection(data, *this); break;
      case encoding::Section::Import:   ReadImportSection(data, *this); break;
      case encoding::Section::Function: ReadFunctionSection(data, *this); break;
      case encoding::Section::Table:    ReadTableSection(data, *this); break;
      case encoding::Section::Memory:   ReadMemorySection(data, *this); break;
      case encoding::Section::Global:   ReadGlobalSection(data, *this); break;
      case encoding::Section::Export:   ReadExportSection(data, *this); break;
      case encoding::Section::Start:    ReadStartSection(data, *this); break;
      case encoding::Section::Element:  ReadElementSection(data, *this); break;
      case encoding::Section::Code:     ReadCodeSection(data, *this); break;
      case encoding::Section::Data:     ReadDataSection(data, *this); break;
      default: break;
    }
  }

  // Type section.
  void OnTypeCount(Index count) { absl::PrintF("Type count: %u\n", count); }
  void OnFuncType(Index type_index, const FuncType& func_type) {
    absl::PrintF("  Type[%u]: %s\n", type_index, ToString(func_type));
  }

  // Import section.
  void OnImportCount(Index count) { absl::PrintF("Import count: %u\n", count); }
  void OnFuncImport(Index import_index, const FuncImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnTableImport(Index import_index, const TableImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnMemoryImport(Index import_index, const MemoryImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }
  void OnGlobalImport(Index import_index, const GlobalImport& import) {
    absl::PrintF("  Import[%u]: %s\n", import_index, ToString(import));
  }

  // Function section.
  void OnFuncCount(Index count) { absl::PrintF("Func count: %u\n", count); }
  void OnFunc(Index func_index, Index type_index) {
    absl::PrintF("  Func[%u]: {type %u, ...}\n", func_index, type_index);
  }

  // Table section.
  void OnTableCount(Index count) { absl::PrintF("Table count: %u\n", count); }
  void OnTable(Index table_index, const TableType& table_type) {
    absl::PrintF("  Table[%u]: {type %s}\n", table_index, ToString(table_type));
  }

  // Memory section.
  void OnMemoryCount(Index count) { absl::PrintF("Memory count: %u\n", count); }
  void OnMemory(Index memory_index, const MemoryType& memory_type) {
    absl::PrintF("  Memory[%u]: {type %s}\n", memory_index,
                 ToString(memory_type));
  }

  // Global section.
  void OnGlobalCount(Index count) { absl::PrintF("Global count: %u\n", count); }
  void OnGlobal(Index global_index, const Global& global) {
    absl::PrintF("  Global[%u]: %s\n", global_index, ToString(global));
  }

  // Export section.
  void OnExportCount(Index count) { absl::PrintF("Export count: %u\n", count); }
  void OnExport(Index export_index, const Export& export_) {
    absl::PrintF("  Export[%u]: %s\n", export_index, ToString(export_));
  }

  // Start section.
  void OnStart(Index func_index) {
    absl::PrintF("Start: {func %u}\n", func_index);
  }

  // Element section.
  void OnElementSegmentCount(Index count) {
    absl::PrintF("Element segment count: %u\n", count);
  }
  void OnElementSegment(Index segment_index, const ElementSegment& segment) {
    absl::PrintF("  ElementSegment[%u]: %s\n", segment_index,
                 ToString(segment));
  }

  // Code section.
  void OnCodeCount(Index count) { absl::PrintF("Code count: %u\n", count); }
  void OnCode(Index code_index, SpanU8 code) {
    absl::PrintF("  Code[%u]: %zd bytes...\n", code_index, code.size());
    ReadCode(code, *this);
  }

  void OnCodeContents(const std::vector<LocalDecl>& locals, const Expr& body) {
    absl::PrintF("    Locals: %s\n", ToString(locals));
    absl::PrintF("    Body: %s\n", ToString(body));
  }

  // Data section.
  void OnDataSegmentCount(Index count) {
    absl::PrintF("Data segment count: %u\n", count);
  }
  void OnDataSegment(Index segment_index, const DataSegment& segment) {
    absl::PrintF("  DataSegment[%u]: %s\n", segment_index, ToString(segment));
  }
};

int main(int argc, char** argv) {
  argc--;
  argv++;
  if (argc == 0) {
    absl::PrintF("No files.\n");
    return 1;
  }

  std::string filename{argv[0]};
  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    absl::PrintF("Error reading file.\n");
    return 1;
  }

  if (!ReadModule(SpanU8{*optbuf}, MyHooks{})) {
    absl::PrintF("Unable to read module.\n");
  }

  return 0;
}

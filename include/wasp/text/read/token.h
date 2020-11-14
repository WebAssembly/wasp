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

#ifndef WASP_TEXT_READ_TOKEN_H_
#define WASP_TEXT_READ_TOKEN_H_

#include <string>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/buffer.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"

namespace wasp::text {

enum class TokenType {
#define WASP_V(Name) Name,
#include "wasp/text/token_type.inc"
#undef WASP_V
};

enum class Sign { None, Plus, Minus };
enum class LiteralKind { Normal, Nan, NanPayload, Infinity };
enum class Base { Decimal, Hex };
enum class HasUnderscores { No, Yes };

enum class SimdShape { I8X16, I16X8, I32X4, I64X2, F32X4, F64X2 };

struct LiteralInfo {
  static LiteralInfo HexNat(HasUnderscores);
  static LiteralInfo Nat(HasUnderscores);
  static LiteralInfo Number(Sign, HasUnderscores);
  static LiteralInfo HexNumber(Sign, HasUnderscores);
  static LiteralInfo Infinity(Sign);
  static LiteralInfo Nan(Sign);
  static LiteralInfo NanPayload(Sign, HasUnderscores);

  explicit LiteralInfo(LiteralKind);
  explicit LiteralInfo(Sign, LiteralKind, Base, HasUnderscores);

  Sign sign;
  LiteralKind kind;
  Base base;
  HasUnderscores has_underscores;
};

struct OpcodeInfo {
  OpcodeInfo(Opcode);
  OpcodeInfo(Opcode, Features);

  Opcode opcode;
  Features features;
};

struct Text {
  void AppendToBuffer(Buffer& buffer) const;
  auto ToString() const -> std::string;

  string_view text;
  u32 byte_size;
};

struct Token {
  using Immediate = variant<monostate,
                            OpcodeInfo,
                            NumericType,
                            ReferenceKind,
                            HeapKind,
                            PackedType,
                            LiteralInfo,
                            Text,
                            SimdShape>;

  Token();
  Token(Location, TokenType);
  Token(Location, TokenType, OpcodeInfo);
  Token(Location, TokenType, NumericType);
  Token(Location, TokenType, ReferenceKind);
  Token(Location, TokenType, HeapKind);
  Token(Location, TokenType, PackedType);
  Token(Location, TokenType, LiteralInfo);
  Token(Location, TokenType, Text);
  Token(Location, TokenType, Immediate);
  Token(Location, TokenType, SimdShape);

  SpanU8 span_u8() const;
  string_view as_string_view() const;

  bool has_opcode() const;
  bool has_numeric_type() const;
  bool has_reference_kind() const;
  bool has_heap_kind() const;
  bool has_packed_type() const;
  bool has_literal_info() const;
  bool has_text() const;
  bool has_simd_shape() const;

  At<Opcode> opcode() const;
  Features opcode_features() const;
  At<NumericType> numeric_type() const;
  At<ReferenceKind> reference_kind() const;
  At<HeapKind> heap_kind() const;
  At<PackedType> packed_type() const;
  LiteralInfo literal_info() const;
  Text text() const;
  SimdShape simd_shape() const;

  Location loc;
  TokenType type;
  Immediate immediate;
};

}  // namespace wasp::text

#include "wasp/text/read/token-inl.h"

#endif // WASP_TEXT_READ_TOKEN_H_

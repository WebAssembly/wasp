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

#include "wasp/base/at.h"
#include "wasp/base/features.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/base/variant.h"
#include "wasp/base/wasm_types.h"

namespace wasp {
namespace text {

enum class TokenType {
#define WASP_V(Name) Name,
#include "wasp/text/token_type.def"
#undef WASP_V
};

enum class Sign { None, Plus, Minus };
enum class LiteralKind { Normal, Nan, NanPayload, Infinity };
enum class Base { Decimal, Hex };
enum class HasUnderscores { No, Yes };

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
  auto ToString() const -> std::string;

  string_view text;
  u32 byte_size;
};

struct Token {
  using Immediate = variant<monostate,
                            OpcodeInfo,
                            ValueType,
                            ReferenceType,
                            LiteralInfo,
                            Text>;

  Token();
  Token(Location, TokenType);
  Token(Location, TokenType, OpcodeInfo);
  Token(Location, TokenType, ValueType);
  Token(Location, TokenType, ReferenceType);
  Token(Location, TokenType, LiteralInfo);
  Token(Location, TokenType, Text);
  Token(Location, TokenType, Immediate);

  SpanU8 span_u8() const;
  string_view as_string_view() const;

  bool has_opcode() const;
  bool has_value_type() const;
  bool has_reference_type() const;
  bool has_literal_info() const;
  bool has_text() const;

  At<Opcode> opcode() const;
  Features opcode_features() const;
  At<ValueType> value_type() const;
  At<ReferenceType> reference_type() const;
  LiteralInfo literal_info() const;
  Text text() const;

  Location loc;
  TokenType type;
  Immediate immediate;
};

}  // namespace text
}  // namespace wasp

#include "wasp/text/read/token-inl.h"

#endif // WASP_TEXT_READ_TOKEN_H_

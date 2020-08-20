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

namespace wasp::text {

// static
inline LiteralInfo LiteralInfo::HexNat(HasUnderscores underscores) {
  return LiteralInfo{Sign::None, LiteralKind::Normal, Base::Hex, underscores};
}

// static
inline LiteralInfo LiteralInfo::Nat(HasUnderscores underscores) {
  return LiteralInfo{Sign::None, LiteralKind::Normal, Base::Decimal,
                     underscores};
}

// static
inline LiteralInfo LiteralInfo::Number(Sign sign, HasUnderscores underscores) {
  return LiteralInfo{sign, LiteralKind::Normal, Base::Decimal, underscores};
}

// static
inline LiteralInfo LiteralInfo::HexNumber(Sign sign,
                                          HasUnderscores underscores) {
  return LiteralInfo{sign, LiteralKind::Normal, Base::Hex, underscores};
}

// static
inline LiteralInfo LiteralInfo::Infinity(Sign sign) {
  return LiteralInfo{sign, LiteralKind::Infinity, Base::Decimal,
                     HasUnderscores::No};
}

// static
inline LiteralInfo LiteralInfo::Nan(Sign sign) {
  return LiteralInfo{sign, LiteralKind::Nan, Base::Decimal, HasUnderscores::No};
}

// static
inline LiteralInfo LiteralInfo::NanPayload(Sign sign,
                                           HasUnderscores underscores) {
  return LiteralInfo{sign, LiteralKind::NanPayload, Base::Decimal, underscores};
}

inline LiteralInfo::LiteralInfo(LiteralKind kind)
    : sign{Sign::None},
      kind{kind},
      base{Base::Decimal},
      has_underscores{HasUnderscores::No} {}

inline LiteralInfo::LiteralInfo(Sign sign,
                                LiteralKind kind,
                                Base base,
                                HasUnderscores has_underscores)
    : sign{sign}, kind{kind}, base{base}, has_underscores{has_underscores} {}

inline OpcodeInfo::OpcodeInfo(Opcode opcode) : opcode{opcode}, features{0} {}

inline OpcodeInfo::OpcodeInfo(Opcode opcode, Features features)
    : opcode{opcode}, features{features} {}

inline string_view Token::as_string_view() const {
  return ToStringView(loc);
}

inline SpanU8 Token::span_u8() const {
  return loc;
}

inline bool Token::has_opcode() const {
  return immediate.index() == 1;
}

inline bool Token::has_numeric_type() const {
  return immediate.index() == 2;
}

inline bool Token::has_reference_kind() const {
  return immediate.index() == 3;
}

inline bool Token::has_heap_kind() const {
  return immediate.index() == 4;
}

inline bool Token::has_literal_info() const {
  return immediate.index() == 5;
}

inline bool Token::has_text() const {
  return immediate.index() == 6;
}

inline At<Opcode> Token::opcode() const {
  return MakeAt(loc, get<OpcodeInfo>(immediate).opcode);
}

inline Features Token::opcode_features() const {
  return get<OpcodeInfo>(immediate).features;
}

inline At<NumericType> Token::numeric_type() const {
  return MakeAt(loc, get<NumericType>(immediate));
}

inline At<ReferenceKind> Token::reference_kind() const {
  return MakeAt(loc, get<ReferenceKind>(immediate));
}

inline At<HeapKind> Token::heap_kind() const {
  return MakeAt(loc, get<HeapKind>(immediate));
}

inline LiteralInfo Token::literal_info() const {
  return get<LiteralInfo>(immediate);
}

inline Text Token::text() const {
  return get<Text>(immediate);
}

}  // namespace wasp::text

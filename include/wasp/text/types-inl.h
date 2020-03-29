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

namespace wasp {
namespace text {

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
inline LiteralInfo LiteralInfo::HexNumber(Sign sign, HasUnderscores underscores) {
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

inline bool operator==(const LiteralInfo& lhs, const LiteralInfo& rhs) {
  return lhs.sign == rhs.sign && lhs.kind == rhs.kind && lhs.base == rhs.base &&
         lhs.has_underscores == rhs.has_underscores;
}

inline bool operator!=(const LiteralInfo& lhs, const LiteralInfo& rhs) {
  return !(lhs == rhs);
}

inline bool operator==(const Token& lhs, const Token& rhs) {
  return lhs.loc == rhs.loc && lhs.type == rhs.type &&
         lhs.immediate == rhs.immediate;
}

inline bool operator!=(const Token& lhs, const Token& rhs) {
  return !(lhs == rhs);
}

}  // namespace text
}  // namespace wasp

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

#include "wasp/text/formatters.h"

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace wasp {

WASP_DEFINE_VARIANT_NAME(text::OpcodeInfo, "opcode_info")
WASP_DEFINE_VARIANT_NAME(text::LiteralInfo, "literal_info")
WASP_DEFINE_VARIANT_NAME(text::Text, "text")

}  // namespace wasp

namespace fmt {

WASP_TEXT_STRUCTS(WASP_FORMATTER_VARGS)

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::TokenType>::format(
    const ::wasp::text::TokenType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(Name)                  \
  case ::wasp::text::TokenType::Name: \
    result = #Name;                   \
    break;
#include "wasp/text/token_type.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Sign>::format(
    const ::wasp::text::Sign& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::Sign::None:  result = "None"; break;
    case ::wasp::text::Sign::Plus:  result = "Plus"; break;
    case ::wasp::text::Sign::Minus: result = "Minus"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::LiteralKind>::format(
    const ::wasp::text::LiteralKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::LiteralKind::Normal:     result = "Normal"; break;
    case ::wasp::text::LiteralKind::Nan:        result = "Nan"; break;
    case ::wasp::text::LiteralKind::NanPayload: result = "NanPayload"; break;
    case ::wasp::text::LiteralKind::Infinity:   result = "Infinity"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::Base>::format(
    const ::wasp::text::Base& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::Base::Decimal: result = "Decimal"; break;
    case ::wasp::text::Base::Hex:     result = "Hex"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::text::HasUnderscores>::format(
    const ::wasp::text::HasUnderscores& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::text::HasUnderscores::No:  result = "No"; break;
    case ::wasp::text::HasUnderscores::Yes: result = "Yes"; break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

}  // namespace fmt

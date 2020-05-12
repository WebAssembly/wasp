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

#include "wasp/text/read/token.h"

namespace wasp {
namespace text {

void Text::ToBuffer(Buffer& buffer) const {
  static const char kHexDigit[256] = {
      /*00*/ 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
      /*10*/ 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
      /*20*/ 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
      /*30*/ 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,
      /*40*/ 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      /*50*/ 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
      /*60*/ 0, 10, 11, 12, 13, 14, 15,
      // The rest are zero.
  };

  buffer.reserve(buffer.size() + byte_size);

  // Remove surrounding quotes.
  assert(text.size() >= 2 && text[0] == '"' && text[text.size() - 1] == '"');
  string_view input = text.substr(1, text.size() - 2);

  // Unescape characters.
  for (const char *p = input.begin(), *end = input.end(); p < end; ++p) {
    char c = *p;
    if (c == '\\') {
      c = *++p;
      switch (c) {
        case 't': buffer.push_back('\t'); break;
        case 'n': buffer.push_back('\n'); break;
        case 'r': buffer.push_back('\r'); break;

        case '"':
        case '\'':
        case '\\':
          buffer.push_back(c);
          break;

        default:
          // Must be a "\xx" hexadecimal sequence.
          buffer.push_back((kHexDigit[int(c)] << 4) | kHexDigit[int(*++p)]);
          break;
      }
    } else {
      buffer.push_back(c);
    }
  }
}

auto Text::ToString() const -> std::string {
  Buffer buffer;
  ToBuffer(buffer);
  return std::string(reinterpret_cast<const char*>(buffer.data()),
                     buffer.size());
}

Token::Token() : loc{}, type{TokenType::Eof}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type)
    : loc{loc}, type{type}, immediate{monostate{}} {}

Token::Token(Location loc, TokenType type, OpcodeInfo info)
    : loc{loc}, type{type}, immediate{info} {}

Token::Token(Location loc, TokenType type, ValueType valtype)
    : loc{loc}, type{type}, immediate{valtype} {}

Token::Token(Location loc, TokenType type, ReferenceType reftype)
    : loc{loc}, type{type}, immediate{reftype} {}

Token::Token(Location loc, TokenType type, LiteralInfo info)
    : loc{loc}, type{type}, immediate{info} {}

Token::Token(Location loc, TokenType type, Text text)
    : loc{loc}, type{type}, immediate{text} {}

Token::Token(Location loc, TokenType type, Immediate immediate)
    : loc{loc}, type{type}, immediate{immediate} {}

}  // namespace text
}  // namespace wasp

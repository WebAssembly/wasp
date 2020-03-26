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

inline Tokenizer::Tokenizer(SpanU8 data) : data_{data} {}

inline bool Tokenizer::empty() const {
  return count_ == 0;
}

inline auto Tokenizer::count() const -> int {
  return count_;
}

inline auto Tokenizer::Previous() const -> Token {
  return previous_token_;
}

inline auto Tokenizer::Read() -> Token {
  if (count_ == 0) {
    previous_token_ = LexNoWhitespace(&data_);
  } else {
    previous_token_ = tokens_[current_];
    current_ = !current_;
    count_--;
  }
  return previous_token_;
}

inline auto Tokenizer::Peek(unsigned at) -> Token {
  if (count_ == 0) {
    tokens_[current_] = LexNoWhitespace(&data_);
    count_++;
  }
  if (at == 0) {
    return tokens_[current_];
  } else {
    assert(at == 1);
    if (count_ == 1) {
      tokens_[!current_] = LexNoWhitespace(&data_);
      count_++;
    }
    return tokens_[!current_];
  }
}

inline auto Tokenizer::Match(TokenType token_type) -> optional<Token> {
  if (Peek().type != token_type) {
    return nullopt;
  }
  return Read();
}

inline auto Tokenizer::MatchLpar(TokenType token_type) -> optional<Token> {
  if (!(Peek(0).type == TokenType::Lpar && Peek(1).type == token_type)) {
    return nullopt;
  }
  Read();
  return Read();
}

}  // namespace text
}  // namespace wasp

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

#ifndef WASP_TEXT_READ_TOKENIZER_H_
#define WASP_TEXT_READ_TOKENIZER_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/types.h"
#include "wasp/text/read/token.h"

namespace wasp::text {

class Tokenizer {
 public:
  explicit Tokenizer(SpanU8 data);

  bool empty() const;
  auto count() const -> int;

  auto Previous() const -> Token;
  auto Read() -> Token;
  auto Peek(unsigned at = 0) -> Token;

  auto Match(TokenType) -> optional<Token>;
  auto MatchLpar(TokenType) -> optional<Token>;

 private:
  SpanU8 data_;
  int current_ = 0;
  int count_ = 0;
  Token tokens_[2];  // Two tokens of lookahead.
  Token previous_token_;
};

}  // namespace wasp::text

#include "wasp/text/read/tokenizer-inl.h"

#endif // WASP_TEXT_READ_TOKENIZER_H_

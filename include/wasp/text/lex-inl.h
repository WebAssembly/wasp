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

inline auto Tokenizer::Read() -> Token {
  if (count_ == 0) {
    return LexNoWhitespace(&data_);
  } else {
    auto token = tokens_[current_];
    current_ = !current_;
    count_--;
    return token;
  }
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

}  // namespace text
}  // namespace wasp

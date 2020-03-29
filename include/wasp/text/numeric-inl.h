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

#include <algorithm>
#include <cassert>
#include <charconv>

namespace wasp {
namespace text {

namespace {

bool HasUnderscores(SpanU8 span) {
  return std::find(span.begin(), span.end(), '_') != span.end();
}

void RemoveUnderscores(SpanU8 span, std::vector<u8>& out) {
  std::copy_if(span.begin(), span.end(), std::back_inserter(out),
               [](u8 c) { return c != '_'; });
}

struct FixNum {
  explicit FixNum(SpanU8 orig) : span{orig} {}

  using cchar = const char;

  size_t size() const { return span.size(); }
  cchar* begin() const { return reinterpret_cast<cchar*>(span.begin()); }
  cchar* end() const { return reinterpret_cast<cchar*>(span.end()); }

  void FixUnderscores() {
    if (HasUnderscores(span)) {
      RemoveUnderscores(span, vec);
      span = vec;
    }
  }

  SpanU8 span;
  std::vector<u8> vec;
};

struct FixNat : FixNum {
  explicit FixNat(SpanU8 orig) : FixNum{orig} { FixUnderscores(); }
};

struct FixHexNat : FixNum {
  explicit FixHexNat(SpanU8 orig) : FixNum{orig} {
    FixUnderscores();
    remove_prefix(&span, 2);  // Skip "0x".
  }
};

struct FixInt : FixNum {
  explicit FixInt(SpanU8 orig) : FixNum(orig) {
    FixUnderscores();
    assert(orig.size() >= 1);
    if (orig[0] == '+') {
      remove_prefix(&span, 1);  // Skip "+", not allowed by std::from_chars.
    }
  }
};

struct FixHexInt : FixNum {
  explicit FixHexInt(SpanU8 orig) : FixNum(orig) {
    assert(orig.size() >= 1);
    switch (orig[0]) {
      case '+':
        FixUnderscores();
        remove_prefix(&span, 1);  // Skip "+", not allowed by std::from_chars.
        break;

      case '-':
        // Have to remove "-0x" and replace with "-", but need to keep the "-"
        // at the beginning.
        vec.push_back('-');
        RemoveUnderscores(orig.subspan(2), vec);
        span = vec;
        break;

      default:
        FixUnderscores();
        break;
    }
  }
};

}  // namespace

template <typename T>
auto StrToNat(LiteralKind kind, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (kind == LiteralKind::Int) {
      FixNat str{orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(kind == LiteralKind::HexInt);
      FixHexNat str{orig};
      return std::from_chars(str.begin(), str.end(), value, 16);
    }
  })();

  if (result.ec != std::errc{}) {
    return {};
  }
  return MakeAt(orig, value);
}

template <typename T>
auto StrToInt(LiteralKind kind, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (kind == LiteralKind::Int) {
      FixInt str{orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(kind == LiteralKind::HexInt);
      FixHexInt str{orig};
      return std::from_chars(str.begin(), str.end(), value, 16);
    }
  })();

  if (result.ec != std::errc{}) {
    return {};
  }
  return MakeAt(orig, value);
}

}  // namespace text
}  // namespace wasp

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

void RemoveUnderscores(SpanU8 span, std::vector<u8>& out) {
  std::copy_if(span.begin(), span.end(), std::back_inserter(out),
               [](u8 c) { return c != '_'; });
}

struct FixNum {
  explicit FixNum(LiteralInfo info, SpanU8 orig) : info{info}, span{orig} {}

  using cchar = const char;

  size_t size() const { return span.size(); }
  cchar* begin() const { return reinterpret_cast<cchar*>(span.begin()); }
  cchar* end() const { return reinterpret_cast<cchar*>(span.end()); }

  void FixUnderscores() {
    if (info.has_underscores == HasUnderscores::Yes) {
      RemoveUnderscores(span, vec);
      span = vec;
    }
  }

  LiteralInfo info;
  SpanU8 span;
  std::vector<u8> vec;
};

struct FixNat : FixNum {
  explicit FixNat(LiteralInfo info, SpanU8 orig) : FixNum{info, orig} {
    FixUnderscores();
  }
};

struct FixHexNat : FixNum {
  explicit FixHexNat(LiteralInfo info, SpanU8 orig) : FixNum{info, orig} {
    FixUnderscores();
    remove_prefix(&span, 2);  // Skip "0x".
  }
};

struct FixInt : FixNum {
  explicit FixInt(LiteralInfo info, SpanU8 orig) : FixNum(info, orig) {
    FixUnderscores();
    assert(orig.size() >= 1);
    if (info.sign == Sign::Plus) {
      remove_prefix(&span, 1);  // Skip "+", not allowed by std::from_chars.
    }
  }
};

struct FixHexInt : FixNum {
  explicit FixHexInt(LiteralInfo info, SpanU8 orig) : FixNum(info, orig) {
    assert(orig.size() >= 1);
    switch (info.sign) {
      case Sign::Plus:
        FixUnderscores();
        remove_prefix(&span, 3);  // Skip "+0x", not allowed by std::from_chars.
        break;

      case Sign::Minus:
        // Have to remove "-0x" and replace with "-", but need to keep the "-"
        // at the beginning.
        vec.push_back('-');
        RemoveUnderscores(orig.subspan(3), vec);
        span = vec;
        break;

      default:
        FixUnderscores();
        remove_prefix(&span, 2);  // Skip "0x".
        break;
    }
  }
};

}  // namespace

template <typename T>
auto StrToNat(LiteralInfo info, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (info.base == Base::Decimal) {
      FixNat str{info, orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(info.base == Base::Hex);
      FixHexNat str{info, orig};
      return std::from_chars(str.begin(), str.end(), value, 16);
    }
  })();

  if (result.ec != std::errc{}) {
    return {};
  }
  return MakeAt(orig, value);
}

template <typename T>
auto StrToInt(LiteralInfo info, SpanU8 orig) -> OptAt<T> {
  T value;
  std::from_chars_result result = ([&]() {
    if (info.base == Base::Decimal) {
      FixInt str{info, orig};
      return std::from_chars(str.begin(), str.end(), value);
    } else {
      assert(info.base == Base::Hex);
      FixHexInt str{info, orig};
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

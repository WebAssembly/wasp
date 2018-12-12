//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BASE_FORMATTERS_H_
#define WASP_BASE_FORMATTERS_H_

#include <vector>

#include "src/base/format.h"
#include "src/base/string_view.h"
#include "src/base/types.h"

namespace fmt {

template <>
struct formatter<::wasp::SpanU8> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) { return ctx.begin(); }

  template <typename Ctx>
  typename Ctx::iterator format(const ::wasp::SpanU8& self, Ctx& ctx) {
    auto it = ctx.begin();
    it = format_to(it, "\"");
    for (auto x: self) {
      it = format_to(it, "\\{:02x}", x);
    }
    return format_to(it, "\"");
  }
};

template <typename T>
struct formatter<std::vector<T>> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) { return ctx.begin(); }

  template <typename Ctx>
  typename Ctx::iterator format(const std::vector<T>& self, Ctx& ctx) {
    bool first = true;
    auto it = ctx.begin();
    it = format_to(it, "[");
    for (const auto& x: self) {
      if (!first) {
        it = format_to(it, " ");
      }
      it = format_to(it, "{}", x);
      first = false;
    }
    return format_to(it, "]");
  }
};

template <typename CharT, typename Traits>
struct formatter<::wasp::basic_string_view<CharT, Traits>>
    : formatter<string_view> {
  template <typename Ctx>
  typename Ctx::iterator parse(Ctx& ctx) { return ctx.begin(); }

  template <typename Ctx>
  typename Ctx::iterator format(::wasp::basic_string_view<CharT, Traits> self,
                                Ctx& ctx) {
    return formatter<string_view>::format(string_view{self.data(), self.size()},
                                          ctx);
  }
};

}  // namespace fmt

#endif // WASP_BASE_FORMATTERS_H_

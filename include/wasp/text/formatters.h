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

#ifndef WASP_TEXT_FORMATTERS_H_
#define WASP_TEXT_FORMATTERS_H_

#include "wasp/base/format.h"
#include "wasp/text/types.h"

namespace fmt {

#define WASP_DEFINE_FORMATTER(Name)                                 \
  template <>                                                       \
  struct formatter<::wasp::text::Name> : formatter<string_view> {   \
    template <typename Ctx>                                         \
    typename Ctx::iterator format(const ::wasp::text::Name&, Ctx&); \
  } /* No semicolon. */

WASP_DEFINE_FORMATTER(TokenType);
WASP_DEFINE_FORMATTER(Sign);
WASP_DEFINE_FORMATTER(LiteralKind);
WASP_DEFINE_FORMATTER(Base);
WASP_DEFINE_FORMATTER(HasUnderscores);
WASP_DEFINE_FORMATTER(LiteralInfo);
WASP_DEFINE_FORMATTER(Token);

#undef WASP_DEFINE_FORMATTER

}  // namespace fmt

#include "wasp/text/formatters-inl.h"

#endif  // WASP_TEXT_FORMATTERS_H_

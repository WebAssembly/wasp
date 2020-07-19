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

#include "wasp/valid/formatters.h"

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"
#include "wasp/binary/formatters.h"

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::valid::Any>::format(
    const ::wasp::valid::Any& self,
    Ctx& ctx) {
  return formatter<string_view>::format("any", ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::valid::StackType>::format(
    const ::wasp::valid::StackType& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.is_value_type()) {
    format_to(buf, "{}", self.value_type());
  } else {
    assert(self.is_any());
    format_to(buf, "any");
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

}  // namespace fmt

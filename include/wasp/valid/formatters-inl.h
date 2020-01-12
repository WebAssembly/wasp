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

namespace fmt {

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::valid::StackType>::format(
    const ::wasp::valid::StackType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...)    \
  case ::wasp::valid::StackType::Name: \
    result = str;                      \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/valid/stack_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

}  // namespace fmt

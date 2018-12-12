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

#ifndef WASP_BASE_VARIANT_H_
#define WASP_BASE_VARIANT_H_

#include "nonstd/variant.hpp"

namespace wasp {

using nonstd::bad_variant_access;
using nonstd::monostate;
using nonstd::variant;
using nonstd::variant_alternative;
using nonstd::variant_alternative_t;
using nonstd::variant_size;

using nonstd::get;
using nonstd::get_if;
using nonstd::holds_alternative;
using nonstd::visit;
using nonstd::operator==;
using nonstd::operator!=;
using nonstd::operator<;
using nonstd::operator<=;
using nonstd::operator>;
using nonstd::operator>=;
using nonstd::swap;

constexpr auto variant_npos = nonstd::variant_npos;

}  // namespace wasp

#endif  // WASP_BASE_VARIANT_H_

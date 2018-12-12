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

#ifndef WASP_BASE_FORMAT_H_
#define WASP_BASE_FORMAT_H_

#include "fmt/format.h"

namespace wasp {

// see http://fmtlib.net/latest/api.html.

using fmt::format;
using fmt::print;
using fmt::vprint;
using fmt::make_format_args;
using fmt::format_arg_store;
using fmt::basic_format_args;
using fmt::format_args;
using fmt::format_to;
using fmt::formatter;
using fmt::to_string;

}  // namespace wasp

#endif  // WASP_BASE_FORMAT_H_


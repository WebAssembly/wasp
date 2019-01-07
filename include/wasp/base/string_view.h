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

#ifndef WASP_BASE_STRING_VIEW_H_
#define WASP_BASE_STRING_VIEW_H_

#include "nonstd/string_view.hpp"

namespace wasp {

using nonstd::string_view;

using nonstd::string_view;
using nonstd::wstring_view;
using nonstd::u16string_view;
using nonstd::u32string_view;
using nonstd::basic_string_view;

using nonstd::operator==;
using nonstd::operator!=;
using nonstd::operator<;
using nonstd::operator<=;
using nonstd::operator>;
using nonstd::operator>=;

using nonstd::operator<<;

}  // namespace wasp

#endif  // WASP_BASE_STRING_VIEW_H_

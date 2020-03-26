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

#ifndef WASP_BASE_OPTIONAL_H_
#define WASP_BASE_OPTIONAL_H_

#include <optional>

namespace wasp {

using std::bad_optional_access;
using std::optional;

using std::nullopt;
using std::nullopt_t;

using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator<=;
using std::operator>;
using std::operator>=;
using std::make_optional;
using std::swap;

}  // namespace wasp

#endif  // WASP_BASE_OPTIONAL_H_

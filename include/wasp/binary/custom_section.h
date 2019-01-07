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

#ifndef WASP_BINARY_CUSTOM_SECTION_H_
#define WASP_BINARY_CUSTOM_SECTION_H_

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

namespace wasp {
namespace binary {

struct CustomSection {
  string_view name;
  SpanU8 data;
};

bool operator==(const CustomSection&, const CustomSection&);
bool operator!=(const CustomSection&, const CustomSection&);

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_CUSTOM_SECTION_H_

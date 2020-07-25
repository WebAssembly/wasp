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

#include "wasp/text/read/context.h"

namespace wasp {
namespace text {

Context::Context(Errors& errors) : errors{errors} {}

Context::Context(const Features& features, Errors& errors)
    : features{features}, errors{errors} {}

void Context::BeginModule() {
  seen_non_import = false;
  seen_start = false;
}

}  // namespace text
}  // namespace wasp

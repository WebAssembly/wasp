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

#include "wasp/binary/read/read_ctx.h"

namespace wasp::binary {

ReadCtx::ReadCtx(Errors& errors) : errors(errors) {}

ReadCtx::ReadCtx(const Features& features, Errors& errors)
    : features(features), errors(errors) {}

void ReadCtx::Reset() {
  last_section_id.reset();
  defined_function_count = 0;
  declared_data_count.reset();
  code_count = 0;
  data_count = 0;
}

}  // namespace wasp::binary

//
// Copyright 2019 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_READ_CONTEXT_H_
#define WASP_BINARY_READ_CONTEXT_H_

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {

class Errors;

struct Context {
  explicit Context(Errors&);
  explicit Context(const Features&, Errors&);

  void Reset();

  Features features;
  Errors& errors;

  optional<SectionId> last_section_id;
  Index defined_function_count = 0;
  optional<Index> declared_data_count;
  Index code_count = 0;
  Index data_count = 0;
};

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_CONTEXT_H_

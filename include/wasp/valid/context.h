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

#ifndef WASP_VALID_CONTEXT_H_
#define WASP_VALID_CONTEXT_H_

#include <vector>

#include "wasp/base/types.h"
#include "wasp/binary/global_type.h"
#include "wasp/binary/memory_type.h"
#include "wasp/binary/table_type.h"
#include "wasp/binary/type_entry.h"

namespace wasp {
namespace valid {

struct Context {
  std::vector<binary::TypeEntry> types;
  std::vector<binary::Function> functions;
  std::vector<binary::TableType> tables;
  std::vector<binary::MemoryType> memories;
  std::vector<binary::GlobalType> globals;
  Index imported_global_count = 0;
};

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_CONTEXT_H_

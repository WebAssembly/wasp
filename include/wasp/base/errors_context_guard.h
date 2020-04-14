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

#ifndef WASP_BASE_ERRORS_CONTEXT_GUARD_H_
#define WASP_BASE_ERRORS_CONTEXT_GUARD_H_

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/errors.h"

namespace wasp {

/// ---
class ErrorsContextGuard {
 public:
  explicit ErrorsContextGuard(Errors& errors, Location loc, string_view desc)
      : errors_{errors} {
    errors.PushContext(loc, desc);
  }
  ~ErrorsContextGuard() { PopContext(); }

  void PopContext() {
    if (!popped_context_) {
      errors_.PopContext();
      popped_context_ = true;
    }
  }

 private:
  Errors& errors_;
  bool popped_context_ = false;
};

}  // namespace wasp

#endif // WASP_BASE_ERRORS_CONTEXT_GUARD_H_

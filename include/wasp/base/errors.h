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

#ifndef WASP_BASE_ERRORS_H_
#define WASP_BASE_ERRORS_H_

#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

namespace wasp {

class Errors {
 public:
  virtual ~Errors() {}
  void PushContext(Location loc, string_view desc);
  void PopContext();
  void OnError(Location loc, string_view message);

  virtual bool HasError() const = 0;

 protected:
  virtual void HandlePushContext(Location loc, string_view desc) = 0;
  virtual void HandlePopContext() = 0;
  virtual void HandleOnError(Location loc, string_view message) = 0;
};

}  // namespace wasp

#include "wasp/base/errors-inl.h"

#endif // WASP_BASE_ERRORS_H_

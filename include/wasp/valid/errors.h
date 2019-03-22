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

#ifndef WASP_VALID_ERRORS_H_
#define WASP_VALID_ERRORS_H_

#include "wasp/base/string_view.h"

namespace wasp {
namespace valid {

class Errors {
 public:
  void PushContext(string_view desc);
  void PopContext();
  void OnError(string_view message);

 protected:
  virtual void HandlePushContext(string_view desc) = 0;
  virtual void HandlePopContext() = 0;
  virtual void HandleOnError(string_view message) = 0;
};

}  // namespace valid
}  // namespace wasp

#include "wasp/valid/errors-inl.h"

#endif // WASP_VALID_ERRORS_H_

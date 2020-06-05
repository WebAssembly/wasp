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

#ifndef SRC_TOOLS_BINARY_ERRORS_H_
#define SRC_TOOLS_BINARY_ERRORS_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "wasp/base/error.h"
#include "wasp/base/errors.h"
#include "wasp/base/span.h"
#include "wasp/base/types.h"

namespace wasp {
namespace tools {

class BinaryErrors : public Errors {
 public:
  // TODO: remove this constructor and always require a filename.
  explicit BinaryErrors(SpanU8 data);
  explicit BinaryErrors(string_view filename, SpanU8 data);

  bool has_error() const { return !errors.empty(); }
  void PrintTo(std::ostream&);

 protected:
  void HandlePushContext(Location loc, string_view desc) override;
  void HandlePopContext() override;
  void HandleOnError(Location loc, string_view message) override;

  auto ErrorToString(const Error&) const -> std::string;

  std::string filename;
  SpanU8 data;
  std::vector<Error> errors;
};

}  // namespace tools
}  // namespace wasp

#endif  // SRC_TOOLS_BINARY_ERRORS_H_

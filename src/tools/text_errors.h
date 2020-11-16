//
// Copyright 2020 WebAssembly Community Group participants
//
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

#ifndef WASP_TOOLS_TEXT_ERRORS_H_
#define WASP_TOOLS_TEXT_ERRORS_H_

#include <vector>

#include "wasp/base/error.h"
#include "wasp/base/errors.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

namespace wasp::tools {

class TextErrors : public Errors {
 public:
  using Offset = size_t;
  using Line = size_t;
  using Column = size_t;

  explicit TextErrors(string_view filename, SpanU8 data);

  void PrintTo(std::ostream&) const;
  bool HasError() const override;

 protected:
  void HandlePushContext(SpanU8 pos, string_view desc) override;
  void HandlePopContext() override;
  void HandleOnError(Location, string_view message) override;

  void CalculateLineNumbers() const;
  auto GetLineRange(Line) const -> std::pair<Offset, Offset>;
  auto GetLineColumn(Offset) const -> std::pair<Line, Column>;
  auto ErrorToString(const Error&) const -> std::string;

  std::string filename;
  SpanU8 data;
  std::vector<Error> errors;
  mutable std::vector<Offset> line_offsets;
};

}  // namespace wasp::tools

#endif  // WASP_TOOLS_TEXT_ERRORS_H_

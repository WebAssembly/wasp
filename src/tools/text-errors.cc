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

#include <algorithm>

#include "src/tools/text-errors.h"

#include "wasp/base/enumerate.h"
#include "wasp/base/format.h"

namespace wasp {
namespace tools {

TextErrors::TextErrors(string_view filename, SpanU8 data)
    : filename{filename}, data{data} {}

void TextErrors::Print() {
  if (has_error()) {
    CalculateLineNumbers();
    for (const auto& error : errors) {
      auto offset = error.loc.data() - data.data();
      auto [line, column] = GetLineColumn(offset);
      print(stderr, "{}:{}:{}: {}\n", filename, line, column, error.message);
    }
  }
}

bool TextErrors::has_error() const {
  return !errors.empty();
}

void TextErrors::HandlePushContext(SpanU8 pos, string_view desc) {}

void TextErrors::HandlePopContext() {}

void TextErrors::HandleOnError(Location loc, string_view message) {
  errors.push_back(Error{loc, std::string{message}});
}

void TextErrors::CalculateLineNumbers() {
  if (!line_offsets.empty()) {
    return;
  }

  line_offsets.push_back(0);
  for (auto [offset, c] : enumerate(data)) {
    if (c == '\n') {
      line_offsets.push_back(offset + 1);
    }
  }
}

auto TextErrors::GetLineColumn(Offset offset) -> std::pair<Line, Column> {
  auto iter =
      std::lower_bound(line_offsets.begin(), line_offsets.end(), offset);
  Line line;
  Column column;
  if (iter == line_offsets.begin()) {
    line = 1;
    column = offset + 1;
  } else {
    --iter;
    line = (iter - line_offsets.begin()) + 1;
    column = (offset - *iter) + 1;
  }

  return std::pair(line, column);
}

}  // namespace tools
}  // namespace wasp

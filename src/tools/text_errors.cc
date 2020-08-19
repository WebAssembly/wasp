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

#include "src/tools/text_errors.h"

#include <algorithm>
#include <iostream>

#include "fmt/format.h"

#include "wasp/base/enumerate.h"

namespace wasp {
namespace tools {

TextErrors::TextErrors(string_view filename, SpanU8 data)
    : filename{filename}, data{data} {}

void TextErrors::PrintTo(std::ostream& os) const {
  if (has_error()) {
    CalculateLineNumbers();
    for (const auto& error : errors) {
      os << ErrorToString(error);
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

void TextErrors::CalculateLineNumbers() const {
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

auto TextErrors::GetLineRange(Line line) const -> std::pair<Offset, Offset> {
  Offset data_size = data.end() - data.begin();
  Offset start = line > 0 ? line_offsets[line - 1] : 0;
  Offset end = line < line_offsets.size() ? line_offsets[line] - 1 : data_size;
  return std::pair(start, end);
}

auto TextErrors::GetLineColumn(Offset offset) const -> std::pair<Line, Column> {
  auto iter =
      std::upper_bound(line_offsets.begin(), line_offsets.end(), offset);
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

auto TextErrors::ErrorToString(const Error& error) const -> std::string {
  auto& loc = error.loc;
  Offset loc_start = loc.begin() - data.data();
  Offset loc_end = loc.end() - data.data();
  auto [line, column] = GetLineColumn(loc_start);

  const ptrdiff_t before = 4, max_size = 80;
  auto [line_start, line_end] = GetLineRange(line);

  if (line_end - line_start > max_size) {
    if (line_end - loc_start <= max_size) {  // Near the end of the line.
      line_start = line_end - max_size;
    } else if (loc_end - line_start <= max_size) {  // Near the beginning.
      line_end = line_start + max_size;
    } else {  // Somewhere in the middle.
      line_start = loc_start - before;
      line_end = line_start + max_size;
    }
  }

  // TODO: Move this somewhere so it can be reused.
  auto clamp = [](const auto& min, const auto& val, const auto& max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
  };

  loc_start = clamp(line_start, loc_start, line_end);
  loc_end = clamp(line_start, loc_end, line_end);

  Location context = {data.begin() + line_start, data.begin() + line_end};
  string_view line1 = ToStringView(context);
  std::string line2 = std::string(loc_start - line_start, ' ') +
                      std::string(loc_end - loc_start, '^') +
                      std::string(line_end - loc_end, ' ');

  return fmt::format("{}:{}:{}: {}\n{}\n{}\n", filename, line, column,
                     error.message, line1, line2);
}

}  // namespace tools
}  // namespace wasp

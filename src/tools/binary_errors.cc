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

#include "src/tools/binary_errors.h"

#include <iostream>
#include <utility>

#include "absl/strings/str_format.h"

namespace wasp::tools {

BinaryErrors::BinaryErrors(SpanU8 data) : BinaryErrors{"<unknown>", data} {}

BinaryErrors::BinaryErrors(string_view filename, SpanU8 data)
    : filename{filename}, data{data} {}

void BinaryErrors::PrintTo(std::ostream& os) {
  for (const auto& error : errors) {
    os << ErrorToString(error);
  }
}

void BinaryErrors::HandlePushContext(Location loc, string_view desc) {}

void BinaryErrors::HandlePopContext() {}

void BinaryErrors::HandleOnError(Location loc, string_view message) {
  errors.push_back(Error{loc, std::string(message)});
}

auto BinaryErrors::ErrorToString(const Error& error) const -> std::string {
  auto& loc = error.loc;
  const ptrdiff_t before = 4, after = 8, max_size = 32;
  size_t start = std::max(before, loc.begin() - data.begin()) - before;
  size_t end = data.size() - std::max(after, data.end() - loc.end()) + after;
  end = std::min(end, start + max_size);

  Location context = MakeSpan(data.begin() + start, data.begin() + end);

  std::string line1 = "    ";
  std::string line2 = "    ";

  bool space = false;
  for (auto iter = context.begin(); iter < context.end(); ++iter) {
    u8 x = *iter;
    line1 += absl::StrFormat("%02x", x);
    if (iter >= loc.begin() && iter < loc.end()) {
      line2 += "^^";
    } else {
      line2 += "  ";
    }
    if (space) {
      line1 += ' ';
      line2 += ' ';
    }
    space = !space;
  }

  return absl::StrFormat("%s:%08x: %s\n%s\n%s\n", filename,
                         loc.begin() - data.begin(), error.message, line1,
                         line2);
}

}  // namespace wasp::tools

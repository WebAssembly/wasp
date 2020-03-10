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

#include <utility>

#include "wasp/base/format.h"

namespace wasp {
namespace tools {

void BinaryErrors::PrintTo(std::ostream& file) {
  for (const auto& error : errors) {
    file << error;
  }
}

void BinaryErrors::HandlePushContext(Location loc, string_view desc) {
  context_stack.push_back(Error{loc, std::string{desc}});
}

void BinaryErrors::HandlePopContext() {
  context_stack.pop_back();
}

void BinaryErrors::HandleOnError(Location loc, string_view message) {
  std::string error =
      format("{:08x}: {}\n", loc.begin() - data.begin(), message);

  const ptrdiff_t before = 4, after = 8, max_size = 32;
  size_t start = std::max(before, loc.begin() - data.begin()) - before;
  size_t end = data.size() - std::max(after, data.end() - loc.end()) + after;
  end = std::min(end, start + max_size);

  Location context = {data.begin() + start, data.begin() + end};

  bool space = false;
  error += "    ";
  for (u8 x : context) {
    error += format("{:02x}", x);
    if (space) {
      error += ' ';
    }
    space = !space;
  }
  error += "\n    ";

  space = false;
  for (auto iter = context.begin(); iter < context.end(); ++iter) {
    if (iter >= loc.begin() && iter < loc.end()) {
      error += "^^";
    } else {
      error += "  ";
    }
    if (space) {
      error += ' ';
    }
    space = !space;
  }
  error += "\n";

  errors.push_back(error);
}

}  // namespace tools
}  // namespace wasp

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

#include <string>

#include "src/base/file.h"
#include "src/base/to_string.h"
#include "src/base/types.h"
#include "src/binary/encoding.h"
#include "src/binary/to_string.h"
#include "src/binary/types.h"
#include "src/binary/reader.h"

using namespace ::wasp;
using namespace ::wasp::binary;

int main(int argc, char** argv) {
  argc--;
  argv++;
  if (argc == 0) {
    absl::PrintF("No files.\n");
    return 1;
  }

  std::string filename{argv[0]};
  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    absl::PrintF("Error reading file.\n");
    return 1;
  }

  auto module = ReadModule(SpanU8{*optbuf}, [](const std::string& msg) {
    absl::PrintF("Error: %s\n", msg);
  });

  if (!module) {
    absl::PrintF("Unable to read module.\n");
  }

  if (!module->codes.empty()) {
    auto code = module->codes[0];
    auto opt_instrs = ReadInstrs(code.body.data);
    if (opt_instrs) {
      for (auto instr: *opt_instrs) {
        absl::PrintF("%s\n", ToString(instr));
      }
    }
  }

  return 0;
}

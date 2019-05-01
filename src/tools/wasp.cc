//
// Copyright 2019 WebAssembly Community Group participants
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

#include "src/tools/callgraph.h"
#include "src/tools/cfg.h"
#include "src/tools/dfg.h"
#include "src/tools/dump.h"

#include "wasp/base/enumerate.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

using namespace ::wasp;

using Command = int (*)(span<string_view> args);

void PrintHelp();

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  for (const auto& arg_pair : enumerate(args)) {
    auto arg = arg_pair.value;
    if (arg[0] == '-') {
      switch (arg[1]) {
        case 'h':
          PrintHelp();
          return 0;
        default:
          print("Unknown short argument {}\n", arg[0]);
          break;
      }
    } else {
      Command command = nullptr;

      if (arg == "dump") {
        command = wasp::tools::dump::Main;
      } else if (arg == "callgraph") {
        command = wasp::tools::callgraph::Main;
      } else if (arg == "cfg") {
        command = wasp::tools::cfg::Main;
      } else if (arg == "dfg") {
        command = wasp::tools::dfg::Main;
      } else {
        print("Unknown command \"{}\"\n", arg);
        return 1;
      }

      return command(span<string_view>{args}.subspan(arg_pair.index + 1));
    }
  }

  PrintHelp();
  return 1;
}

void PrintHelp() {
  print("usage: wasp <command> [<options>]\n");
  print("\n");
  print("commands:\n");
  print("  dump        Dump the contents of a WebAssembly file.\n");
  print("  callgraph   Generate DOT file for the function call graph.\n");
  print("  cfg         Generate DOT file of a function's control flow graph.\n");
  print("  dfg         Generate DOT file of a function's data flow graph.\n");
}

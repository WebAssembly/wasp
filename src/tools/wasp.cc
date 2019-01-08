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

#include "src/tools/dump.h"

#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/string_view.h"

using namespace ::wasp;

using Command = int (*)(int argc, char** argv);

void PrintHelp();

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    string_view arg = argv[i];
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
        command = wasp::tools::DumpMain;
      } else {
        print("Unknown command \"{}\"\n", arg);
        return 1;
      }

      return command(argc - i - 1, argv + i + 1);
    }
  }

  PrintHelp();
  return 1;
}

void PrintHelp() {
  print("usage: wasp <command> [<options>]\n");
  print("\n");
  print("commands:\n");
  print("  dump      Dump the contents of a WebAssembly file.\n");
}

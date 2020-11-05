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
#include <iostream>
#include <map>

#include "absl/strings/str_format.h"

#include "src/tools/argparser.h"
#include "src/tools/callgraph.h"
#include "src/tools/cfg.h"
#include "src/tools/dfg.h"
#include "src/tools/dump.h"
#include "src/tools/pattern.h"
#include "src/tools/validate.h"
#include "src/tools/wat2wasm.h"
#include "src/tools/wasm2wat.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/formatters.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"

using namespace ::wasp;

using absl::Format;

using Command = int (*)(span<const string_view> args);

void PrintHelp(int);

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  const std::map<string_view, Command> commands = {
      {"dump", wasp::tools::dump::Main},
      {"callgraph", wasp::tools::callgraph::Main},
      {"cfg", wasp::tools::cfg::Main},
      {"dfg", wasp::tools::dfg::Main},
      {"validate", wasp::tools::validate::Main},
      {"pattern", wasp::tools::pattern::Main},
      {"wat2wasm", wasp::tools::wat2wasm::Main},
      {"wasm2wat", wasp::tools::wasm2wat::Main},
  };

  wasp::tools::ArgParser parser{"wasp"};
  parser.Add('h', "--help", "print help and exit", []() { PrintHelp(0); })
      .Add("<command>", "command", [&](string_view arg) {
        auto iter = commands.find(arg);
        if (iter == commands.end()) {
          Format(&std::cerr, "Unknown command `%s`\n", arg);
          PrintHelp(1);
        } else {
          exit(iter->second(parser.RestOfArgs()));
        }
      });
  parser.Parse(args);
  PrintHelp(1);
}

void PrintHelp(int errcode) {
  Format(&std::cerr, "usage: wasp <command> [<options>]\n");
  Format(&std::cerr, "\n");
  Format(&std::cerr, "commands:\n");
  Format(&std::cerr, "  dump        Dump the contents of a WebAssembly file.\n");
  Format(&std::cerr, "  callgraph   Generate DOT file for the function call graph.\n");
  Format(&std::cerr, "  cfg         Generate DOT file of a function's control flow graph.\n");
  Format(&std::cerr, "  dfg         Generate DOT file of a function's data flow graph.\n");
  Format(&std::cerr, "  validate    Validate a WebAssembly file.\n");
  Format(&std::cerr, "  pattern     Find common instruction sequences.\n");
  Format(&std::cerr, "  wat2wasm    Convert a WebAssembly text file to binary.\n");
  Format(&std::cerr, "  wasm2wat    Convert a WebAssembly binary file to text.\n");
  exit(errcode);
}

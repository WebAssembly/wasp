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

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <utility>

#include "src/tools/argparser.h"
#include "src/tools/text_errors.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/error.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/convert/to_binary.h"
#include "wasp/text/desugar.h"
#include "wasp/text/read.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/tokenizer.h"
#include "wasp/text/resolve.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

using namespace ::wasp;
namespace fs = std::filesystem;

static bool s_verbose = false;

void DoFile(const fs::path&, const Features&);

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  std::vector<string_view> filenames;

  tools::ArgParser parser{"run_spec_tests"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() {
             print(parser.GetHelpString());
             exit(0);
           })
      .Add('v', "--verbose", "verbose output", [&]() { s_verbose = true; })
      .Add("<filename>", "filename",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    print(std::cerr, "No filename given.\n");
    return 1;
  }

  std::vector<fs::path> sources;

  for (auto&& filename:  filenames) {
    if (fs::is_directory(filename)) {
      for (auto& dir : fs::recursive_directory_iterator(filename)) {
        if (dir.path().extension() == ".wast") {
          sources.push_back(dir.path());
        }
      }
    } else if (fs::is_regular_file(filename)) {
      sources.push_back(fs::path{filename});
    }
  }

  std::vector<std::pair<std::string, Features::Bits>> directory_feature_map = {
      {"bulk-memory-operations", Features::BulkMemory},
      {"mutable-global", Features::MutableGlobals},
      {"reference-types", Features::ReferenceTypes},
      {"simd", Features::Simd},
      {"tail-call", Features::TailCall},
      {"threads", Features::Threads},
  };

  std::sort(sources.begin(), sources.end());
  for (auto& source : sources) {
    Features features;
    for (auto&& [directory, feature_bits] : directory_feature_map) {
      if (source.string().find(directory) != std::string::npos) {
        features = Features{feature_bits};
      }
    }

    // TODO: Merge these defaults into features.def, since they're now merged
    // to the upstream spec.
    features.enable_mutable_globals();
    features.enable_multi_value();
    features.enable_saturating_float_to_int();
    features.enable_sign_extension();

    DoFile(source, features);
  }
}

void DoFile(const fs::path& path, const Features& features) {
  if (s_verbose) {
    print("Reading {}...\n", path.string());
  }

  auto data = ReadFile(path.string());
  if (!data) {
    print(std::cerr, "Error reading file {}", path.filename().string());
    return;
  }

  text::Tokenizer tokenizer{*data};
  std::string filename = path.string();
  tools::TextErrors errors{filename, *data};
  text::Context context{features, errors};
  auto script = ReadScript(tokenizer, context);

  if (!script || errors.has_error()) {
    errors.PrintTo(std::cerr);
    return;
  }

  for (auto&& command : *script) {
    if (command->is_script_module()) {
      auto&& script_module = command->script_module();
      if (script_module.has_module()) {
        auto&& text_module = script_module.module();
        text::Desugar(text_module);
        convert::Context convert_context;
        auto binary_module = convert::ToBinary(convert_context, text_module);
        valid::Context valid_context{features, errors};
        Validate(valid_context, binary_module);
      }
    }
  }

  if (errors.has_error()) {
    errors.PrintTo(std::cerr);
  }
}

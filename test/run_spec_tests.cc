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

#include "src/tools/argparser.h"

#include <algorithm>
#include <filesystem>
#include <utility>

#include "wasp/base/enumerate.h"
#include "wasp/base/error.h"
#include "wasp/base/errors.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/hash.h"
#include "wasp/text/read.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/tokenizer.h"

using namespace ::wasp;
namespace fs = std::filesystem;

class ErrorsBasic : public Errors {
 public:
  using Offset = size_t;
  using Line = size_t;
  using Column = size_t;

  explicit ErrorsBasic(const fs::path& path, SpanU8 data)
      : path{path}, data{data} {}

  void Print() {
    if (has_error()) {
      CalculateLineNumbers();
      for (const auto& error : errors) {
        auto offset = error.loc.data() - data.data();
        auto [line, column] = GetLineColumn(offset);
        print(stderr, "{}:{}:{}: {}\n", path.filename().string(), line, column,
              error.message);
      }
    }
  }

  bool has_error() const {
    return !errors.empty();
  }

 protected:
  void HandlePushContext(SpanU8 pos, string_view desc) override {}
  void HandlePopContext() override {}
  void HandleOnError(Location loc, string_view message) override {
    errors.push_back(Error{loc, std::string{message}});
  }

  void CalculateLineNumbers() {
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

  std::pair<Line, Column> GetLineColumn(Offset offset) {
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

  fs::path path;
  SpanU8 data;
  std::vector<Error> errors;
  std::vector<Offset> line_offsets;
};

void DoFile(const fs::path&, const Features&);

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  std::vector<string_view> filenames;

  wasp::tools::ArgParser parser{"run_spec_tests"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() {
             print(parser.GetHelpString());
             exit(0);
           })
      .Add("<filename>", "filename",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    print(stderr, "No filename given.\n");
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
    // TODO: Merge these defaults into features.def, since they're now merged
    // to the upstream spec.
    features.enable_multi_value();
    features.enable_saturating_float_to_int();
    features.enable_sign_extension();

    for (auto&& [directory, feature_bits] : directory_feature_map) {
      if (source.string().find(directory) != std::string::npos) {
        features = Features{feature_bits};
      }
    }
    DoFile(source, features);
  }
}

void DoFile(const fs::path& path, const Features& features) {
  print("Reading {}...\n", path.string());
  auto data = ReadFile(path.string());
  if (!data) {
    print(stderr, "Error reading file {}", path.filename().string());
    return;
  }

  text::Tokenizer tokenizer{*data};
  ErrorsBasic errors{path, *data};
  text::Context context{features, errors};
  auto script = ReadScript(tokenizer, context);

  if (errors.has_error()) {
    errors.Print();
  }
}

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

#include <filesystem>
#include <fstream>
#include <iostream>

#include "src/tools/argparser.h"
#include "src/tools/text_errors.h"
#include "wasp/base/buffer.h"
#include "wasp/base/errors.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/encoding.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/types.h"
#include "wasp/binary/visitor.h"
#include "wasp/binary/write.h"
#include "wasp/convert/to_binary.h"
#include "wasp/text/desugar.h"
#include "wasp/text/read.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/tokenizer.h"
#include "wasp/text/resolve.h"
#include "wasp/text/types.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

namespace fs = std::filesystem;

namespace wasp {
namespace tools {
namespace wat2wasm {

struct Options {
  Features features;
  bool validate = true;
  std::string output_filename;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);

  enum class PrintChars { No, Yes };

  int Run();

  std::string filename;
  Options options;
  SpanU8 data;
};

int Main(span<string_view> args) {
  string_view filename;
  Options options;

  ArgParser parser{"wasp wat2wasm"};
  parser
      .Add("--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add("--no-validate", "Don't validate before writing",
           [&]() { options.validate = false; })
      .AddFeatureFlags(options.features)
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          print(std::cerr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    print(std::cerr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print(std::cerr, "Error reading file {}.\n", filename);
    return 1;
  }

  if (options.output_filename.empty()) {
    // Create an output filename from the input filename.
    options.output_filename =
        fs::path(filename).replace_extension(".wasm").string();
  }

  SpanU8 data{*optbuf};
  Tool tool{filename, data, options};
  return tool.Run();
}

Tool::Tool(string_view filename, SpanU8 data, Options options)
    : filename{filename}, options{options}, data{data} {}

int Tool::Run() {
  text::Tokenizer tokenizer{data};
  tools::TextErrors errors{filename, data};
  text::Context read_context{options.features, errors};
  auto text_module =
      ReadModule(tokenizer, read_context).value_or(text::Module{});
  Resolve(read_context, text_module);
  Desugar(text_module);

  if (errors.has_error()) {
    errors.PrintTo(std::cerr);
    return 1;
  }

  convert::Context convert_context;
  auto binary_module = convert::ToBinary(convert_context, text_module);

  if (options.validate) {
    valid::Context validate_context{options.features, errors};
    Validate(validate_context, binary_module);

    if (errors.has_error()) {
      errors.PrintTo(std::cerr);
      return 1;
    }
  }

  Buffer buffer;
  Write(binary_module, std::back_inserter(buffer));

  std::ofstream fstream(options.output_filename,
                        std::ios_base::out | std::ios_base::binary);
  if (!fstream) {
    print(std::cerr, "Unable to open file {}.\n", options.output_filename);
    return 1;
  }

  auto span = ToStringView(buffer);
  fstream.write(span.data(), span.size());
  return 0;
}

}  // namespace wat2wasm
}  // namespace tools
}  // namespace wasp

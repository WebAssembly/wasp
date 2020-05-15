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
#include "src/tools/text-errors.h"
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
#include "wasp/binary/write.h"
#include "wasp/convert/to_binary.h"
#include "wasp/text/desugar.h"
#include "wasp/text/read.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/tokenizer.h"
#include "wasp/text/resolve.h"
#include "wasp/text/types.h"

namespace wasp {
namespace tools {
namespace wat2wasm {

struct Options {
  Features features;
  string_view output_filename;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);

  enum class PrintChars { No, Yes };

  int Run();
  void PrintMemory(SpanU8 data,
                   Index offset,
                   PrintChars print_chars = PrintChars::Yes,
                   string_view prefix = "",
                   int octets_per_line = 16,
                   int octets_per_group = 2);

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
      .AddFeatureFlags(options.features)
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          print(stderr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    print(stderr, "No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    print(stderr, "Error reading file {}.\n", filename);
    return 1;
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
  auto text_module = ReadModule(tokenizer, read_context);
  Resolve(read_context, text_module);
  Desugar(text_module);

  if (errors.has_error()) {
    errors.Print();
    return 1;
  }

  convert::Context convert_context;
  auto binary_module = convert::ToBinary(convert_context, text_module);

  Buffer buffer;
  Write(binary_module, std::back_inserter(buffer));
  PrintMemory(buffer, 0);
  return 0;
}

void Tool::PrintMemory(SpanU8 start,
                       Index offset,
                       PrintChars print_chars,
                       string_view prefix,
                       int octets_per_line,
                       int octets_per_group) {
  SpanU8 data = start;
  while (!data.empty()) {
    auto line_size = std::min<size_t>(data.size(), octets_per_line);
    const SpanU8 line = data.subspan(0, line_size);
    print("{}", prefix);
    print("{:08x}: ", (line.begin() - start.begin()) + offset);
    for (int i = 0; i < octets_per_line;) {
      for (int j = 0; j < octets_per_group; ++j, ++i) {
        if (i < line.size()) {
          print("{:02x}", line[i]);
        } else {
          print("  ");
        }
      }
      print(" ");
    }

    if (print_chars == PrintChars::Yes) {
      print(" ");
      for (int c : line) {
        print("{:c}", isprint(c) ? static_cast<char>(c) : '.');
      }
    }
    print("\n");
    remove_prefix(&data, line_size);
  }
}

}  // namespace wat2wasm
}  // namespace tools
}  // namespace wasp

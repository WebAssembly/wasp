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
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "wasp/base/concat.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/formatters.h"
#include "wasp/base/hash.h"
#include "wasp/base/hashmap.h"
#include "wasp/base/optional.h"
#include "wasp/base/str_to_u32.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/visitor.h"

namespace wasp {
namespace tools {
namespace pattern {

const size_t kMaxPatternSize = 5;

using absl::PrintF;
using absl::Format;

using namespace ::wasp::binary;

using Instructions = std::vector<Instruction>;

struct Options {
  Features features;
  string_view function;
  string_view output_filename;
  u32 max = 10;
};

struct Tool {
  explicit Tool(SpanU8 data, Options);

  int Run();

  struct Visitor : visit::SkipVisitor {
    explicit Visitor(Tool&);

    visit::Result OnSection(At<Section>);
    visit::Result BeginCodeSection(LazyCodeSection);
    visit::Result BeginCode(const At<Code>&);

    Tool& tool;
  };

  BinaryErrors errors;
  Options options;
  LazyModule module;

  flat_hash_map<Instructions, u64> patterns;
  u64 total_instructions = 0;
};

int Main(span<const string_view> args) {
  string_view filename;
  Options options;
  options.features.EnableAll();

  ArgParser parser{"wasp pattern"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('o', "--output", "<filename>", "write DOT file output to <filename>",
           [&](string_view arg) { options.output_filename = arg; })
      .Add('d', "--display", "<int>", "maximum to display",
           [&](string_view arg) { options.max = StrToU32(arg).value_or(10); })
      .Add("<filename>", "input wasm file", [&](string_view arg) {
        if (filename.empty()) {
          filename = arg;
        } else {
          Format(&std::cerr, "Filename already given\n");
        }
      });
  parser.Parse(args);

  if (filename.empty()) {
    Format(&std::cerr, "No filename given.\n");
    parser.PrintHelpAndExit(1);
  }

  auto optbuf = ReadFile(filename);
  if (!optbuf) {
    Format(&std::cerr, "Error reading file %s.\n", filename);
    return 1;
  }

  SpanU8 data{*optbuf};
  Tool tool{data, options};

  int result = tool.Run();
  tool.errors.PrintTo(std::cerr);
  return result;
}

Tool::Tool(SpanU8 data, Options options)
    : errors{data},
      options{options},
      module{ReadLazyModule(data, options.features, errors)} {}

int Tool::Run() {
  Visitor visitor{*this};
  visit::Visit(module, visitor);

  using pair = std::pair<Instructions, u64>;
  std::vector<pair> sorted(options.max);
  std::partial_sort_copy(
      patterns.begin(), patterns.end(), sorted.begin(), sorted.end(),
      [](const pair& lhs, const pair& rhs) { return lhs.second > rhs.second; });

  for (const auto& pattern : sorted) {
    if (pattern.second > 1) {
      u64 pattern_instructions = u64(pattern.first.size()) * pattern.second;
      PrintF("%d: [%d] %s %.2f%%\n", pattern.second, pattern.first.size(),
             concat(pattern.first),
             100.0 * pattern_instructions / total_instructions);
    }
  }
  PrintF("total instructions: %d\n", total_instructions);
  return 0;
}

Tool::Visitor::Visitor(Tool& tool) : tool{tool} {}

visit::Result Tool::Visitor::OnSection(At<Section> section) {
  return section->id() == SectionId::Code ? visit::Result::Ok
                                          : visit::Result::Skip;
}

visit::Result Tool::Visitor::BeginCodeSection(LazyCodeSection) {
  return visit::Result::Ok;
}

visit::Result Tool::Visitor::BeginCode(const At<Code>& code) {
  Instructions instructions;

  auto instrs = ReadExpression(code->body, tool.module.context);
  for (auto it = instrs.begin(), end = instrs.end(); it != end; ++it) {
    auto instr = *it;
#if 0
    // Make all offsets and indexes 0.
    if (instr.has_mem_arg_immediate()) {
      instr.mem_arg_immediate().offset = 0;
    } else if (instr.has_index_immediate()) {
      instr.index_immediate() = 0;
    }
#endif

    instructions.push_back(instr);

    if (instructions.size() > kMaxPatternSize) {
      instructions.erase(instructions.begin());
    }
    if (instructions.size() > 1) {
      Instructions slice;
      slice.push_back(instructions[0]);
      for (size_t i = 1; i < instructions.size(); ++i) {
        slice.push_back(instructions[i]);
        ++tool.patterns[slice];
      }
    }
    ++tool.total_instructions;
  }
  // Skip iterating over instructions.
  return visit::Result::Skip;
}

}  // namespace pattern
}  // namespace tools
}  // namespace wasp

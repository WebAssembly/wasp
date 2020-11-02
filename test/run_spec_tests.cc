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

#include "absl/strings/str_format.h"

#include "src/tools/argparser.h"
#include "src/tools/binary_errors.h"
#include "src/tools/text_errors.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/error.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/visitor.h"
#include "wasp/convert/to_binary.h"
#include "wasp/text/desugar.h"
#include "wasp/text/read.h"
#include "wasp/text/read/context.h"
#include "wasp/text/read/tokenizer.h"
#include "wasp/text/resolve.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

using absl::Format;
using absl::PrintF;
using absl::StrFormat;

using namespace ::wasp;
namespace fs = std::filesystem;

static int s_verbose = 0;

struct DirectoryInfo {
  std::string directory;        // Name of the directory.
  bool enabled;                 // Whether to run these tests.
  Features::Bits feature_bits;  // Features to enable for these tests.
};

const std::vector<DirectoryInfo> directory_info_map = {
    {"bulk-memory-operations", true, Features::BulkMemory},
    {"exception-handling", true, Features::Exceptions},
    {"function-references", true, Features::FunctionReferences},
    {"memory64", false, 0},
    {"mutable-global", true, Features::MutableGlobals},
    {"reference-types", true, Features::ReferenceTypes},
    {"simd", true, Features::Simd},
    {"tail-call", true, Features::TailCall},
    {"threads", true, Features::Threads},
};

class Tool {
 public:
  explicit Tool(string_view filename, SpanU8 data, const Features& features)
      : filename{filename},
        data{data},
        features{features},
        errors{filename, data} {}

  void Run();

 private:
  void OnCommand(At<text::Command>&);
  void OnScriptModuleCommand(const text::ScriptModule&);
  void OnAssertionCommand(text::Assertion&);
  void OnAssertMalformedText(Location, string_view filename, const Buffer&);
  void OnAssertMalformedBinary(Location, string_view filename, const Buffer&);
  void OnAssertInvalid(Location, const text::Module&);

  std::string filename;
  SpanU8 data;
  Features features;
  tools::TextErrors errors;
  int assertion_count = 0;
};

void DoFile(const fs::path&, const Features&);

int main(int argc, char** argv) {
  std::vector<string_view> args(argc - 1);
  std::copy(&argv[1], &argv[argc], args.begin());

  std::vector<string_view> filenames;

  tools::ArgParser parser{"run_spec_tests"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('v', "--verbose", "verbose output", [&]() { s_verbose++; })
      .Add("<filename>", "filename",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    Format(&std::cerr, "No filename given.\n");
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

  std::sort(sources.begin(), sources.end());
  for (auto& source : sources) {
    Features features;
    bool enabled = true;
    for (auto&& info : directory_info_map) {
      if (source.string().find(info.directory) != std::string::npos) {
        enabled = info.enabled;
        features = Features{info.feature_bits};
        break;
      }
    }

    if (!enabled) {
      if (s_verbose) {
        PrintF("Skipping %s.\n", source.string());
      }
      continue;
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
    PrintF("Reading %s...\n", path.string());
  }

  std::string filename = path.string();
  auto data = ReadFile(filename);
  if (!data) {
    Format(&std::cerr, "Error reading file %s", path.filename().string());
    return;
  }

  Tool tool{filename, *data, features};
  tool.Run();
}

void Tool::Run() {
  text::Tokenizer tokenizer{data};
  text::Context context{features, errors};
  auto script = ReadScript(tokenizer, context);
  if (script) {
    Resolve(*script, errors);
  }

  if (!script || errors.has_error()) {
    errors.PrintTo(std::cerr);
    return;
  }

  for (auto&& command : *script) {
    OnCommand(command);
  }

  if (errors.has_error()) {
    errors.PrintTo(std::cerr);
  }
}

void Tool::OnCommand(At<text::Command>& command) {
  switch (command->kind()) {
    case text::CommandKind::ScriptModule:
      OnScriptModuleCommand(command->script_module());
      break;

    case text::CommandKind::Assertion:
      OnAssertionCommand(command->assertion());
      break;

    default:
      break;
  }
}

void Tool::OnScriptModuleCommand(const text::ScriptModule& script_module) {
  if (script_module.has_module()) {
    auto text_module = script_module.module();
    text::Desugar(text_module);
    convert::Context convert_context;
    auto binary_module = convert::ToBinary(convert_context, text_module);
    valid::Context valid_context{features, errors};
    Validate(valid_context, binary_module);
  }
}

void Tool::OnAssertionCommand(text::Assertion& assertion) {
  if (assertion.kind != text::AssertionKind::Malformed &&
      assertion.kind != text::AssertionKind::Invalid) {
    return;
  }

  auto&& module_assertion =
      get<text::ModuleAssertion>(assertion.desc);
  auto&& script_module = module_assertion.module;

  if (script_module->has_text_list()) {
    Buffer buffer;
    ToBuffer(script_module->text_list(), buffer);

    if (assertion.kind == text::AssertionKind::Malformed) {
      if (script_module->kind == text::ScriptModuleKind::Quote) {
        OnAssertMalformedText(script_module.loc(),
                              StrFormat("malformed_%d.wat", assertion_count++),
                              buffer);
      } else {
        OnAssertMalformedBinary(
            script_module.loc(),
            StrFormat("malformed_%d.wasm", assertion_count++), buffer);
      }
    } else if (assertion.kind == text::AssertionKind::Invalid) {
      errors.OnError(script_module.loc(), "assert_invalid with quote/bin?");
    }
  } else if (script_module->has_module()) {
    OnAssertInvalid(script_module.loc(), script_module->module());
  }
}

void Tool::OnAssertMalformedText(Location loc,
                                 string_view filename,
                                 const Buffer& buffer) {
  text::Tokenizer tokenizer{buffer};
  tools::TextErrors nested_errors{filename, buffer};
  text::Context context{features, nested_errors};
  auto script = ReadScript(tokenizer, context);
  if (script) {
    Resolve(*script, nested_errors);
  }
  if (!nested_errors.has_error()) {
    errors.OnError(loc, "Expected malformed text module.");
  }
  if (s_verbose > 1) {
    nested_errors.PrintTo(std::cout);
  }
}

void Tool::OnAssertMalformedBinary(Location loc,
                                 string_view filename,
                                 const Buffer& buffer) {
  tools::BinaryErrors nested_errors{filename, buffer};
  binary::LazyModule module =
      binary::ReadModule(buffer, features, nested_errors);
  binary::visit::Visitor visitor;
  binary::visit::Visit(module, visitor);
  if (!nested_errors.has_error()) {
    errors.OnError(loc, "Expected malformed binary module.");
  }
  if (s_verbose > 1) {
    nested_errors.PrintTo(std::cout);
  }
}

void Tool::OnAssertInvalid(Location loc, const text::Module& orig_text_module) {
  tools::TextErrors nested_errors{filename, data};
  // TODO: Have to copy since Desugar modifies the module in-place. Should we
  // have a version that returns a new Module too?
  text::Module text_module = orig_text_module;
  text::Desugar(text_module);
  convert::Context convert_context;
  auto binary_module = convert::ToBinary(convert_context, text_module);
  valid::Context valid_context{features, nested_errors};
  bool result = Validate(valid_context, binary_module);
  if (result || !nested_errors.has_error()) {
    errors.OnError(loc, "Expected invalid module.");
  }
  if (s_verbose > 1) {
    nested_errors.PrintTo(std::cout);
  }
}

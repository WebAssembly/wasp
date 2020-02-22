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

#include "wasp/valid/validate.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "src/tools/argparser.h"
#include "wasp/base/enumerate.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/errors_nop.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/read/end_module.h"
#include "wasp/binary/visitor.h"
#include "wasp/valid/begin_code.h"
#include "wasp/valid/context.h"
#include "wasp/valid/errors.h"

namespace wasp {
namespace tools {
namespace validate {

using namespace ::wasp::binary;

struct Message {
  optional<u32> pos;
  std::string message;
};

// TODO: share w/ dump.cc
class ErrorsBasic : public Errors, public valid::Errors {
 public:
  explicit ErrorsBasic(SpanU8 data) : data{data} {}

  void Print() {
    for (const auto& error : errors) {
      if (error.pos) {
        print(stderr, "{:08x}: {}\n", *error.pos, error.message);
      } else {
        // TODO: get error location for validation errors.
        print(stderr, "????????: {}\n", error.message);
      }
    }
  }

  bool has_error() const {
    return !errors.empty();
  }

  using binary::Errors::OnError;
  using valid::Errors::OnError;

 protected:
  void HandlePushContext(SpanU8 pos, string_view desc) override {}
  void HandlePushContext(string_view desc) override {}
  void HandlePopContext() override {}
  void HandleOnError(SpanU8 pos, string_view message) override {
    errors.push_back(Message{pos.data() - data.data(), message.to_string()});
  }
  void HandleOnError(string_view message) override {
    errors.push_back(Message{nullopt, message.to_string()});
  }

  SpanU8 data;
  std::vector<Message> errors;
};

struct Options {
  Features features;
  bool verbose = false;
};

struct Tool {
  explicit Tool(string_view filename, SpanU8 data, Options);

  bool Run();

  struct Visitor : visit::Visitor {
    explicit Visitor(Tool&);

    visit::Result EndModule();
    visit::Result OnType(const At<TypeEntry>&);
    visit::Result OnImport(const At<Import>&);
    visit::Result OnFunction(const At<Function>&);
    visit::Result OnTable(const At<Table>&);
    visit::Result OnMemory(const At<Memory>&);
    visit::Result OnGlobal(const At<Global>&);
    visit::Result OnExport(const At<Export>&);
    visit::Result OnStart(const At<Start>&);
    visit::Result OnElement(const At<ElementSegment>&);
    visit::Result OnDataCount(const At<DataCount>&);
    visit::Result OnCode(const At<Code>&);
    visit::Result OnData(const At<DataSegment>&);

    visit::Result FailUnless(bool);

    Tool& tool;
    valid::Context& context;
    Features& features;
    ErrorsBasic& errors;
  };

  std::string filename;
  Options options;
  SpanU8 data;
  ErrorsBasic errors;
  LazyModule module;

  valid::Context context;
};

int Main(span<string_view> args) {
  std::vector<string_view> filenames;
  Options options;

  ArgParser parser{"wasp validate"};
  parser
      .Add('h', "--help", "print help and exit",
           [&]() { parser.PrintHelpAndExit(0); })
      .Add('v', "--verbose", "print filename and whether it was valid",
           [&]() { options.verbose = true; })
      .AddFeatureFlags(options.features)
      .Add("<filenames...>", "input wasm files",
           [&](string_view arg) { filenames.push_back(arg); });
  parser.Parse(args);

  if (filenames.empty()) {
    print("No filenames given.\n");
    parser.PrintHelpAndExit(1);
  }

  bool ok = true;
  for (auto filename : filenames) {
    auto optbuf = ReadFile(filename);
    if (!optbuf) {
      print(stderr, "Error reading file {}.\n", filename);
      ok = false;
      continue;
    }

    SpanU8 data{*optbuf};
    Tool tool{filename, data, options};
    bool valid = tool.Run();
    if (options.verbose) {
      print("[{:^4}] {}\n", valid ? "OK" : "FAIL", filename);
      tool.errors.Print();
    }
    ok &= valid;
  }

  return ok ? 0 : 1;
}

Tool::Tool(string_view filename, SpanU8 data, Options options)
    : filename(filename),
      options{options},
      data{data},
      errors{data},
      module{ReadModule(data, options.features, errors)} {}

bool Tool::Run() {
  if (module.magic && module.version) {
    Visitor visitor{*this};
    visit::Visit(module, visitor);
  }
  return !errors.has_error();
}

Tool::Visitor::Visitor(Tool& tool)
    : tool{tool},
      context{tool.context},
      features{tool.options.features},
      errors{tool.errors} {}

visit::Result Tool::Visitor::EndModule() {
  return FailUnless(binary::EndModule(&tool.module.data, tool.module.context));
}

visit::Result Tool::Visitor::OnType(const At<TypeEntry>& type_entry) {
  return FailUnless(Validate(type_entry, context, features, errors));
}

visit::Result Tool::Visitor::OnImport(const At<Import>& import) {
  return FailUnless(Validate(import, context, features, errors));
}

visit::Result Tool::Visitor::OnFunction(const At<Function>& function) {
  return FailUnless(Validate(function, context, features, errors));
}

visit::Result Tool::Visitor::OnTable(const At<Table>& table) {
  return FailUnless(Validate(table, context, features, errors));
}

visit::Result Tool::Visitor::OnMemory(const At<Memory>& memory) {
  return FailUnless(Validate(memory, context, features, errors));
}

visit::Result Tool::Visitor::OnGlobal(const At<Global>& global) {
  return FailUnless(Validate(global, context, features, errors));
}

visit::Result Tool::Visitor::OnExport(const At<Export>& export_) {
  return FailUnless(Validate(export_, context, features, errors));
}

visit::Result Tool::Visitor::OnStart(const At<Start>& start) {
  return FailUnless(Validate(start, context, features, errors));
}

visit::Result Tool::Visitor::OnElement(const At<ElementSegment>& segment) {
  return FailUnless(Validate(segment, context, features, errors));
}

visit::Result Tool::Visitor::OnDataCount(const At<DataCount>& data_count) {
  return FailUnless(Validate(data_count, context, features, errors));
}

visit::Result Tool::Visitor::OnCode(const At<Code>& code) {
  if (!BeginCode(context, features, errors)) {
    return visit::Result::Fail;
  }

  for (const auto& locals : code->locals) {
    if (!Validate(locals, context, features, errors)) {
      return visit::Result::Fail;
    }
  }

  for (const auto& instruction :
       ReadExpression(code->body, tool.module.context)) {
    if (!Validate(instruction, context, features, errors)) {
      return visit::Result::Fail;
    }
  }

  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnData(const At<DataSegment>& segment) {
  return FailUnless(Validate(segment, context, features, errors));
}

visit::Result Tool::Visitor::FailUnless(bool b) {
  return b ? visit::Result::Ok : visit::Result::Fail;
}

}  // namespace validate
}  // namespace tools
}  // namespace wasp

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
#include "wasp/base/error.h"
#include "wasp/base/errors.h"
#include "wasp/base/features.h"
#include "wasp/base/file.h"
#include "wasp/base/format.h"
#include "wasp/base/formatters.h"
#include "wasp/base/optional.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_expression.h"
#include "wasp/binary/read/end_module.h"
#include "wasp/binary/visitor.h"
#include "wasp/valid/begin_code.h"
#include "wasp/valid/context.h"

namespace wasp {
namespace tools {
namespace validate {

using namespace ::wasp::binary;

// TODO: share w/ dump.cc
class ErrorsBasic : public Errors {
 public:
  explicit ErrorsBasic(SpanU8 data) : data{data} {}

  void Print() {
    for (const auto& error : errors) {
      print(stderr, "{:08x}: {}\n", error.loc.data() - data.data(),
            error.message);
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

  SpanU8 data;
  std::vector<Error> errors;
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
    if (!valid || options.verbose) {
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
      module{ReadModule(data, options.features, errors)},
      context{options.features, errors} {}

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
  return FailUnless(Validate(type_entry, context));
}

visit::Result Tool::Visitor::OnImport(const At<Import>& import) {
  return FailUnless(Validate(import, context));
}

visit::Result Tool::Visitor::OnFunction(const At<Function>& function) {
  return FailUnless(Validate(function, context));
}

visit::Result Tool::Visitor::OnTable(const At<Table>& table) {
  return FailUnless(Validate(table, context));
}

visit::Result Tool::Visitor::OnMemory(const At<Memory>& memory) {
  return FailUnless(Validate(memory, context));
}

visit::Result Tool::Visitor::OnGlobal(const At<Global>& global) {
  return FailUnless(Validate(global, context));
}

visit::Result Tool::Visitor::OnExport(const At<Export>& export_) {
  return FailUnless(Validate(export_, context));
}

visit::Result Tool::Visitor::OnStart(const At<Start>& start) {
  return FailUnless(Validate(start, context));
}

visit::Result Tool::Visitor::OnElement(const At<ElementSegment>& segment) {
  return FailUnless(Validate(segment, context));
}

visit::Result Tool::Visitor::OnDataCount(const At<DataCount>& data_count) {
  return FailUnless(Validate(data_count, context));
}

visit::Result Tool::Visitor::OnCode(const At<Code>& code) {
  if (!BeginCode(code.loc(), context)) {
    return visit::Result::Fail;
  }

  for (const auto& locals : code->locals) {
    if (!Validate(locals, context)) {
      return visit::Result::Fail;
    }
  }

  for (const auto& instruction :
       ReadExpression(code->body, tool.module.context)) {
    if (!Validate(instruction, context)) {
      return visit::Result::Fail;
    }
  }

  return visit::Result::Ok;
}

visit::Result Tool::Visitor::OnData(const At<DataSegment>& segment) {
  return FailUnless(Validate(segment, context));
}

visit::Result Tool::Visitor::FailUnless(bool b) {
  return b ? visit::Result::Ok : visit::Result::Fail;
}

}  // namespace validate
}  // namespace tools
}  // namespace wasp

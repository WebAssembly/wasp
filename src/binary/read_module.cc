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

#include "wasp/binary/read.h"

#include "wasp/base/errors_context_guard.h"
#include "wasp/binary/lazy_module.h"
#include "wasp/binary/read/location_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/visitor.h"

namespace wasp::binary {

using visit::Result;

struct EagerModuleVisitor : visit::Visitor {
  explicit EagerModuleVisitor(Module& module) : module{module} {}

  auto OnType(const At<DefinedType>& type) -> Result {
    module.types.push_back(type);
    return Result::Ok;
  }

  auto OnImport(const At<Import>& import) -> Result {
    module.imports.push_back(import);
    return Result::Ok;
  }

  auto OnFunction(const At<Function>& function) -> Result {
    module.functions.push_back(function);
    return Result::Ok;
  }

  auto OnTable(const At<Table>& table) -> Result {
    module.tables.push_back(table);
    return Result::Ok;
  }

  auto OnMemory(const At<Memory>& memory) -> Result {
    module.memories.push_back(memory);
    return Result::Ok;
  }

  auto OnGlobal(const At<Global>& global) -> Result {
    module.globals.push_back(global);
    return Result::Ok;
  }

  auto OnEvent(const At<Event>& event) -> Result {
    module.events.push_back(event);
    return Result::Ok;
  }

  auto OnExport(const At<Export>& export_) -> Result {
    module.exports.push_back(export_);
    return Result::Ok;
  }

  auto OnStart(const At<Start>& start) -> Result {
    module.start = start;
    return Result::Ok;
  }

  auto OnElement(const At<ElementSegment>& element_segment) -> Result {
    module.element_segments.push_back(element_segment);
    return Result::Ok;
  }

  auto OnDataCount(const At<DataCount>& data_count) -> Result {
    module.data_count = data_count;
    return Result::Ok;
  }

  auto BeginCode(const At<Code>& code) -> Result {
    module.codes.push_back(At{code.loc(), UnpackedCode{code->locals, {}}});
    return Result::Ok;
  }

  auto OnInstruction(const At<Instruction>& instruction) -> Result {
    module.codes.back()->body.instructions.push_back(instruction);
    return Result::Ok;
  }

  auto OnData(const At<DataSegment>& data_segment) -> Result {
    module.data_segments.push_back(data_segment);
    return Result::Ok;
  }

  Module& module;
};

auto ReadModule(SpanU8 data, Context& context) -> optional<Module> {
  ErrorsContextGuard error_guard{context.errors, data, "module"};
  LazyModule lazy_module{data, context.features, context.errors};
  if (!(lazy_module.magic.has_value() && lazy_module.version.has_value())) {
    return nullopt;
  }

  Module module;
  EagerModuleVisitor visitor{module};
  if (Visit(lazy_module, visitor) == Result::Fail ||
      context.errors.HasError()) {
    return nullopt;
  }
  return module;
}

}  // namespace wasp::binary

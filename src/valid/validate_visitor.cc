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

#include "wasp/valid/validate_visitor.h"

#include <cassert>

#include "wasp/binary/read/end_module.h"
#include "wasp/valid/begin_code.h"

namespace wasp {
namespace valid {

ValidateVisitor::ValidateVisitor(Features features, Errors& errors)
    : context{features, errors}, features{features}, errors{errors} {}

auto ValidateVisitor::EndModule(binary::LazyModule& module) -> Result {
  return FailUnless(binary::EndModule(&module.data, module.context) &&
                    valid::EndModule(context));
}

auto ValidateVisitor::OnType(const At<binary::TypeEntry>& type_entry)
    -> Result {
  return FailUnless(Validate(type_entry, context));
}

auto ValidateVisitor::OnImport(const At<binary::Import>& import) -> Result {
  return FailUnless(Validate(import, context));
}

auto ValidateVisitor::OnFunction(const At<binary::Function>& function)
    -> Result {
  return FailUnless(Validate(function, context));
}

auto ValidateVisitor::OnTable(const At<binary::Table>& table) -> Result {
  return FailUnless(Validate(table, context));
}

auto ValidateVisitor::OnMemory(const At<binary::Memory>& memory) -> Result {
  return FailUnless(Validate(memory, context));
}

auto ValidateVisitor::OnGlobal(const At<binary::Global>& global) -> Result {
  return FailUnless(Validate(global, context));
}

auto ValidateVisitor::OnExport(const At<binary::Export>& export_) -> Result {
  return FailUnless(Validate(export_, context));
}

auto ValidateVisitor::OnStart(const At<binary::Start>& start) -> Result {
  return FailUnless(Validate(start, context));
}

auto ValidateVisitor::OnElement(const At<binary::ElementSegment>& segment)
    -> Result {
  return FailUnless(Validate(segment, context));
}

auto ValidateVisitor::OnDataCount(const At<binary::DataCount>& data_count)
    -> Result {
  return FailUnless(Validate(data_count, context));
}

auto ValidateVisitor::OnCode(const At<binary::Code>& code) -> Result {
  if (!BeginCode(code.loc(), context)) {
    return Result::Fail;
  }

  for (const auto& locals : code->locals) {
    if (!Validate(locals, context)) {
      return Result::Fail;
    }
  }
  return Result::Ok;
}

auto ValidateVisitor::OnInstruction(const At<binary::Instruction>& instruction)
    -> Result {
  return FailUnless(Validate(instruction, context));
}

auto ValidateVisitor::OnData(const At<binary::DataSegment>& segment) -> Result {
  return FailUnless(Validate(segment, context));
}

auto ValidateVisitor::FailUnless(bool b) -> Result {
  return b ? Result::Ok : Result::Fail;
}

}  // namespace valid
}  // namespace wasp

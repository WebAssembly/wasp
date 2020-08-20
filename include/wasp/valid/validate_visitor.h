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

#ifndef WASP_VALID_VALIDATE_VISITOR_H_
#define WASP_VALID_VALIDATE_VISITOR_H_

#include "wasp/binary/visitor.h"
#include "wasp/valid/context.h"
#include "wasp/valid/validate.h"

namespace wasp {

class Errors;

namespace valid {

struct ValidateVisitor : binary::visit::Visitor {
  using Result = binary::visit::Result;

  explicit ValidateVisitor(Features features, Errors& errors);

  auto BeginTypeSection(binary::LazyTypeSection) -> Result;
  auto OnType(const At<binary::DefinedType>&) -> Result;
  auto OnImport(const At<binary::Import>&) -> Result;
  auto OnFunction(const At<binary::Function>&) -> Result;
  auto OnTable(const At<binary::Table>&) -> Result;
  auto OnMemory(const At<binary::Memory>&) -> Result;
  auto OnGlobal(const At<binary::Global>&) -> Result;
  auto OnExport(const At<binary::Export>&) -> Result;
  auto OnStart(const At<binary::Start>&) -> Result;
  auto OnElement(const At<binary::ElementSegment>&) -> Result;
  auto OnDataCount(const At<binary::DataCount>&) -> Result;
  auto BeginCode(const At<binary::Code>&) -> Result;
  auto OnInstruction(const At<binary::Instruction>&) -> Result;
  auto OnData(const At<binary::DataSegment>&) -> Result;

  auto FailUnless(bool) -> Result;

  valid::Context context;
  Features features;
  Errors& errors;
};

}  // namespace valid
}  // namespace wasp

#endif  // WASP_VALID_VALIDATE_VISITOR_H_

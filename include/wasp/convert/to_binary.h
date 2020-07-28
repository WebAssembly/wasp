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

#ifndef WASP_CONVERT_TO_BINARY_H_
#define WASP_CONVERT_TO_BINARY_H_

#include <memory>
#include <string>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/buffer.h"
#include "wasp/base/optional.h"
#include "wasp/binary/types.h"
#include "wasp/text/types.h"

namespace wasp {
namespace convert {

struct Context {
  string_view Add(std::string);
  SpanU8 Add(Buffer);

  // TODO: The buffers need to have stable pointers (for use by string_view or
  // span), so for now they are all std::unique_ptr<T>. This could be optimized
  // in a number of ways, however (e.g. allocating in larger blocks of chars,
  // and appending to that).
  std::vector<std::unique_ptr<std::string>> strings;
  std::vector<std::unique_ptr<Buffer>> buffers;
};

// Helpers.
auto ToBinary(Context&, const At<text::HeapType>&) -> At<binary::HeapType>;
auto ToBinary(Context&, const At<text::RefType>&) -> At<binary::RefType>;
auto ToBinary(Context&, const At<text::ReferenceType>&) -> At<binary::ReferenceType>;
auto ToBinary(Context&, const At<text::ValueType>&) -> At<binary::ValueType>;
auto ToBinary(Context&, const text::ValueTypeList&) -> binary::ValueTypeList;
auto ToBinary(Context&, const At<text::Text>&) -> At<string_view>;
auto ToBinary(Context&, const At<text::Var>&) -> At<Index>;
auto ToBinary(Context&, const OptAt<text::Var>&) -> At<Index>;
auto ToBinary(Context&, const OptAt<text::Var>&, Index default_index) -> At<Index>;
auto ToBinary(Context&, const text::VarList&) -> binary::IndexList;
auto ToBinary(Context&, const At<text::FunctionType>&) -> At<binary::FunctionType>;

// Section 1: Type
auto ToBinary(Context&, const text::BoundValueTypeList&) -> binary::ValueTypeList;
auto ToBinary(Context&, const At<text::TypeEntry>&) -> At<binary::TypeEntry>;

// Section 2: Import
auto ToBinary(Context&, const At<text::Import>&) -> At<binary::Import>;

// Section 3: Function
auto ToBinary(Context&, const At<text::Function>&) -> OptAt<binary::Function>;

// Section 4: Table
auto ToBinary(Context&, const At<text::TableType>&) -> At<binary::TableType>;
auto ToBinary(Context&, const At<text::Table>&) -> OptAt<binary::Table>;

// Section 5: Memory
auto ToBinary(Context&, const At<text::Memory>&) -> OptAt<binary::Memory>;

// Section 6: Global
auto ToBinary(Context&, const At<text::ConstantExpression>&) -> At<binary::ConstantExpression>;
auto ToBinary(Context&, const At<text::GlobalType>&) -> At<binary::GlobalType>;
auto ToBinary(Context&, const At<text::Global>&) -> OptAt<binary::Global>;

// Section 7: Export
auto ToBinary(Context&, const At<text::Export>&) -> At<binary::Export>;

// Section 8: Start
auto ToBinary(Context&, const At<text::Start>&) -> At<binary::Start>;

// Section 9: Elem
auto ToBinary(Context&, const At<text::ElementExpression>&) -> At<binary::ElementExpression>;
auto ToBinary(Context&, const text::ElementExpressionList&) -> binary::ElementExpressionList;
auto ToBinary(Context&, const text::ElementList&) -> binary::ElementList;
auto ToBinary(Context&, const At<text::ElementSegment>&) -> At<binary::ElementSegment>;

// Section 10: Code
auto ToBinary(Context&, const At<text::BlockImmediate>&) -> At<binary::BlockType>;
auto ToBinary(Context&, const At<text::BrOnExnImmediate>&) -> At<binary::BrOnExnImmediate>;
auto ToBinary(Context&, const At<text::BrTableImmediate>&) -> At<binary::BrTableImmediate>;
auto ToBinary(Context&, const At<text::CallIndirectImmediate>&) -> At<binary::CallIndirectImmediate>;
auto ToBinary(Context&, const At<text::CopyImmediate>&) -> At<binary::CopyImmediate>;
auto ToBinary(Context&, const At<text::InitImmediate>&) -> At<binary::InitImmediate>;
auto ToBinary(Context&, const At<text::LetImmediate>&) -> At<binary::LetImmediate>;
auto ToBinary(Context&, const At<text::MemArgImmediate>&, u32 natural_align) -> At<binary::MemArgImmediate>;
auto ToBinary(Context&, const At<text::Instruction>&) -> At<binary::Instruction>;
auto ToBinary(Context&, const text::InstructionList&) -> binary::InstructionList;

// TODO: Create text::Expression instead of using text::InstructionList here.
auto ToBinaryUnpackedExpression(Context&, const At<text::InstructionList>&) -> At<binary::UnpackedExpression>;
auto ToBinaryLocalsList(Context&, const At<text::BoundValueTypeList>&) -> At<binary::LocalsList>;
auto ToBinaryCode(Context&, const At<text::Function>&) -> OptAt<binary::UnpackedCode>;

// Section 11: Data
auto ToBinary(Context&, const At<text::TextList>&) -> SpanU8;
auto ToBinary(Context&, const At<text::DataSegment>&) -> At<binary::DataSegment>;

// Section 12: DataCount

// Section 13: Event
auto ToBinary(Context&, const At<text::EventType>&) -> At<binary::EventType>;
auto ToBinary(Context&, const At<text::Event>&) -> OptAt<binary::Event>;

// Module
auto ToBinary(Context&, const At<text::Module>&) -> At<binary::Module>;

}  // namespace convert
}  // namespace wasp

#endif // WASP_CONVERT_TO_BINARY_H_

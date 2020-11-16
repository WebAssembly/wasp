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

#ifndef WASP_CONVERT_TO_TEXT_H_
#define WASP_CONVERT_TO_TEXT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "wasp/base/at.h"
#include "wasp/base/buffer.h"
#include "wasp/base/optional.h"
#include "wasp/binary/types.h"
#include "wasp/text/types.h"

namespace wasp::convert {

// TODO: Rename
struct TextContext {
  text::Text Add(string_view);

  // TODO: The buffers need to have stable pointers (for use by string_view or
  // span), so for now they are all std::unique_ptr<T>. This could be optimized
  // in a number of ways, however (e.g. allocating in larger blocks of chars,
  // and appending to that).
  std::vector<std::unique_ptr<std::string>> strings;
};

// Helpers.
auto ToText(TextContext&, const At<binary::HeapType>&) -> At<text::HeapType>;
auto ToText(TextContext&, const At<binary::RefType>&) -> At<text::RefType>;
auto ToText(TextContext&, const At<binary::ReferenceType>&) -> At<text::ReferenceType>;
auto ToText(TextContext&, const At<binary::Rtt>&) -> At<text::Rtt>;
auto ToText(TextContext&, const At<binary::ValueType>&) -> At<text::ValueType>;
auto ToText(TextContext&, const binary::ValueTypeList&) -> text::ValueTypeList;
auto ToTextBound(TextContext&, const binary::ValueTypeList&) -> At<text::BoundValueTypeList>;
auto ToText(TextContext&, const At<binary::StorageType>&) -> At<text::StorageType>;
auto ToText(TextContext&, const At<string_view>&) -> At<text::Text>;
auto ToText(TextContext&, const At<Index>&) -> At<text::Var>;
auto ToText(TextContext&, const OptAt<Index>&) -> OptAt<text::Var>;
auto ToText(TextContext&, const binary::IndexList&) -> text::VarList;
auto ToText(TextContext&, const At<binary::FunctionType>&) -> At<text::FunctionType>;

// Section 1: Type
auto ToTextBound(TextContext&, const At<binary::FunctionType>&) -> At<text::BoundFunctionType>;
auto ToText(TextContext&, const At<binary::FieldType>&) -> At<text::FieldType>;
auto ToText(TextContext&, const binary::FieldTypeList&) -> text::FieldTypeList;
auto ToText(TextContext&, const At<binary::StructType>&) -> At<text::StructType>;
auto ToText(TextContext&, const At<binary::ArrayType>&) -> At<text::ArrayType>;
auto ToText(TextContext&, const At<binary::DefinedType>&) -> At<text::DefinedType>;

// Section 2: Import
auto ToText(TextContext&, const At<binary::Import>&) -> At<text::Import>;

// Section 3: Function
auto ToText(TextContext&, const At<binary::Function>&) -> At<text::Function>;

// Section 4: Table
auto ToText(TextContext&, const At<binary::TableType>&) -> At<text::TableType>;
auto ToText(TextContext&, const At<binary::Table>&) -> At<text::Table>;

// Section 5: Memory
auto ToText(TextContext&, const At<binary::Memory>&) -> At<text::Memory>;

// Section 6: Global
auto ToText(TextContext&, const At<binary::ConstantExpression>&) -> At<text::ConstantExpression>;
auto ToText(TextContext&, const At<binary::GlobalType>&) -> At<text::GlobalType>;
auto ToText(TextContext&, const At<binary::Global>&) -> At<text::Global>;

// Section 7: Export
auto ToText(TextContext&, const At<binary::Export>&) -> At<text::Export>;

// Section 8: Start
auto ToText(TextContext&, const At<binary::Start>&) -> At<text::Start>;

// Section 9: Elem
auto ToText(TextContext&, const At<binary::ElementExpression>&) -> At<text::ElementExpression>;
auto ToText(TextContext&, const binary::ElementExpressionList&) -> text::ElementExpressionList;
auto ToText(TextContext&, const binary::ElementList&) -> text::ElementList;
auto ToText(TextContext&, const At<binary::ElementSegment>&) -> At<text::ElementSegment>;

// Section 10: Code
auto ToText(TextContext&, const At<binary::BlockType>&) -> At<text::BlockImmediate>;
auto ToText(TextContext&, const At<binary::BrOnCastImmediate>&) -> At<text::BrOnCastImmediate>;
auto ToText(TextContext&, const At<binary::BrOnExnImmediate>&) -> At<text::BrOnExnImmediate>;
auto ToText(TextContext&, const At<binary::BrTableImmediate>&) -> At<text::BrTableImmediate>;
auto ToText(TextContext&, const At<binary::CallIndirectImmediate>&) -> At<text::CallIndirectImmediate>;
auto ToText(TextContext&, const At<binary::CopyImmediate>&) -> At<text::CopyImmediate>;
auto ToText(TextContext&, const At<binary::FuncBindImmediate>&) -> At<text::FuncBindImmediate>;
auto ToText(TextContext&, const At<binary::HeapType2Immediate>&) -> At<text::HeapType2Immediate>;
auto ToText(TextContext&, const At<binary::InitImmediate>&) -> At<text::InitImmediate>;
auto ToText(TextContext&, const At<binary::LetImmediate>&) -> At<text::LetImmediate>;
auto ToText(TextContext&, const At<binary::MemArgImmediate>&) -> At<text::MemArgImmediate>;
auto ToText(TextContext&, const At<binary::RttSubImmediate>&) -> At<text::RttSubImmediate>;
auto ToText(TextContext&, const At<binary::StructFieldImmediate>&) -> At<text::StructFieldImmediate>;
auto ToText(TextContext&, const At<binary::Instruction>&) -> At<text::Instruction>;
auto ToText(TextContext&, const binary::InstructionList&) -> text::InstructionList;

auto ToText(TextContext&, const At<binary::UnpackedExpression>&) -> text::InstructionList;
auto ToText(TextContext&, const binary::LocalsList&) -> At<text::BoundValueTypeList>;
auto ToText(TextContext&, const At<binary::UnpackedCode>&, At<text::Function>&) -> At<text::Function>&;

// Section 11: Data
auto ToText(TextContext&, const At<SpanU8>&) -> text::DataItemList;
auto ToText(TextContext&, const At<binary::DataSegment>&) -> At<text::DataSegment>;

// Section 12: DataCount

// Section 13: Event
auto ToText(TextContext&, const At<binary::EventType>&) -> At<text::EventType>;
auto ToText(TextContext&, const At<binary::Event>&) -> At<text::Event>;

// Module
auto ToText(TextContext&, const At<binary::Module>&) -> At<text::Module>;

}  // namespace wasp::convert

#endif // WASP_CONVERT_TO_TEXT_H_

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
#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/binary/types.h"
#include "wasp/text/types.h"

namespace wasp::convert {

struct BinCtx {
  explicit BinCtx() = default;
  explicit BinCtx(const Features&);

  string_view Add(std::string);
  SpanU8 Add(Buffer);

  Features features;

  // TODO: The buffers need to have stable pointers (for use by string_view or
  // span), so for now they are all std::unique_ptr<T>. This could be optimized
  // in a number of ways, however (e.g. allocating in larger blocks of chars,
  // and appending to that).
  std::vector<std::unique_ptr<std::string>> strings;
  std::vector<std::unique_ptr<Buffer>> buffers;
};

// Helpers.
auto ToBinary(BinCtx&, const At<text::HeapType>&) -> At<binary::HeapType>;
auto ToBinary(BinCtx&, const At<text::RefType>&) -> At<binary::RefType>;
auto ToBinary(BinCtx&, const At<text::ReferenceType>&) -> At<binary::ReferenceType>;
auto ToBinary(BinCtx&, const At<text::Rtt>&) -> At<binary::Rtt>;
auto ToBinary(BinCtx&, const At<text::ValueType>&) -> At<binary::ValueType>;
auto ToBinary(BinCtx&, const text::ValueTypeList&) -> binary::ValueTypeList;
auto ToBinary(BinCtx&, const At<text::StorageType>&) -> At<binary::StorageType>;
auto ToBinary(BinCtx&, const At<text::Text>&) -> At<string_view>;
auto ToBinary(BinCtx&, const At<text::Var>&) -> At<Index>;
auto ToBinary(BinCtx&, const OptAt<text::Var>&) -> At<Index>;
auto ToBinary(BinCtx&, const OptAt<text::Var>&, Index default_index) -> At<Index>;
auto ToBinary(BinCtx&, const text::VarList&) -> binary::IndexList;
auto ToBinary(BinCtx&, const At<text::FunctionType>&) -> At<binary::FunctionType>;

// Section 1: Type
auto ToBinary(BinCtx&, const text::BoundValueTypeList&) -> binary::ValueTypeList;
auto ToBinary(BinCtx&, const At<text::FieldType>&) -> At<binary::FieldType>;
auto ToBinary(BinCtx&, const text::FieldTypeList&) -> binary::FieldTypeList;
auto ToBinary(BinCtx&, const At<text::StructType>&) -> At<binary::StructType>;
auto ToBinary(BinCtx&, const At<text::ArrayType>&) -> At<binary::ArrayType>;
auto ToBinary(BinCtx&, const At<text::DefinedType>&) -> At<binary::DefinedType>;

// Section 2: Import
auto ToBinary(BinCtx&, const At<text::Import>&) -> At<binary::Import>;

// Section 3: Function
auto ToBinary(BinCtx&, const At<text::Function>&) -> OptAt<binary::Function>;

// Section 4: Table
auto ToBinary(BinCtx&, const At<text::TableType>&) -> At<binary::TableType>;
auto ToBinary(BinCtx&, const At<text::Table>&) -> OptAt<binary::Table>;

// Section 5: Memory
auto ToBinary(BinCtx&, const At<text::Memory>&) -> OptAt<binary::Memory>;

// Section 6: Global
auto ToBinary(BinCtx&, const At<text::ConstantExpression>&) -> At<binary::ConstantExpression>;
auto ToBinary(BinCtx&, const At<text::GlobalType>&) -> At<binary::GlobalType>;
auto ToBinary(BinCtx&, const At<text::Global>&) -> OptAt<binary::Global>;

// Section 7: Export
auto ToBinary(BinCtx&, const At<text::Export>&) -> At<binary::Export>;

// Section 8: Start
auto ToBinary(BinCtx&, const At<text::Start>&) -> At<binary::Start>;

// Section 9: Elem
auto ToBinary(BinCtx&, const At<text::ElementExpression>&) -> At<binary::ElementExpression>;
auto ToBinary(BinCtx&, const text::ElementExpressionList&) -> binary::ElementExpressionList;
auto ToBinary(BinCtx&, const text::ElementList&) -> binary::ElementList;
auto ToBinary(BinCtx&, const At<text::ElementSegment>&) -> At<binary::ElementSegment>;

// Section 10: Code
auto ToBinary(BinCtx&, const At<text::BlockImmediate>&) -> At<binary::BlockType>;
auto ToBinary(BinCtx&, const At<text::BrOnCastImmediate>&) -> At<binary::BrOnCastImmediate>;
auto ToBinary(BinCtx&, const At<text::BrOnExnImmediate>&) -> At<binary::BrOnExnImmediate>;
auto ToBinary(BinCtx&, const At<text::BrTableImmediate>&) -> At<binary::BrTableImmediate>;
auto ToBinary(BinCtx&, const At<text::CallIndirectImmediate>&) -> At<binary::CallIndirectImmediate>;
auto ToBinary(BinCtx&, const At<text::CopyImmediate>&) -> At<binary::CopyImmediate>;
auto ToBinary(BinCtx&, const At<text::FuncBindImmediate>&) -> At<binary::FuncBindImmediate>;
auto ToBinary(BinCtx&, const At<text::HeapType2Immediate>&) -> At<binary::HeapType2Immediate>;
auto ToBinary(BinCtx&, const At<text::InitImmediate>&) -> At<binary::InitImmediate>;
auto ToBinary(BinCtx&, const At<text::LetImmediate>&) -> At<binary::LetImmediate>;
auto ToBinary(BinCtx&, const At<text::MemArgImmediate>&, u32 natural_align) -> At<binary::MemArgImmediate>;
auto ToBinary(BinCtx&, const At<text::RttSubImmediate>&) -> At<binary::RttSubImmediate>;
auto ToBinary(BinCtx&, const At<text::StructFieldImmediate>&) -> At<binary::StructFieldImmediate>;
auto ToBinary(BinCtx&, const At<text::SimdMemoryLaneImmediate>&, u32 natural_align) -> At<binary::SimdMemoryLaneImmediate>;
auto ToBinary(BinCtx&, const At<text::Instruction>&) -> At<binary::Instruction>;
auto ToBinary(BinCtx&, const text::InstructionList&) -> binary::InstructionList;

// TODO: Create text::Expression instead of using text::InstructionList here.
auto ToBinaryUnpackedExpression(BinCtx&, const At<text::InstructionList>&) -> At<binary::UnpackedExpression>;
auto ToBinaryLocalsList(BinCtx&, const At<text::BoundValueTypeList>&) -> At<binary::LocalsList>;
auto ToBinaryCode(BinCtx&, const At<text::Function>&) -> OptAt<binary::UnpackedCode>;

// Section 11: Data
auto ToBinary(BinCtx&, const At<text::DataItemList>&) -> SpanU8;
auto ToBinary(BinCtx&, const At<text::DataSegment>&) -> At<binary::DataSegment>;

// Section 12: DataCount

// Section 13: Event
auto ToBinary(BinCtx&, const At<text::EventType>&) -> At<binary::EventType>;
auto ToBinary(BinCtx&, const At<text::Event>&) -> OptAt<binary::Event>;

// Module
auto ToBinary(BinCtx&, const At<text::Module>&) -> At<binary::Module>;

}  // namespace wasp::convert

#endif // WASP_CONVERT_TO_BINARY_H_

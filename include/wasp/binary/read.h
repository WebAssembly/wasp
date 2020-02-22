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

#ifndef WASP_BINARY_READ_H_
#define WASP_BINARY_READ_H_

#include <utility>

#include "wasp/base/at.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {

struct Context;

enum class BulkImmediateKind {
  Memory,
  Table,
};

template <typename T>
struct Tag {};

template <typename T, typename... Args>
OptAt<T> Read(SpanU8* data, Context& context, Args&&... args) {
  return Read(data, context, Tag<T>{}, std::forward<Args>(args)...);
}

OptAt<BlockType> Read(SpanU8*, Context&, Tag<BlockType>);

OptAt<BrOnExnImmediate> Read(SpanU8*, Context&, Tag<BrOnExnImmediate>);

OptAt<BrTableImmediate> Read(SpanU8*, Context&, Tag<BrTableImmediate>);

OptAt<SpanU8> ReadBytesExpected(SpanU8* data,
                                SpanU8 expected,
                                Context&,
                                string_view desc);

OptAt<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Context&);

OptAt<CallIndirectImmediate> Read(SpanU8*,
                                  Context&,
                                  Tag<CallIndirectImmediate>);

OptAt<Index> ReadCheckLength(SpanU8*,
                          Context&,
                          string_view context_name,
                          string_view error_name);

OptAt<Code> Read(SpanU8*, Context&, Tag<Code>);

OptAt<ConstantExpression> Read(SpanU8*, Context&, Tag<ConstantExpression>);

OptAt<CopyImmediate> Read(SpanU8* data,
                       Context&,
                       Tag<CopyImmediate>,
                       BulkImmediateKind);

OptAt<Index> ReadCount(SpanU8*, Context&);

OptAt<DataCount> Read(SpanU8*, Context&, Tag<DataCount>);

OptAt<DataSegment> Read(SpanU8*, Context&, Tag<DataSegment>);

OptAt<ElementExpression> Read(SpanU8*, Context&, Tag<ElementExpression>);

OptAt<ElementSegment> Read(SpanU8*, Context&, Tag<ElementSegment>);

OptAt<ElementType> Read(SpanU8*, Context&, Tag<ElementType>);

OptAt<Event> Read(SpanU8*, Context&, Tag<Event>);

OptAt<EventAttribute> Read(SpanU8*, Context&, Tag<EventAttribute>);

OptAt<EventType> Read(SpanU8*, Context&, Tag<EventType>);

OptAt<Export> Read(SpanU8*, Context&, Tag<Export>);

OptAt<ExternalKind> Read(SpanU8*, Context&, Tag<ExternalKind>);

OptAt<f32> Read(SpanU8*, Context&, Tag<f32>);

OptAt<f64> Read(SpanU8*, Context&, Tag<f64>);

OptAt<Function> Read(SpanU8*, Context&, Tag<Function>);

OptAt<FunctionType> Read(SpanU8*, Context&, Tag<FunctionType>);

OptAt<Global> Read(SpanU8*, Context&, Tag<Global>);

OptAt<GlobalType> Read(SpanU8*, Context&, Tag<GlobalType>);

OptAt<Import> Read(SpanU8*, Context&, Tag<Import>);

OptAt<Index> ReadIndex(SpanU8*, Context&, string_view desc);

OptAt<InitImmediate> Read(SpanU8*,
                          Context&,
                          Tag<InitImmediate>,
                          BulkImmediateKind);

OptAt<Instruction> Read(SpanU8*, Context&, Tag<Instruction>);

OptAt<Index> ReadLength(SpanU8*, Context&);

OptAt<Limits> Read(SpanU8*, Context&, Tag<Limits>);

OptAt<Locals> Read(SpanU8*, Context&, Tag<Locals>);

OptAt<MemArgImmediate> Read(SpanU8*, Context&, Tag<MemArgImmediate>);

OptAt<Memory> Read(SpanU8*, Context&, Tag<Memory>);

OptAt<MemoryType> Read(SpanU8*, Context&, Tag<MemoryType>);

OptAt<Mutability> Read(SpanU8*, Context&, Tag<Mutability>);

OptAt<Opcode> Read(SpanU8*, Context&, Tag<Opcode>);

OptAt<u8> ReadReserved(SpanU8*, Context&);

OptAt<Index> ReadReservedIndex(SpanU8*, Context&);

OptAt<s32> Read(SpanU8*, Context&, Tag<s32>);

OptAt<s64> Read(SpanU8*, Context&, Tag<s64>);

OptAt<Section> Read(SpanU8*, Context&, Tag<Section>);

OptAt<SectionId> Read(SpanU8*, Context&, Tag<SectionId>);

OptAt<ShuffleImmediate> Read(SpanU8*, Context&, Tag<ShuffleImmediate>);

OptAt<Start> Read(SpanU8*, Context&, Tag<Start>);

OptAt<string_view> ReadString(SpanU8*, Context&, string_view desc);

OptAt<string_view> ReadUtf8String(SpanU8*, Context&, string_view desc);

OptAt<Table> Read(SpanU8*, Context&, Tag<Table>);

OptAt<TableType> Read(SpanU8*, Context&, Tag<TableType>);

OptAt<TypeEntry> Read(SpanU8*, Context&, Tag<TypeEntry>);

OptAt<u32> Read(SpanU8*, Context&, Tag<u32>);

OptAt<u8> Read(SpanU8*, Context&, Tag<u8>);

OptAt<v128> Read(SpanU8*, Context&, Tag<v128>);

OptAt<ValueType> Read(SpanU8*, Context&, Tag<ValueType>);

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_H_

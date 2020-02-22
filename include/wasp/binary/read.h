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
optional<T> Read(SpanU8* data, Context& context, Args&&... args) {
  return Read(data, context, Tag<T>{}, std::forward<Args>(args)...);
}

optional<BlockType> Read(SpanU8*, Context&, Tag<BlockType>);

optional<BrOnExnImmediate> Read(SpanU8*, Context&, Tag<BrOnExnImmediate>);

optional<BrTableImmediate> Read(SpanU8*, Context&, Tag<BrTableImmediate>);

optional<SpanU8> ReadBytesExpected(SpanU8* data,
                                   SpanU8 expected,
                                   Context&,
                                   string_view desc);

optional<SpanU8> ReadBytes(SpanU8* data, SpanU8::index_type N, Context&);

optional<CallIndirectImmediate> Read(SpanU8*,
                                     Context&,
                                     Tag<CallIndirectImmediate>);

optional<Index> ReadCheckLength(SpanU8*,
                                Context&,
                                string_view context_name,
                                string_view error_name);

optional<Code> Read(SpanU8*, Context&, Tag<Code>);

optional<ConstantExpression> Read(SpanU8*, Context&, Tag<ConstantExpression>);

optional<CopyImmediate> Read(SpanU8* data,
                             Context&,
                             Tag<CopyImmediate>,
                             BulkImmediateKind);

optional<Index> ReadCount(SpanU8*, Context&);

optional<DataCount> Read(SpanU8*, Context&, Tag<DataCount>);

optional<DataSegment> Read(SpanU8*, Context&, Tag<DataSegment>);

optional<ElementExpression> Read(SpanU8*, Context&, Tag<ElementExpression>);

optional<ElementSegment> Read(SpanU8*, Context&, Tag<ElementSegment>);

optional<ElementType> Read(SpanU8*, Context&, Tag<ElementType>);

optional<Event> Read(SpanU8*, Context&, Tag<Event>);

optional<EventAttribute> Read(SpanU8*, Context&, Tag<EventAttribute>);

optional<EventType> Read(SpanU8*, Context&, Tag<EventType>);

optional<Export> Read(SpanU8*, Context&, Tag<Export>);

optional<ExternalKind> Read(SpanU8*, Context&, Tag<ExternalKind>);

optional<f32> Read(SpanU8*, Context&, Tag<f32>);

optional<f64> Read(SpanU8*, Context&, Tag<f64>);

optional<Function> Read(SpanU8*, Context&, Tag<Function>);

optional<FunctionType> Read(SpanU8*, Context&, Tag<FunctionType>);

optional<Global> Read(SpanU8*, Context&, Tag<Global>);

optional<GlobalType> Read(SpanU8*, Context&, Tag<GlobalType>);

optional<Import> Read(SpanU8*, Context&, Tag<Import>);

optional<Index> ReadIndex(SpanU8*, Context&, string_view desc);

optional<InitImmediate> Read(SpanU8*,
                             Context&,
                             Tag<InitImmediate>,
                             BulkImmediateKind);

optional<Instruction> Read(SpanU8*, Context&, Tag<Instruction>);

optional<Index> ReadLength(SpanU8*, Context&);

optional<Limits> Read(SpanU8*, Context&, Tag<Limits>);

optional<Locals> Read(SpanU8*, Context&, Tag<Locals>);

optional<MemArgImmediate> Read(SpanU8*, Context&, Tag<MemArgImmediate>);

optional<Memory> Read(SpanU8*, Context&, Tag<Memory>);

optional<MemoryType> Read(SpanU8*, Context&, Tag<MemoryType>);

optional<Mutability> Read(SpanU8*, Context&, Tag<Mutability>);

optional<Opcode> Read(SpanU8*, Context&, Tag<Opcode>);

optional<u8> ReadReserved(SpanU8*, Context&);

optional<s32> Read(SpanU8*, Context&, Tag<s32>);

optional<s64> Read(SpanU8*, Context&, Tag<s64>);

optional<Section> Read(SpanU8*, Context&, Tag<Section>);

optional<SectionId> Read(SpanU8*, Context&, Tag<SectionId>);

optional<ShuffleImmediate> Read(SpanU8*, Context&, Tag<ShuffleImmediate>);

optional<Start> Read(SpanU8*, Context&, Tag<Start>);

optional<string_view> ReadString(SpanU8*, Context&, string_view desc);

optional<string_view> ReadUtf8String(SpanU8*, Context&, string_view desc);

optional<Table> Read(SpanU8*, Context&, Tag<Table>);

optional<TableType> Read(SpanU8*, Context&, Tag<TableType>);

optional<TypeEntry> Read(SpanU8*, Context&, Tag<TypeEntry>);

optional<u32> Read(SpanU8*, Context&, Tag<u32>);

optional<u8> Read(SpanU8*, Context&, Tag<u8>);

optional<v128> Read(SpanU8*, Context&, Tag<v128>);

optional<ValueType> Read(SpanU8*, Context&, Tag<ValueType>);

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_H_

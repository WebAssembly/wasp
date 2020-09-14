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

namespace wasp::binary {

struct Context;

template <typename T>
struct Tag {};

// Generic Read function.
template <typename T, typename... Args>
auto Read(SpanU8* data, Context& context, Args&&... args) -> OptAt<T> {
  return Read(data, context, Tag<T>{}, std::forward<Args>(args)...);
}

// Read functions that return SpanU8.
auto ReadBytes(SpanU8* data, span_extent_t N, Context&) -> OptAt<SpanU8>;
auto ReadBytesExpected(SpanU8* data,
                       SpanU8 expected,
                       Context&,
                       string_view desc) -> OptAt<SpanU8>;

// Read a byte without advancing the span.
auto PeekU8(SpanU8*, Context&) -> OptAt<u8>;

// Functions to read a length/count. The difference is only in the error word
// used. Both ReadCount and ReadLength forward to ReadCheckLength.
auto ReadCount(SpanU8*, Context&) -> OptAt<Index>;
auto ReadLength(SpanU8*, Context&) -> OptAt<Index>;
auto ReadCheckLength(SpanU8*,
                     Context&,
                     string_view context_name,
                     string_view error_name) -> OptAt<Index>;

// Identical to reading a u32.
auto ReadIndex(SpanU8*, Context&, string_view desc) -> OptAt<Index>;

// Read a single byte that must be a 0. ReadReservedIndex casts the resulting
// value to an Index.
auto ReadReserved(SpanU8*, Context&) -> OptAt<u8>;
auto ReadReservedIndex(SpanU8*, Context&) -> OptAt<Index>;

// ReadString reads a string as a collection of bytes. ReadUtf8String requires
// that the string is utf-8 encoded.
auto ReadString(SpanU8*, Context&, string_view desc) -> OptAt<string_view>;
auto ReadUtf8String(SpanU8*, Context&, string_view desc) -> OptAt<string_view>;

enum class BulkImmediateKind {
  Memory,
  Table,
};

// Read functions for various binary types.
auto Read(SpanU8*, Context&, Tag<ArrayType>) -> OptAt<ArrayType>;
auto Read(SpanU8*, Context&, Tag<BlockType>) -> OptAt<BlockType>;
auto Read(SpanU8*, Context&, Tag<BrOnCastImmediate>)
    -> OptAt<BrOnCastImmediate>;
auto Read(SpanU8*, Context&, Tag<BrOnExnImmediate>) -> OptAt<BrOnExnImmediate>;
auto Read(SpanU8*, Context&, Tag<BrTableImmediate>) -> OptAt<BrTableImmediate>;
auto Read(SpanU8*, Context&, Tag<CallIndirectImmediate>)
    -> OptAt<CallIndirectImmediate>;
auto Read(SpanU8*, Context&, Tag<Code>) -> OptAt<Code>;
auto Read(SpanU8*, Context&, Tag<ConstantExpression>)
    -> OptAt<ConstantExpression>;
auto Read(SpanU8*, Context&, Tag<CopyImmediate>, BulkImmediateKind)
    -> OptAt<CopyImmediate>;
auto Read(SpanU8*, Context&, Tag<DataCount>) -> OptAt<DataCount>;
auto Read(SpanU8*, Context&, Tag<DataSegment>) -> OptAt<DataSegment>;
auto Read(SpanU8*, Context&, Tag<DefinedType>) -> OptAt<DefinedType>;
auto Read(SpanU8*, Context&, Tag<ElementExpression>)
    -> OptAt<ElementExpression>;
auto Read(SpanU8*, Context&, Tag<ElementSegment>) -> OptAt<ElementSegment>;
auto Read(SpanU8*, Context&, Tag<EventAttribute>) -> OptAt<EventAttribute>;
auto Read(SpanU8*, Context&, Tag<Event>) -> OptAt<Event>;
auto Read(SpanU8*, Context&, Tag<EventType>) -> OptAt<EventType>;
auto Read(SpanU8*, Context&, Tag<Export>) -> OptAt<Export>;
auto Read(SpanU8*, Context&, Tag<ExternalKind>) -> OptAt<ExternalKind>;
auto Read(SpanU8*, Context&, Tag<f32>) -> OptAt<f32>;
auto Read(SpanU8*, Context&, Tag<f64>) -> OptAt<f64>;
auto Read(SpanU8*, Context&, Tag<FieldType>) -> OptAt<FieldType>;
auto Read(SpanU8*, Context&, Tag<Function>) -> OptAt<Function>;
auto Read(SpanU8*, Context&, Tag<FunctionType>) -> OptAt<FunctionType>;
auto Read(SpanU8*, Context&, Tag<Global>) -> OptAt<Global>;
auto Read(SpanU8*, Context&, Tag<GlobalType>) -> OptAt<GlobalType>;
auto Read(SpanU8*, Context&, Tag<HeapType>) -> OptAt<HeapType>;
auto Read(SpanU8*, Context&, Tag<HeapType2Immediate>)
    -> OptAt<HeapType2Immediate>;
auto Read(SpanU8*, Context&, Tag<Import>) -> OptAt<Import>;
auto Read(SpanU8*, Context&, Tag<InitImmediate>, BulkImmediateKind)
    -> OptAt<InitImmediate>;
auto Read(SpanU8*, Context&, Tag<Instruction>) -> OptAt<Instruction>;
auto Read(SpanU8*, Context&, Tag<InstructionList>) -> OptAt<InstructionList>;
auto Read(SpanU8*, Context&, Tag<LetImmediate>) -> OptAt<LetImmediate>;
auto Read(SpanU8*, Context&, Tag<Limits>) -> OptAt<Limits>;
auto Read(SpanU8*, Context&, Tag<Locals>) -> OptAt<Locals>;
auto Read(SpanU8*, Context&, Tag<MemArgImmediate>) -> OptAt<MemArgImmediate>;
auto Read(SpanU8*, Context&, Tag<Memory>) -> OptAt<Memory>;
auto Read(SpanU8*, Context&, Tag<MemoryType>) -> OptAt<MemoryType>;
auto Read(SpanU8*, Context&, Tag<Mutability>) -> OptAt<Mutability>;
auto Read(SpanU8*, Context&, Tag<Opcode>) -> OptAt<Opcode>;
auto Read(SpanU8*, Context&, Tag<s32>) -> OptAt<s32>;
auto Read(SpanU8*, Context&, Tag<s64>) -> OptAt<s64>;
auto Read(SpanU8*, Context&, Tag<ReferenceType>) -> OptAt<ReferenceType>;
auto Read(SpanU8*, Context&, Tag<RttSubImmediate>) -> OptAt<RttSubImmediate>;
auto Read(SpanU8*, Context&, Tag<SectionId>) -> OptAt<SectionId>;
auto Read(SpanU8*, Context&, Tag<Section>) -> OptAt<Section>;
auto Read(SpanU8*, Context&, Tag<ShuffleImmediate>) -> OptAt<ShuffleImmediate>;
auto Read(SpanU8*, Context&, Tag<Start>) -> OptAt<Start>;
auto Read(SpanU8*, Context&, Tag<StorageType>) -> OptAt<StorageType>;
auto Read(SpanU8*, Context&, Tag<StructType>) -> OptAt<StructType>;
auto Read(SpanU8*, Context&, Tag<StructFieldImmediate>)
    -> OptAt<StructFieldImmediate>;
auto Read(SpanU8*, Context&, Tag<Table>) -> OptAt<Table>;
auto Read(SpanU8*, Context&, Tag<TableType>) -> OptAt<TableType>;
auto Read(SpanU8*, Context&, Tag<u32>) -> OptAt<u32>;
auto Read(SpanU8*, Context&, Tag<u8>) -> OptAt<u8>;
auto Read(SpanU8*, Context&, Tag<v128>) -> OptAt<v128>;
auto Read(SpanU8*, Context&, Tag<ValueType>) -> OptAt<ValueType>;

bool EndCode(SpanU8, Context&);
bool EndModule(SpanU8, Context&);

}  // namespace wasp::binary

#endif  // WASP_BINARY_READ_H_

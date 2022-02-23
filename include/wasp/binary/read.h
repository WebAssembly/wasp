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

struct ReadCtx;

// Read a full binary module eagerly (see ReadLazyModule to read lazily).
auto ReadModule(SpanU8, ReadCtx&) -> optional<Module>;


template <typename T>
struct ReadTag {};

// Generic Read function.
template <typename T, typename... Args>
auto Read(SpanU8* data, ReadCtx& ctx, Args&&... args) -> OptAt<T> {
  return Read(data, ctx, ReadTag<T>{}, std::forward<Args>(args)...);
}

// Read functions that return SpanU8.
auto ReadBytes(SpanU8* data, span_extent_t N, ReadCtx&) -> OptAt<SpanU8>;
auto ReadBytesExpected(SpanU8* data,
                       SpanU8 expected,
                       ReadCtx&,
                       string_view desc) -> OptAt<SpanU8>;

// Read a byte without advancing the span.
auto PeekU8(SpanU8*, ReadCtx&) -> OptAt<u8>;

// Functions to read a length/count. The difference is only in the error word
// used. Both ReadCount and ReadLength forward to ReadCheckLength.
auto ReadCount(SpanU8*, ReadCtx&) -> OptAt<Index>;
auto ReadLength(SpanU8*, ReadCtx&) -> OptAt<Index>;
auto ReadCheckLength(SpanU8*,
                     ReadCtx&,
                     string_view context_name,
                     string_view error_name) -> OptAt<Index>;

// Identical to reading a u32.
auto ReadIndex(SpanU8*, ReadCtx&, string_view desc) -> OptAt<Index>;

// Read a single byte that must be a 0. ReadReservedIndex casts the resulting
// value to an Index.
auto ReadReserved(SpanU8*, ReadCtx&) -> OptAt<u8>;
auto ReadReservedIndex(SpanU8*, ReadCtx&) -> OptAt<Index>;

// ReadString reads a string as a collection of bytes. ReadUtf8String requires
// that the string is utf-8 encoded.
auto ReadString(SpanU8*, ReadCtx&, string_view desc) -> OptAt<string_view>;
auto ReadUtf8String(SpanU8*, ReadCtx&, string_view desc) -> OptAt<string_view>;

enum class BulkImmediateKind { Memory, Table };
enum class LimitsKind { Memory, Table };

// Read functions for various binary types.
auto Read(SpanU8*, ReadCtx&, ReadTag<ArrayType>) -> OptAt<ArrayType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<BlockType>) -> OptAt<BlockType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<BrOnCastImmediate>)
    -> OptAt<BrOnCastImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<BrTableImmediate>)
    -> OptAt<BrTableImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<CallIndirectImmediate>)
    -> OptAt<CallIndirectImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Code>) -> OptAt<Code>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ConstantExpression>)
    -> OptAt<ConstantExpression>;
auto Read(SpanU8*, ReadCtx&, ReadTag<CopyImmediate>, BulkImmediateKind)
    -> OptAt<CopyImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<DataCount>) -> OptAt<DataCount>;
auto Read(SpanU8*, ReadCtx&, ReadTag<DataSegment>) -> OptAt<DataSegment>;
auto Read(SpanU8*, ReadCtx&, ReadTag<DefinedType>) -> OptAt<DefinedType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ElementExpression>)
    -> OptAt<ElementExpression>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ElementSegment>) -> OptAt<ElementSegment>;
auto Read(SpanU8*, ReadCtx&, ReadTag<TagAttribute>) -> OptAt<TagAttribute>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Tag>) -> OptAt<Tag>;
auto Read(SpanU8*, ReadCtx&, ReadTag<TagType>) -> OptAt<TagType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Export>) -> OptAt<Export>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ExternalKind>) -> OptAt<ExternalKind>;
auto Read(SpanU8*, ReadCtx&, ReadTag<f32>) -> OptAt<f32>;
auto Read(SpanU8*, ReadCtx&, ReadTag<f64>) -> OptAt<f64>;
auto Read(SpanU8*, ReadCtx&, ReadTag<FieldType>) -> OptAt<FieldType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<FuncBindImmediate>)
    -> OptAt<FuncBindImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Function>) -> OptAt<Function>;
auto Read(SpanU8*, ReadCtx&, ReadTag<FunctionType>) -> OptAt<FunctionType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Global>) -> OptAt<Global>;
auto Read(SpanU8*, ReadCtx&, ReadTag<GlobalType>) -> OptAt<GlobalType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<HeapType>) -> OptAt<HeapType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<HeapType2Immediate>)
    -> OptAt<HeapType2Immediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Import>) -> OptAt<Import>;
auto Read(SpanU8*, ReadCtx&, ReadTag<InitImmediate>, BulkImmediateKind)
    -> OptAt<InitImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Instruction>) -> OptAt<Instruction>;
auto Read(SpanU8*, ReadCtx&, ReadTag<InstructionList>)
    -> OptAt<InstructionList>;
auto Read(SpanU8*, ReadCtx&, ReadTag<LetImmediate>) -> OptAt<LetImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Limits>, LimitsKind) -> OptAt<Limits>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Locals>) -> OptAt<Locals>;
auto Read(SpanU8*, ReadCtx&, ReadTag<MemArgImmediate>)
    -> OptAt<MemArgImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Memory>) -> OptAt<Memory>;
auto Read(SpanU8*, ReadCtx&, ReadTag<MemoryType>) -> OptAt<MemoryType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Mutability>) -> OptAt<Mutability>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Opcode>) -> OptAt<Opcode>;
auto Read(SpanU8*, ReadCtx&, ReadTag<s32>) -> OptAt<s32>;
auto Read(SpanU8*, ReadCtx&, ReadTag<s64>) -> OptAt<s64>;
auto Read(SpanU8*, ReadCtx&, ReadTag<RefType>) -> OptAt<RefType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ReferenceType>) -> OptAt<ReferenceType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Rtt>) -> OptAt<Rtt>;
auto Read(SpanU8*, ReadCtx&, ReadTag<RttSubImmediate>)
    -> OptAt<RttSubImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<SectionId>) -> OptAt<SectionId>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Section>) -> OptAt<Section>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ShuffleImmediate>)
    -> OptAt<ShuffleImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Start>) -> OptAt<Start>;
auto Read(SpanU8*, ReadCtx&, ReadTag<StorageType>) -> OptAt<StorageType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<StructType>) -> OptAt<StructType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<StructFieldImmediate>)
    -> OptAt<StructFieldImmediate>;
auto Read(SpanU8*, ReadCtx&, ReadTag<Table>) -> OptAt<Table>;
auto Read(SpanU8*, ReadCtx&, ReadTag<TableType>) -> OptAt<TableType>;
auto Read(SpanU8*, ReadCtx&, ReadTag<u32>) -> OptAt<u32>;
auto Read(SpanU8*, ReadCtx&, ReadTag<u8>) -> OptAt<u8>;
auto Read(SpanU8*, ReadCtx&, ReadTag<v128>) -> OptAt<v128>;
auto Read(SpanU8*, ReadCtx&, ReadTag<ValueType>) -> OptAt<ValueType>;

bool EndCode(SpanU8, ReadCtx&);
bool EndModule(SpanU8, ReadCtx&);

}  // namespace wasp::binary

#endif  // WASP_BINARY_READ_H_

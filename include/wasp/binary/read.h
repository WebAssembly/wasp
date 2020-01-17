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

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/binary/types.h"

namespace wasp {

class Features;

namespace binary {

class Errors;

template <typename T>
struct Tag {};

template <typename T>
optional<T> Read(SpanU8* data, const Features& features, Errors& errors) {
  return Read(data, features, errors, Tag<T>{});
}

optional<BlockType> Read(SpanU8*, const Features&, Errors&, Tag<BlockType>);

optional<BrOnExnImmediate> Read(SpanU8*,
                                const Features&,
                                Errors&,
                                Tag<BrOnExnImmediate>);

optional<BrTableImmediate> Read(SpanU8*,
                                const Features&,
                                Errors&,
                                Tag<BrTableImmediate>);

optional<SpanU8> ReadBytesExpected(SpanU8* data,
                                   SpanU8 expected,
                                   const Features&,
                                   Errors&,
                                   string_view desc);

optional<SpanU8> ReadBytes(SpanU8* data,
                           SpanU8::index_type N,
                           const Features&,
                           Errors&);

optional<CallIndirectImmediate> Read(SpanU8*,
                                     const Features&,
                                     Errors&,
                                     Tag<CallIndirectImmediate>);

optional<Index> ReadCheckLength(SpanU8*,
                                const Features&,
                                Errors&,
                                string_view context_name,
                                string_view error_name);

optional<Code> Read(SpanU8*, const Features&, Errors&, Tag<Code>);

optional<ConstantExpression> Read(SpanU8*,
                                  const Features&,
                                  Errors&,
                                  Tag<ConstantExpression>);

optional<CopyImmediate> Read(SpanU8* data,
                             const Features& features,
                             Errors& errors,
                             Tag<CopyImmediate>);

optional<Index> ReadCount(SpanU8*, const Features&, Errors&);

optional<DataCount> Read(SpanU8*, const Features&, Errors&, Tag<DataCount>);

optional<DataSegment> Read(SpanU8*, const Features&, Errors&, Tag<DataSegment>);

optional<ElementExpression> Read(SpanU8*,
                                 const Features&,
                                 Errors&,
                                 Tag<ElementExpression>);

optional<ElementSegment> Read(SpanU8*,
                              const Features&,
                              Errors&,
                              Tag<ElementSegment>);

optional<ElementType> Read(SpanU8*, const Features&, Errors&, Tag<ElementType>);

optional<Export> Read(SpanU8*, const Features&, Errors&, Tag<Export>);

optional<ExternalKind> Read(SpanU8*,
                            const Features&,
                            Errors&,
                            Tag<ExternalKind>);

optional<f32> Read(SpanU8*, const Features&, Errors&, Tag<f32>);

optional<f64> Read(SpanU8*, const Features&, Errors&, Tag<f64>);

optional<Function> Read(SpanU8*, const Features&, Errors&, Tag<Function>);

optional<FunctionType> Read(SpanU8*,
                            const Features&,
                            Errors&,
                            Tag<FunctionType>);

optional<Global> Read(SpanU8*, const Features&, Errors&, Tag<Global>);

optional<GlobalType> Read(SpanU8*, const Features&, Errors&, Tag<GlobalType>);

optional<Import> Read(SpanU8*, const Features&, Errors&, Tag<Import>);

optional<Index> ReadIndex(SpanU8*, const Features&, Errors&, string_view desc);

optional<InitImmediate> Read(SpanU8*,
                             const Features&,
                             Errors&,
                             Tag<InitImmediate>);

optional<Instruction> Read(SpanU8*, const Features&, Errors&, Tag<Instruction>);

optional<Index> ReadLength(SpanU8*, const Features&, Errors&);

optional<Limits> Read(SpanU8*, const Features&, Errors&, Tag<Limits>);

optional<Locals> Read(SpanU8*, const Features&, Errors&, Tag<Locals>);

optional<MemArgImmediate> Read(SpanU8*,
                               const Features&,
                               Errors&,
                               Tag<MemArgImmediate>);

optional<Memory> Read(SpanU8*, const Features&, Errors&, Tag<Memory>);

optional<MemoryType> Read(SpanU8*, const Features&, Errors&, Tag<MemoryType>);

optional<Mutability> Read(SpanU8*, const Features&, Errors&, Tag<Mutability>);

optional<Opcode> Read(SpanU8*, const Features&, Errors&, Tag<Opcode>);

optional<u8> ReadReserved(SpanU8*, const Features&, Errors&);

optional<s32> Read(SpanU8*, const Features&, Errors&, Tag<s32>);

optional<s64> Read(SpanU8*, const Features&, Errors&, Tag<s64>);

optional<Section> Read(SpanU8*, const Features&, Errors&, Tag<Section>);

optional<SectionId> Read(SpanU8*, const Features&, Errors&, Tag<SectionId>);

optional<ShuffleImmediate> Read(SpanU8*,
                                const Features&,
                                Errors&,
                                Tag<ShuffleImmediate>);

optional<Start> Read(SpanU8*, const Features&, Errors&, Tag<Start>);

optional<string_view> ReadString(SpanU8*,
                                 const Features&,
                                 Errors&,
                                 string_view desc);

optional<Table> Read(SpanU8*, const Features&, Errors&, Tag<Table>);

optional<TableType> Read(SpanU8*, const Features&, Errors&, Tag<TableType>);

optional<TypeEntry> Read(SpanU8*, const Features&, Errors&, Tag<TypeEntry>);

optional<u32> Read(SpanU8*, const Features&, Errors&, Tag<u32>);

optional<u8> Read(SpanU8*, const Features&, Errors&, Tag<u8>);

optional<v128> Read(SpanU8*, const Features&, Errors&, Tag<v128>);

optional<ValueType> Read(SpanU8*, const Features&, Errors&, Tag<ValueType>);

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_H_

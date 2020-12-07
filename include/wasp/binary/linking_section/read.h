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

#ifndef WASP_BINARY_LINKING_SECTION_READ_H_
#define WASP_BINARY_LINKING_SECTION_READ_H_

#include "wasp/binary/linking_section/types.h"
#include "wasp/binary/read.h"

namespace wasp::binary {

struct ReadCtx;

auto Read(SpanU8*, ReadCtx&, Tag<Comdat>) -> OptAt<Comdat>;
auto Read(SpanU8*, ReadCtx&, Tag<ComdatSymbol>) -> OptAt<ComdatSymbol>;
auto Read(SpanU8*, ReadCtx&, Tag<ComdatSymbolKind>) -> OptAt<ComdatSymbolKind>;
auto Read(SpanU8*, ReadCtx&, Tag<InitFunction>) -> OptAt<InitFunction>;
auto Read(SpanU8*, ReadCtx&, Tag<LinkingSubsection>) -> OptAt<LinkingSubsection>;
auto Read(SpanU8*, ReadCtx&, Tag<LinkingSubsectionId>) -> OptAt<LinkingSubsectionId>;
auto Read(SpanU8*, ReadCtx&, Tag<RelocationEntry>) -> OptAt<RelocationEntry>;
auto Read(SpanU8*, ReadCtx&, Tag<RelocationType>) -> OptAt<RelocationType>;
auto Read(SpanU8*, ReadCtx&, Tag<SegmentInfo>) -> OptAt<SegmentInfo>;
auto Read(SpanU8*, ReadCtx&, Tag<SymbolInfo>) -> OptAt<SymbolInfo>;
auto Read(SpanU8*, ReadCtx&, Tag<SymbolInfoKind>) -> OptAt<SymbolInfoKind>;

}  // namespace wasp::binary

#endif  // WASP_BINARY_LINKING_SECTION_READ_H_

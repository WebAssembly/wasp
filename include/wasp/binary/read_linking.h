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

#ifndef WASP_BINARY_READ_LINKING_H_
#define WASP_BINARY_READ_LINKING_H_

#include "wasp/binary/read.h"
#include "wasp/binary/types_linking.h"

namespace wasp {
namespace binary {

struct Context;

OptAt<Comdat> Read(SpanU8*, Context&, Tag<Comdat>);

OptAt<ComdatSymbol> Read(SpanU8*, Context&, Tag<ComdatSymbol>);

OptAt<ComdatSymbolKind> Read(SpanU8*, Context&, Tag<ComdatSymbolKind>);

OptAt<InitFunction> Read(SpanU8*, Context&, Tag<InitFunction>);

OptAt<LinkingSubsection> Read(SpanU8*, Context&, Tag<LinkingSubsection>);

OptAt<LinkingSubsectionId> Read(SpanU8*, Context&, Tag<LinkingSubsectionId>);

OptAt<RelocationEntry> Read(SpanU8*, Context&, Tag<RelocationEntry>);

OptAt<RelocationType> Read(SpanU8*, Context&, Tag<RelocationType>);

OptAt<SegmentInfo> Read(SpanU8*, Context&, Tag<SegmentInfo>);

OptAt<SymbolInfo> Read(SpanU8*, Context&, Tag<SymbolInfo>);

OptAt<SymbolInfoKind> Read(SpanU8*, Context&, Tag<SymbolInfoKind>);

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_LINKING_H_

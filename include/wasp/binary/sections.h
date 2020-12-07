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

#ifndef WASP_BINARY_SECTIONS_H_
#define WASP_BINARY_SECTIONS_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/read.h"
#include "wasp/binary/types.h"

namespace wasp {

class Errors;

namespace binary {

using LazyTypeSection = LazySection<DefinedType>;
using LazyImportSection = LazySection<Import>;
using LazyFunctionSection = LazySection<Function>;
using LazyTableSection = LazySection<Table>;
using LazyMemorySection = LazySection<Memory>;
using LazyGlobalSection = LazySection<Global>;
using LazyEventSection = LazySection<Event>;
using LazyExportSection = LazySection<Export>;
using StartSection = OptAt<Start>;
using LazyElementSection = LazySection<ElementSegment>;
using DataCountSection = OptAt<DataCount>;
using LazyCodeSection = LazySection<Code>;
using LazyDataSection = LazySection<DataSegment>;

auto ReadTypeSection(SpanU8, ReadCtx&) -> LazyTypeSection;
auto ReadTypeSection(KnownSection, ReadCtx&) -> LazyTypeSection;
auto ReadImportSection(SpanU8, ReadCtx&) -> LazyImportSection;
auto ReadImportSection(KnownSection, ReadCtx&) -> LazyImportSection;
auto ReadFunctionSection(SpanU8, ReadCtx&) -> LazyFunctionSection;
auto ReadFunctionSection(KnownSection, ReadCtx&) -> LazyFunctionSection;
auto ReadTableSection(SpanU8, ReadCtx&) -> LazyTableSection;
auto ReadTableSection(KnownSection, ReadCtx&) -> LazyTableSection;
auto ReadMemorySection(SpanU8, ReadCtx&) -> LazyMemorySection;
auto ReadMemorySection(KnownSection, ReadCtx&) -> LazyMemorySection;
auto ReadGlobalSection(SpanU8, ReadCtx&) -> LazyGlobalSection;
auto ReadGlobalSection(KnownSection, ReadCtx&) -> LazyGlobalSection;
auto ReadEventSection(SpanU8, ReadCtx&) -> LazyEventSection;
auto ReadEventSection(KnownSection, ReadCtx&) -> LazyEventSection;
auto ReadExportSection(SpanU8, ReadCtx&) -> LazyExportSection;
auto ReadExportSection(KnownSection, ReadCtx&) -> LazyExportSection;
auto ReadElementSection(SpanU8, ReadCtx&) -> LazyElementSection;
auto ReadElementSection(KnownSection, ReadCtx&) -> LazyElementSection;
auto ReadDataCountSection(SpanU8, ReadCtx&) -> DataCountSection;
auto ReadDataCountSection(KnownSection, ReadCtx&) -> DataCountSection;
auto ReadCodeSection(SpanU8, ReadCtx&) -> LazyCodeSection;
auto ReadCodeSection(KnownSection, ReadCtx&) -> LazyCodeSection;
auto ReadDataSection(SpanU8, ReadCtx&) -> LazyDataSection;
auto ReadDataSection(KnownSection, ReadCtx&) -> LazyDataSection;

inline StartSection ReadStartSection(SpanU8 data, ReadCtx& ctx) {
  SpanU8 copy = data;
  return Read<Start>(&copy, ctx);
}

inline StartSection ReadStartSection(KnownSection sec, ReadCtx& ctx) {
  return ReadStartSection(sec.data, ctx);
}

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SECTIONS_H_

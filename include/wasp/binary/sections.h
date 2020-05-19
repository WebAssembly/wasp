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

using LazyTypeSection = LazySection<TypeEntry>;
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

auto ReadTypeSection(SpanU8, Context&) -> LazyTypeSection;
auto ReadTypeSection(KnownSection, Context&) -> LazyTypeSection;
auto ReadImportSection(SpanU8, Context&) -> LazyImportSection;
auto ReadImportSection(KnownSection, Context&) -> LazyImportSection;
auto ReadFunctionSection(SpanU8, Context&) -> LazyFunctionSection;
auto ReadFunctionSection(KnownSection, Context&) -> LazyFunctionSection;
auto ReadTableSection(SpanU8, Context&) -> LazyTableSection;
auto ReadTableSection(KnownSection, Context&) -> LazyTableSection;
auto ReadMemorySection(SpanU8, Context&) -> LazyMemorySection;
auto ReadMemorySection(KnownSection, Context&) -> LazyMemorySection;
auto ReadGlobalSection(SpanU8, Context&) -> LazyGlobalSection;
auto ReadGlobalSection(KnownSection, Context&) -> LazyGlobalSection;
auto ReadEventSection(SpanU8, Context&) -> LazyEventSection;
auto ReadEventSection(KnownSection, Context&) -> LazyEventSection;
auto ReadExportSection(SpanU8, Context&) -> LazyExportSection;
auto ReadExportSection(KnownSection, Context&) -> LazyExportSection;
auto ReadElementSection(SpanU8, Context&) -> LazyElementSection;
auto ReadElementSection(KnownSection, Context&) -> LazyElementSection;
auto ReadDataCountSection(SpanU8, Context&) -> DataCountSection;
auto ReadDataCountSection(KnownSection, Context&) -> DataCountSection;
auto ReadCodeSection(SpanU8, Context&) -> LazyCodeSection;
auto ReadCodeSection(KnownSection, Context&) -> LazyCodeSection;
auto ReadDataSection(SpanU8, Context&) -> LazyDataSection;
auto ReadDataSection(KnownSection, Context&) -> LazyDataSection;

inline StartSection ReadStartSection(SpanU8 data, Context& context) {
  SpanU8 copy = data;
  return Read<Start>(&copy, context);
}

inline StartSection ReadStartSection(KnownSection sec, Context& context) {
  return ReadStartSection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SECTIONS_H_

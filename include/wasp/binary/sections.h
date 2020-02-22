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
namespace binary {

class Errors;

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

LazyTypeSection ReadTypeSection(SpanU8, Context&);
LazyTypeSection ReadTypeSection(KnownSection, Context&);
LazyImportSection ReadImportSection(SpanU8, Context&);
LazyImportSection ReadImportSection(KnownSection, Context&);
LazyFunctionSection ReadFunctionSection(SpanU8, Context&);
LazyFunctionSection ReadFunctionSection(KnownSection, Context&);
LazyTableSection ReadTableSection(SpanU8, Context&);
LazyTableSection ReadTableSection(KnownSection, Context&);
LazyMemorySection ReadMemorySection(SpanU8, Context&);
LazyMemorySection ReadMemorySection(KnownSection, Context&);
LazyGlobalSection ReadGlobalSection(SpanU8, Context&);
LazyGlobalSection ReadGlobalSection(KnownSection, Context&);
LazyEventSection ReadEventSection(SpanU8, Context&);
LazyEventSection ReadEventSection(KnownSection, Context&);
LazyExportSection ReadExportSection(SpanU8, Context&);
LazyExportSection ReadExportSection(KnownSection, Context&);
LazyElementSection ReadElementSection(SpanU8, Context&);
LazyElementSection ReadElementSection(KnownSection, Context&);
DataCountSection ReadDataCountSection(SpanU8, Context&);
DataCountSection ReadDataCountSection(KnownSection, Context&);
LazyCodeSection ReadCodeSection(SpanU8, Context&);
LazyCodeSection ReadCodeSection(KnownSection, Context&);
LazyDataSection ReadDataSection(SpanU8, Context&);
LazyDataSection ReadDataSection(KnownSection, Context&);

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

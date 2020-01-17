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

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/read.h"
#include "wasp/binary/types.h"

namespace wasp {

class Features;

namespace binary {

class Errors;

using LazyTypeSection = LazySection<TypeEntry>;
using LazyImportSection = LazySection<Import>;
using LazyFunctionSection = LazySection<Function>;
using LazyTableSection = LazySection<Table>;
using LazyMemorySection = LazySection<Memory>;
using LazyGlobalSection = LazySection<Global>;
using LazyExportSection = LazySection<Export>;
using StartSection = optional<Start>;
using LazyElementSection = LazySection<ElementSegment>;
using DataCountSection = optional<DataCount>;
using LazyCodeSection = LazySection<Code>;
using LazyDataSection = LazySection<DataSegment>;

LazyTypeSection ReadTypeSection(SpanU8, const Features&, Errors&);
LazyTypeSection ReadTypeSection(KnownSection, const Features&, Errors&);
LazyImportSection ReadImportSection(SpanU8, const Features&, Errors&);
LazyImportSection ReadImportSection(KnownSection, const Features&, Errors&);
LazyFunctionSection ReadFunctionSection(SpanU8, const Features&, Errors&);
LazyFunctionSection ReadFunctionSection(KnownSection, const Features&, Errors&);
LazyTableSection ReadTableSection(SpanU8, const Features&, Errors&);
LazyTableSection ReadTableSection(KnownSection, const Features&, Errors&);
LazyMemorySection ReadMemorySection(SpanU8, const Features&, Errors&);
LazyMemorySection ReadMemorySection(KnownSection, const Features&, Errors&);
LazyGlobalSection ReadGlobalSection(SpanU8, const Features&, Errors&);
LazyGlobalSection ReadGlobalSection(KnownSection, const Features&, Errors&);
LazyExportSection ReadExportSection(SpanU8, const Features&, Errors&);
LazyExportSection ReadExportSection(KnownSection, const Features&, Errors&);
LazyElementSection ReadElementSection(SpanU8, const Features&, Errors&);
LazyElementSection ReadElementSection(KnownSection, const Features&, Errors&);
DataCountSection ReadDataCountSection(SpanU8, const Features&, Errors&);
DataCountSection ReadDataCountSection(KnownSection, const Features&, Errors&);
LazyCodeSection ReadCodeSection(SpanU8, const Features&, Errors&);
LazyCodeSection ReadCodeSection(KnownSection, const Features&, Errors&);
LazyDataSection ReadDataSection(SpanU8, const Features&, Errors&);
LazyDataSection ReadDataSection(KnownSection, const Features&, Errors&);

inline StartSection ReadStartSection(SpanU8 data,
                                     const Features& features,
                                     Errors& errors) {
  SpanU8 copy = data;
  return Read<Start>(&copy, features, errors);
}

inline StartSection ReadStartSection(KnownSection sec,
                                     const Features& features,
                                     Errors& errors) {
  return ReadStartSection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SECTIONS_H_

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

#ifndef WASP_BINARY_SECTIONS_LINKING_H_
#define WASP_BINARY_SECTIONS_LINKING_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/read_linking.h"
#include "wasp/binary/types.h"
#include "wasp/binary/types_linking.h"

namespace wasp {
namespace binary {

struct RelocationSection {
  explicit RelocationSection(SpanU8, Context&);

  SpanU8 data;
  optional<Index> section_index;
  optional<Index> count;
  LazySequence<RelocationEntry> entries;
};

struct LinkingSection {
  explicit LinkingSection(SpanU8, Context&);

  SpanU8 data;
  optional<u32> version;
  LazySequence<LinkingSubsection> subsections;
};

using LazySegmentInfoSubsection = LazySection<SegmentInfo>;
using LazyInitFunctionsSubsection = LazySection<InitFunction>;
using LazyComdatSubsection = LazySection<Comdat>;
using LazySymbolTableSubsection = LazySection<SymbolInfo>;

RelocationSection ReadRelocationSection(SpanU8 data, Context&);
RelocationSection ReadRelocationSection(CustomSection sec, Context&);

LinkingSection ReadLinkingSection(SpanU8, Context&);
LinkingSection ReadLinkingSection(CustomSection, Context&);

LazySegmentInfoSubsection ReadSegmentInfoSubsection(SpanU8, Context&);
LazySegmentInfoSubsection ReadSegmentInfoSubsection(LinkingSubsection,
                                                    Context&);

LazyInitFunctionsSubsection ReadInitFunctionsSubsection(SpanU8, Context&);
LazyInitFunctionsSubsection ReadInitFunctionsSubsection(LinkingSubsection,
                                                        Context&);

LazyComdatSubsection ReadComdatSubsection(SpanU8, Context&);
LazyComdatSubsection ReadComdatSubsection(LinkingSubsection, Context&);

LazySymbolTableSubsection ReadSymbolTableSubsection(SpanU8, Context&);
LazySymbolTableSubsection ReadSymbolTableSubsection(LinkingSubsection,
                                                    Context&);

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_SECTIONS_LINKING_H_

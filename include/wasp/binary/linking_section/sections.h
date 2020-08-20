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

#ifndef WASP_BINARY_LINKING_SECTION_SECTIONS_H_
#define WASP_BINARY_LINKING_SECTION_SECTIONS_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/linking_section/read.h"
#include "wasp/binary/linking_section/types.h"
#include "wasp/binary/types.h"

namespace wasp::binary {

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

auto ReadRelocationSection(SpanU8 data, Context&) -> RelocationSection;
auto ReadRelocationSection(CustomSection sec, Context&) -> RelocationSection;

auto ReadLinkingSection(SpanU8, Context&) -> LinkingSection;
auto ReadLinkingSection(CustomSection, Context&) -> LinkingSection;

auto ReadSegmentInfoSubsection(SpanU8, Context&) -> LazySegmentInfoSubsection;
auto ReadSegmentInfoSubsection(LinkingSubsection, Context&)
    -> LazySegmentInfoSubsection;

auto ReadInitFunctionsSubsection(SpanU8, Context&)
    -> LazyInitFunctionsSubsection;
auto ReadInitFunctionsSubsection(LinkingSubsection, Context&)
    -> LazyInitFunctionsSubsection;

auto ReadComdatSubsection(SpanU8, Context&) -> LazyComdatSubsection;
auto ReadComdatSubsection(LinkingSubsection, Context&) -> LazyComdatSubsection;

auto ReadSymbolTableSubsection(SpanU8, Context&) -> LazySymbolTableSubsection;
auto ReadSymbolTableSubsection(LinkingSubsection, Context&)
    -> LazySymbolTableSubsection;

}  // namespace wasp::binary

#endif // WASP_BINARY_LINKING_SECTION_SECTIONS_H_

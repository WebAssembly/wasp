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
  explicit RelocationSection(SpanU8, ReadCtx&);

  SpanU8 data;
  optional<Index> section_index;
  optional<Index> count;
  LazySequence<RelocationEntry> entries;
};

struct LinkingSection {
  explicit LinkingSection(SpanU8, ReadCtx&);

  SpanU8 data;
  optional<u32> version;
  LazySequence<LinkingSubsection> subsections;
};

using LazySegmentInfoSubsection = LazySection<SegmentInfo>;
using LazyInitFunctionsSubsection = LazySection<InitFunction>;
using LazyComdatSubsection = LazySection<Comdat>;
using LazySymbolTableSubsection = LazySection<SymbolInfo>;

auto ReadRelocationSection(SpanU8 data, ReadCtx&) -> RelocationSection;
auto ReadRelocationSection(CustomSection sec, ReadCtx&) -> RelocationSection;

auto ReadLinkingSection(SpanU8, ReadCtx&) -> LinkingSection;
auto ReadLinkingSection(CustomSection, ReadCtx&) -> LinkingSection;

auto ReadSegmentInfoSubsection(SpanU8, ReadCtx&) -> LazySegmentInfoSubsection;
auto ReadSegmentInfoSubsection(LinkingSubsection, ReadCtx&)
    -> LazySegmentInfoSubsection;

auto ReadInitFunctionsSubsection(SpanU8, ReadCtx&)
    -> LazyInitFunctionsSubsection;
auto ReadInitFunctionsSubsection(LinkingSubsection, ReadCtx&)
    -> LazyInitFunctionsSubsection;

auto ReadComdatSubsection(SpanU8, ReadCtx&) -> LazyComdatSubsection;
auto ReadComdatSubsection(LinkingSubsection, ReadCtx&) -> LazyComdatSubsection;

auto ReadSymbolTableSubsection(SpanU8, ReadCtx&) -> LazySymbolTableSubsection;
auto ReadSymbolTableSubsection(LinkingSubsection, ReadCtx&)
    -> LazySymbolTableSubsection;

}  // namespace wasp::binary

#endif // WASP_BINARY_LINKING_SECTION_SECTIONS_H_

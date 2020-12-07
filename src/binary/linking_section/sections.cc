//
// Copyright 2019 WebAssembly Community Group participants
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

#include "wasp/binary/linking_section/sections.h"

#include "wasp/base/concat.h"
#include "wasp/base/errors.h"
#include "wasp/binary/formatters.h"

namespace wasp::binary {

LinkingSection::LinkingSection(SpanU8 data, ReadCtx& ctx)
    : data{data},
      version{Read<u32>(&data, ctx)},
      subsections{data, ctx} {
  constexpr u32 kVersion = 2;
  if (version && version != kVersion) {
    ctx.errors.OnError(data, concat("Expected linking section version: ",
                                    kVersion, ", got ", *version));
  }
}

auto ReadLinkingSection(SpanU8 data, ReadCtx& ctx) -> LinkingSection {
  return LinkingSection{data, ctx};
}

auto ReadLinkingSection(CustomSection sec, ReadCtx& ctx) -> LinkingSection {
  return LinkingSection{sec.data, ctx};
}

RelocationSection::RelocationSection(SpanU8 data, ReadCtx& ctx)
    : data{data},
      section_index{Read<u32>(&data, ctx)},
      count{ReadCount(&data, ctx)},
      entries{data, count, "relocation section", ctx} {}

auto ReadRelocationSection(SpanU8 data, ReadCtx& ctx) -> RelocationSection {
  return RelocationSection{data, ctx};
}

auto ReadRelocationSection(CustomSection sec, ReadCtx& ctx)
    -> RelocationSection {
  return RelocationSection{sec.data, ctx};
}

auto ReadComdatSubsection(SpanU8 data, ReadCtx& ctx) -> LazyComdatSubsection {
  return LazyComdatSubsection{data, "comdat subsection", ctx};
}

auto ReadComdatSubsection(LinkingSubsection sec, ReadCtx& ctx)
    -> LazyComdatSubsection {
  return ReadComdatSubsection(sec.data, ctx);
}

auto ReadInitFunctionsSubsection(SpanU8 data, ReadCtx& ctx)
    -> LazyInitFunctionsSubsection {
  return LazyInitFunctionsSubsection{data, "init functions subsection", ctx};
}

auto ReadInitFunctionsSubsection(LinkingSubsection sec, ReadCtx& ctx)
    -> LazyInitFunctionsSubsection {
  return ReadInitFunctionsSubsection(sec.data, ctx);
}

auto ReadSegmentInfoSubsection(SpanU8 data, ReadCtx& ctx)
    -> LazySegmentInfoSubsection {
  return LazySegmentInfoSubsection{data, "segment info subsection", ctx};
}

auto ReadSegmentInfoSubsection(LinkingSubsection sec, ReadCtx& ctx)
    -> LazySegmentInfoSubsection {
  return ReadSegmentInfoSubsection(sec.data, ctx);
}

auto ReadSymbolTableSubsection(SpanU8 data, ReadCtx& ctx)
    -> LazySymbolTableSubsection {
  return LazySymbolTableSubsection{data, "symbol table subsection", ctx};
}

auto ReadSymbolTableSubsection(LinkingSubsection sec, ReadCtx& ctx)
    -> LazySymbolTableSubsection {
  return ReadSymbolTableSubsection(sec.data, ctx);
}

}  // namespace wasp::binary

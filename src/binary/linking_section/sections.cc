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

namespace wasp {
namespace binary {

LinkingSection::LinkingSection(SpanU8 data, Context& context)
    : data{data},
      version{Read<u32>(&data, context)},
      subsections{data, context} {
  constexpr u32 kVersion = 2;
  if (version && version != kVersion) {
    context.errors.OnError(data, concat("Expected linking section version: ",
                                        kVersion, ", got ", *version));
  }
}

auto ReadLinkingSection(SpanU8 data, Context& context) -> LinkingSection {
  return LinkingSection{data, context};
}

auto ReadLinkingSection(CustomSection sec, Context& context) -> LinkingSection {
  return LinkingSection{sec.data, context};
}

RelocationSection::RelocationSection(SpanU8 data, Context& context)
    : data{data},
      section_index{Read<u32>(&data, context)},
      count{ReadCount(&data, context)},
      entries{data, count, "relocation section", context} {}

auto ReadRelocationSection(SpanU8 data, Context& context) -> RelocationSection {
  return RelocationSection{data, context};
}

auto ReadRelocationSection(CustomSection sec, Context& context)
    -> RelocationSection {
  return RelocationSection{sec.data, context};
}

auto ReadComdatSubsection(SpanU8 data, Context& context)
    -> LazyComdatSubsection {
  return LazyComdatSubsection{data, "comdat subsection", context};
}

auto ReadComdatSubsection(LinkingSubsection sec, Context& context)
    -> LazyComdatSubsection {
  return ReadComdatSubsection(sec.data, context);
}

auto ReadInitFunctionsSubsection(SpanU8 data, Context& context)
    -> LazyInitFunctionsSubsection {
  return LazyInitFunctionsSubsection{data, "init functions subsection",
                                     context};
}

auto ReadInitFunctionsSubsection(LinkingSubsection sec, Context& context)
    -> LazyInitFunctionsSubsection {
  return ReadInitFunctionsSubsection(sec.data, context);
}

auto ReadSegmentInfoSubsection(SpanU8 data, Context& context)
    -> LazySegmentInfoSubsection {
  return LazySegmentInfoSubsection{data, "segment info subsection", context};
}

auto ReadSegmentInfoSubsection(LinkingSubsection sec, Context& context)
    -> LazySegmentInfoSubsection {
  return ReadSegmentInfoSubsection(sec.data, context);
}

auto ReadSymbolTableSubsection(SpanU8 data, Context& context)
    -> LazySymbolTableSubsection {
  return LazySymbolTableSubsection{data, "symbol table subsection", context};
}

auto ReadSymbolTableSubsection(LinkingSubsection sec, Context& context)
    -> LazySymbolTableSubsection {
  return ReadSymbolTableSubsection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

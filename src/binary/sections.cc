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

#include "wasp/binary/errors.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/linking_section/sections.h"
#include "wasp/binary/name_section/sections.h"
#include "wasp/binary/sections.h"

namespace wasp {
namespace binary {

LazyCodeSection ReadCodeSection(SpanU8 data, Context& context) {
  return LazyCodeSection{data, "code section", context};
}

LazyCodeSection ReadCodeSection(KnownSection sec, Context& context) {
  return ReadCodeSection(sec.data, context);
}

LazyDataSection ReadDataSection(SpanU8 data, Context& context) {
  return LazyDataSection{data, "data section", context};
}

LazyDataSection ReadDataSection(KnownSection sec, Context& context) {
  return ReadDataSection(sec.data, context);
}

DataCountSection ReadDataCountSection(SpanU8 data, Context& context) {
  SpanU8 copy = data;
  return Read<DataCount>(&copy, context);
}

DataCountSection ReadDataCountSection(KnownSection sec, Context& context) {
  return ReadDataCountSection(sec.data, context);
}

LazyElementSection ReadElementSection(SpanU8 data, Context& context) {
  return LazyElementSection{data, "element section", context};
}

LazyElementSection ReadElementSection(KnownSection sec, Context& context) {
  return ReadElementSection(sec.data, context);
}

LazyEventSection ReadEventSection(SpanU8 data, Context& context) {
  return LazyEventSection{data, "event section", context};
}

LazyEventSection ReadEventSection(KnownSection sec, Context& context) {
  return ReadEventSection(sec.data, context);
}

LazyExportSection ReadExportSection(SpanU8 data, Context& context) {
  return LazyExportSection{data, "export section", context};
}

LazyExportSection ReadExportSection(KnownSection sec, Context& context) {
  return ReadExportSection(sec.data, context);
}

LazyFunctionSection ReadFunctionSection(SpanU8 data, Context& context) {
  return LazyFunctionSection{data, "function section", context};
}

LazyFunctionSection ReadFunctionSection(KnownSection sec, Context& context) {
  return ReadFunctionSection(sec.data, context);
}

LazyGlobalSection ReadGlobalSection(SpanU8 data, Context& context) {
  return LazyGlobalSection{data, "global section", context};
}

LazyGlobalSection ReadGlobalSection(KnownSection sec, Context& context) {
  return ReadGlobalSection(sec.data, context);
}

LazyImportSection ReadImportSection(SpanU8 data, Context& context) {
  return LazyImportSection{data, "import section", context};
}

LazyImportSection ReadImportSection(KnownSection sec, Context& context) {
  return ReadImportSection(sec.data, context);
}

LinkingSection::LinkingSection(SpanU8 data, Context& context)
    : data{data},
      version{Read<u32>(&data, context)},
      subsections{data, context} {
  constexpr u32 kVersion = 2;
  if (version && version != kVersion) {
    context.errors.OnError(
        data, format("Expected linking section version: {}, got {}", kVersion,
                     *version));
  }
}

LinkingSection ReadLinkingSection(SpanU8 data, Context& context) {
  return LinkingSection{data, context};
}

LinkingSection ReadLinkingSection(CustomSection sec, Context& context) {
  return LinkingSection{sec.data, context};
}

LazyMemorySection ReadMemorySection(SpanU8 data, Context& context) {
  return LazyMemorySection{data, "memory section", context};
}

LazyMemorySection ReadMemorySection(KnownSection sec, Context& context) {
  return ReadMemorySection(sec.data, context);
}

LazyNameSection ReadNameSection(SpanU8 data, Context& context) {
  return LazyNameSection{data, context};
}

LazyNameSection ReadNameSection(CustomSection sec, Context& context) {
  return LazyNameSection{sec.data, context};
}

RelocationSection::RelocationSection(SpanU8 data, Context& context)
    : data{data},
      section_index{Read<u32>(&data, context)},
      count{ReadCount(&data, context)},
      entries{data, count, "relocation section", context} {}

RelocationSection ReadRelocationSection(SpanU8 data, Context& context) {
  return RelocationSection{data, context};
}

RelocationSection ReadRelocationSection(CustomSection sec, Context& context) {
  return RelocationSection{sec.data, context};
}

LazyTableSection ReadTableSection(SpanU8 data, Context& context) {
  return LazyTableSection{data, "table section", context};
}

LazyTableSection ReadTableSection(KnownSection sec, Context& context) {
  return ReadTableSection(sec.data, context);
}

LazyTypeSection ReadTypeSection(SpanU8 data, Context& context) {
  return LazyTypeSection{data, "type section", context};
}

LazyTypeSection ReadTypeSection(KnownSection sec, Context& context) {
  return ReadTypeSection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

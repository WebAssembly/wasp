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

#include "wasp/binary/sections.h"

#include "wasp/base/errors.h"
#include "wasp/binary/formatters.h"

namespace wasp {
namespace binary {

auto ReadCodeSection(SpanU8 data, Context& context) -> LazyCodeSection {
  return LazyCodeSection{data, "code section", context};
}

auto ReadCodeSection(KnownSection sec, Context& context) -> LazyCodeSection {
  return ReadCodeSection(sec.data, context);
}

auto ReadDataSection(SpanU8 data, Context& context) -> LazyDataSection {
  return LazyDataSection{data, "data section", context};
}

auto ReadDataSection(KnownSection sec, Context& context) -> LazyDataSection {
  return ReadDataSection(sec.data, context);
}

auto ReadDataCountSection(SpanU8 data, Context& context) -> DataCountSection {
  SpanU8 copy = data;
  return Read<DataCount>(&copy, context);
}

auto ReadDataCountSection(KnownSection sec, Context& context)
    -> DataCountSection {
  return ReadDataCountSection(sec.data, context);
}

auto ReadElementSection(SpanU8 data, Context& context) -> LazyElementSection {
  return LazyElementSection{data, "element section", context};
}

auto ReadElementSection(KnownSection sec, Context& context)
    -> LazyElementSection {
  return ReadElementSection(sec.data, context);
}

auto ReadEventSection(SpanU8 data, Context& context) -> LazyEventSection {
  return LazyEventSection{data, "event section", context};
}

auto ReadEventSection(KnownSection sec, Context& context) -> LazyEventSection {
  return ReadEventSection(sec.data, context);
}

auto ReadExportSection(SpanU8 data, Context& context) -> LazyExportSection {
  return LazyExportSection{data, "export section", context};
}

auto ReadExportSection(KnownSection sec, Context& context)
    -> LazyExportSection {
  return ReadExportSection(sec.data, context);
}

auto ReadFunctionSection(SpanU8 data, Context& context) -> LazyFunctionSection {
  return LazyFunctionSection{data, "function section", context};
}

auto ReadFunctionSection(KnownSection sec, Context& context)
    -> LazyFunctionSection {
  return ReadFunctionSection(sec.data, context);
}

auto ReadGlobalSection(SpanU8 data, Context& context) -> LazyGlobalSection {
  return LazyGlobalSection{data, "global section", context};
}

auto ReadGlobalSection(KnownSection sec, Context& context)
    -> LazyGlobalSection {
  return ReadGlobalSection(sec.data, context);
}

auto ReadImportSection(SpanU8 data, Context& context) -> LazyImportSection {
  return LazyImportSection{data, "import section", context};
}

auto ReadImportSection(KnownSection sec, Context& context)
    -> LazyImportSection {
  return ReadImportSection(sec.data, context);
}

auto ReadMemorySection(SpanU8 data, Context& context) -> LazyMemorySection {
  return LazyMemorySection{data, "memory section", context};
}

auto ReadMemorySection(KnownSection sec, Context& context)
    -> LazyMemorySection {
  return ReadMemorySection(sec.data, context);
}

auto ReadTableSection(SpanU8 data, Context& context) -> LazyTableSection {
  return LazyTableSection{data, "table section", context};
}

auto ReadTableSection(KnownSection sec, Context& context) -> LazyTableSection {
  return ReadTableSection(sec.data, context);
}

auto ReadTypeSection(SpanU8 data, Context& context) -> LazyTypeSection {
  return LazyTypeSection{data, "type section", context};
}

auto ReadTypeSection(KnownSection sec, Context& context) -> LazyTypeSection {
  return ReadTypeSection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

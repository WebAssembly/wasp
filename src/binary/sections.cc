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

namespace wasp::binary {

auto ReadCodeSection(SpanU8 data, ReadCtx& ctx) -> LazyCodeSection {
  return LazyCodeSection{data, "code section", ctx};
}

auto ReadCodeSection(KnownSection sec, ReadCtx& ctx) -> LazyCodeSection {
  return ReadCodeSection(sec.data, ctx);
}

auto ReadDataSection(SpanU8 data, ReadCtx& ctx) -> LazyDataSection {
  return LazyDataSection{data, "data section", ctx};
}

auto ReadDataSection(KnownSection sec, ReadCtx& ctx) -> LazyDataSection {
  return ReadDataSection(sec.data, ctx);
}

auto ReadDataCountSection(SpanU8 data, ReadCtx& ctx) -> DataCountSection {
  SpanU8 copy = data;
  return Read<DataCount>(&copy, ctx);
}

auto ReadDataCountSection(KnownSection sec, ReadCtx& ctx) -> DataCountSection {
  return ReadDataCountSection(sec.data, ctx);
}

auto ReadElementSection(SpanU8 data, ReadCtx& ctx) -> LazyElementSection {
  return LazyElementSection{data, "element section", ctx};
}

auto ReadElementSection(KnownSection sec, ReadCtx& ctx) -> LazyElementSection {
  return ReadElementSection(sec.data, ctx);
}

auto ReadTagSection(SpanU8 data, ReadCtx& ctx) -> LazyTagSection {
  return LazyTagSection{data, "tag section", ctx};
}

auto ReadTagSection(KnownSection sec, ReadCtx& ctx) -> LazyTagSection {
  return ReadTagSection(sec.data, ctx);
}

auto ReadExportSection(SpanU8 data, ReadCtx& ctx) -> LazyExportSection {
  return LazyExportSection{data, "export section", ctx};
}

auto ReadExportSection(KnownSection sec, ReadCtx& ctx) -> LazyExportSection {
  return ReadExportSection(sec.data, ctx);
}

auto ReadFunctionSection(SpanU8 data, ReadCtx& ctx) -> LazyFunctionSection {
  return LazyFunctionSection{data, "function section", ctx};
}

auto ReadFunctionSection(KnownSection sec, ReadCtx& ctx)
    -> LazyFunctionSection {
  return ReadFunctionSection(sec.data, ctx);
}

auto ReadGlobalSection(SpanU8 data, ReadCtx& ctx) -> LazyGlobalSection {
  return LazyGlobalSection{data, "global section", ctx};
}

auto ReadGlobalSection(KnownSection sec, ReadCtx& ctx) -> LazyGlobalSection {
  return ReadGlobalSection(sec.data, ctx);
}

auto ReadImportSection(SpanU8 data, ReadCtx& ctx) -> LazyImportSection {
  return LazyImportSection{data, "import section", ctx};
}

auto ReadImportSection(KnownSection sec, ReadCtx& ctx) -> LazyImportSection {
  return ReadImportSection(sec.data, ctx);
}

auto ReadMemorySection(SpanU8 data, ReadCtx& ctx) -> LazyMemorySection {
  return LazyMemorySection{data, "memory section", ctx};
}

auto ReadMemorySection(KnownSection sec, ReadCtx& ctx) -> LazyMemorySection {
  return ReadMemorySection(sec.data, ctx);
}

auto ReadTableSection(SpanU8 data, ReadCtx& ctx) -> LazyTableSection {
  return LazyTableSection{data, "table section", ctx};
}

auto ReadTableSection(KnownSection sec, ReadCtx& ctx) -> LazyTableSection {
  return ReadTableSection(sec.data, ctx);
}

auto ReadTypeSection(SpanU8 data, ReadCtx& ctx) -> LazyTypeSection {
  return LazyTypeSection{data, "type section", ctx};
}

auto ReadTypeSection(KnownSection sec, ReadCtx& ctx) -> LazyTypeSection {
  return ReadTypeSection(sec.data, ctx);
}

}  // namespace wasp::binary

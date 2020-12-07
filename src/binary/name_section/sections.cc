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

#include "wasp/binary/name_section/sections.h"

namespace wasp::binary {

auto ReadNameSection(SpanU8 data, ReadCtx& ctx) -> LazyNameSection {
  return LazyNameSection{data, ctx};
}

auto ReadNameSection(CustomSection sec, ReadCtx& ctx) -> LazyNameSection {
  return LazyNameSection{sec.data, ctx};
}

auto ReadFunctionNamesSubsection(SpanU8 data, ReadCtx& ctx)
    -> LazyFunctionNamesSubsection {
  return LazyFunctionNamesSubsection{data, "function names subsection", ctx};
}

auto ReadFunctionNamesSubsection(NameSubsection sec, ReadCtx& ctx)
    -> LazyFunctionNamesSubsection {
  return ReadFunctionNamesSubsection(sec.data, ctx);
}

auto ReadLocalNamesSubsection(SpanU8 data, ReadCtx& ctx)
    -> LazyLocalNamesSubsection {
  return LazyLocalNamesSubsection{data, "local names subsection", ctx};
}

auto ReadLocalNamesSubsection(NameSubsection sec, ReadCtx& ctx)
    -> LazyLocalNamesSubsection {
  return ReadLocalNamesSubsection(sec.data, ctx);
}

auto ReadModuleNameSubsection(SpanU8 data, ReadCtx& ctx)
    -> ModuleNameSubsection {
  return ReadString(&data, ctx, "module name");
}

auto ReadModuleNameSubsection(NameSubsection sec, ReadCtx& ctx)
    -> ModuleNameSubsection {
  return ReadModuleNameSubsection(sec.data, ctx);
}

}  // namespace wasp::binary

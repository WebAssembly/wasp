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

namespace wasp {
namespace binary {

auto ReadNameSection(SpanU8 data, Context& context) -> LazyNameSection {
  return LazyNameSection{data, context};
}

auto ReadNameSection(CustomSection sec, Context& context) -> LazyNameSection {
  return LazyNameSection{sec.data, context};
}

auto ReadFunctionNamesSubsection(SpanU8 data, Context& context)
    -> LazyFunctionNamesSubsection {
  return LazyFunctionNamesSubsection{data, "function names subsection",
                                     context};
}

auto ReadFunctionNamesSubsection(NameSubsection sec, Context& context)
    -> LazyFunctionNamesSubsection {
  return ReadFunctionNamesSubsection(sec.data, context);
}

auto ReadLocalNamesSubsection(SpanU8 data, Context& context)
    -> LazyLocalNamesSubsection {
  return LazyLocalNamesSubsection{data, "local names subsection", context};
}

auto ReadLocalNamesSubsection(NameSubsection sec, Context& context)
    -> LazyLocalNamesSubsection {
  return ReadLocalNamesSubsection(sec.data, context);
}

auto ReadModuleNameSubsection(SpanU8 data, Context& context)
    -> ModuleNameSubsection {
  return ReadString(&data, context, "module name");
}

auto ReadModuleNameSubsection(NameSubsection sec, Context& context)
    -> ModuleNameSubsection {
  return ReadModuleNameSubsection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

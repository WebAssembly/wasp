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

#include "wasp/binary/sections_name.h"

namespace wasp {
namespace binary {

LazyFunctionNamesSubsection ReadFunctionNamesSubsection(SpanU8 data,
                                                        Context& context) {
  return LazyFunctionNamesSubsection{data, "function names subsection",
                                     context};
}

LazyFunctionNamesSubsection ReadFunctionNamesSubsection(NameSubsection sec,
                                                        Context& context) {
  return ReadFunctionNamesSubsection(sec.data, context);
}

LazyLocalNamesSubsection ReadLocalNamesSubsection(SpanU8 data,
                                                  Context& context) {
  return LazyLocalNamesSubsection{data, "local names subsection", context};
}

LazyLocalNamesSubsection ReadLocalNamesSubsection(NameSubsection sec,
                                                  Context& context) {
  return ReadLocalNamesSubsection(sec.data, context);
}

ModuleNameSubsection ReadModuleNameSubsection(SpanU8 data, Context& context) {
  return ReadString(&data, context, "module name");
}

ModuleNameSubsection ReadModuleNameSubsection(NameSubsection sec,
                                              Context& context) {
  return ReadModuleNameSubsection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

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

LazyFunctionNamesSubsection ReadFunctionNamesSubsection(
    SpanU8 data,
    const Features& features,
    Errors& errors) {
  return LazyFunctionNamesSubsection{data, "function names subsection",
                                     features, errors};
}

LazyFunctionNamesSubsection ReadFunctionNamesSubsection(
    NameSubsection sec,
    const Features& features,
    Errors& errors) {
  return ReadFunctionNamesSubsection(sec.data, features, errors);
}

LazyLocalNamesSubsection ReadLocalNamesSubsection(SpanU8 data,
                                                  const Features& features,
                                                  Errors& errors) {
  return LazyLocalNamesSubsection{data, "local names subsection", features,
                                  errors};
}

LazyLocalNamesSubsection ReadLocalNamesSubsection(NameSubsection sec,
                                                  const Features& features,
                                                  Errors& errors) {
  return ReadLocalNamesSubsection(sec.data, features, errors);
}

ModuleNameSubsection ReadModuleNameSubsection(SpanU8 data,
                                              const Features& features,
                                              Errors& errors) {
  return ReadString(&data, features, errors, "module name");
}

ModuleNameSubsection ReadModuleNameSubsection(NameSubsection sec,
                                              const Features& features,
                                              Errors& errors) {
  return ReadModuleNameSubsection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

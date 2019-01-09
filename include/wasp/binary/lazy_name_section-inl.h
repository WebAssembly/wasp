//
// Copyright 2018 WebAssembly Community Group participants
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

#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_indirect_name_assoc.h"
#include "wasp/binary/read/read_name_assoc.h"
#include "wasp/binary/read/read_name_subsection.h"
#include "wasp/binary/read/read_string.h"

namespace wasp {
namespace binary {

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(SpanU8 data,
                                        const Features& features,
                                        Errors& errors) {
  return LazyNameSection<Errors>{data, features, errors};
}

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(CustomSection sec,
                                        const Features& features,
                                        Errors& errors) {
  return LazyNameSection<Errors>{sec.data, features, errors};
}

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(SpanU8 data,
                                              const Features& features,
                                              Errors& errors) {
  return ReadString(&data, features, errors, "module name");
}

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(NameSubsection sec,
                                              const Features& features,
                                              Errors& errors) {
  return ReadModuleNameSubsection(sec.data, features, errors);
}

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(
    SpanU8 data,
    const Features& features,
    Errors& errors) {
  return LazyFunctionNamesSubsection<Errors>{data, features, errors};
}

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(
    NameSubsection sec,
    const Features& features,
    Errors& errors) {
  return ReadFunctionNamesSubsection(sec.data, features, errors);
}

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(
    SpanU8 data,
    const Features& features,
    Errors& errors) {
  return LazyLocalNamesSubsection<Errors>{data, features, errors};
}

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(
    NameSubsection sec,
    const Features& features,
    Errors& errors) {
  return ReadLocalNamesSubsection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

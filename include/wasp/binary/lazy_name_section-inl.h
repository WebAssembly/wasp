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

#include "wasp/binary/reader.h"

namespace wasp {
namespace binary {

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(SpanU8 data, Errors& errors) {
  return LazyNameSection<Errors>{data, errors};
}

template <typename Errors>
LazyNameSection<Errors> ReadNameSection(CustomSection sec, Errors& errors) {
  return LazyNameSection<Errors>{sec.data, errors};
}

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(SpanU8 data, Errors& errors) {
  return ReadString(&data, errors, "module name");
}

template <typename Errors>
ModuleNameSubsection ReadModuleNameSubsection(NameSubsection sec,
                                              Errors& errors) {
  return ReadModuleNameSubsection(sec.data, errors);
}

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(
    SpanU8 data,
    Errors& errors) {
  return LazyFunctionNamesSubsection<Errors>{data, errors};
}

template <typename Errors>
LazyFunctionNamesSubsection<Errors> ReadFunctionNamesSubsection(
    NameSubsection sec,
    Errors& errors) {
  return ReadFunctionNamesSubsection(sec.data, errors);
}

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(SpanU8 data,
                                                          Errors& errors) {
  return LazyLocalNamesSubsection<Errors>{data, errors};
}

template <typename Errors>
LazyLocalNamesSubsection<Errors> ReadLocalNamesSubsection(NameSubsection sec,
                                                          Errors& errors) {
  return ReadLocalNamesSubsection(sec.data, errors);
}

}  // namespace binary
}  // namespace wasp

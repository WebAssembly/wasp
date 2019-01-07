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

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(SpanU8 data, Errors& errors)
    : count(ReadCount(&data, errors)), sequence(data, errors) {}

#define WASP_DEFINE_LAZY_READ_FUNC(SectionType, ReadFunc)          \
  template <typename Errors>                                       \
  SectionType<Errors> ReadFunc(SpanU8 data, Errors& errors) {      \
    return SectionType<Errors>{data, errors};                      \
  }                                                                \
  template <typename Errors>                                       \
  SectionType<Errors> ReadFunc(KnownSection sec, Errors& errors) { \
    return ReadFunc(sec.data, errors);                             \
  }

WASP_DEFINE_LAZY_READ_FUNC(LazyTypeSection, ReadTypeSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyImportSection, ReadImportSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyFunctionSection, ReadFunctionSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyTableSection, ReadTableSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyMemorySection, ReadMemorySection)
WASP_DEFINE_LAZY_READ_FUNC(LazyGlobalSection, ReadGlobalSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyExportSection, ReadExportSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyElementSection, ReadElementSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyCodeSection, ReadCodeSection)
WASP_DEFINE_LAZY_READ_FUNC(LazyDataSection, ReadDataSection)

#undef WASP_DEFINE_LAZY_READ_FUNC

template <typename Errors>
StartSection ReadStartSection(SpanU8 data, Errors& errors) {
  SpanU8 copy = data;
  return Read<Start>(&copy, errors);
}

template <typename Errors>
StartSection ReadStartSection(KnownSection sec, Errors& errors) {
  return ReadStartSection(sec.data, errors);
}

}  // namespace binary
}  // namespace wasp

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

#include "wasp/base/features.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_code.h"
#include "wasp/binary/read/read_count.h"
#include "wasp/binary/read/read_data_segment.h"
#include "wasp/binary/read/read_element_segment.h"
#include "wasp/binary/read/read_export.h"
#include "wasp/binary/read/read_function.h"
#include "wasp/binary/read/read_global.h"
#include "wasp/binary/read/read_import.h"
#include "wasp/binary/read/read_memory.h"
#include "wasp/binary/read/read_start.h"
#include "wasp/binary/read/read_table.h"
#include "wasp/binary/read/read_type_entry.h"

namespace wasp {
namespace binary {

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(SpanU8 data,
                                    const Features& features,
                                    Errors& errors)
    : count{ReadCount(&data, features, errors)},
      sequence{data, features, errors} {}

#define WASP_DEFINE_LAZY_READ_FUNC(SectionType, ReadFunc)                  \
  template <typename Errors>                                               \
  SectionType<Errors> ReadFunc(SpanU8 data, const Features& features,      \
                               Errors& errors) {                           \
    return SectionType<Errors>{data, features, errors};                    \
  }                                                                        \
  template <typename Errors>                                               \
  SectionType<Errors> ReadFunc(KnownSection sec, const Features& features, \
                               Errors& errors) {                           \
    return ReadFunc(sec.data, features, errors);                           \
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
StartSection ReadStartSection(SpanU8 data,
                              const Features& features,
                              Errors& errors) {
  SpanU8 copy = data;
  return Read<Start>(&copy, features, errors);
}

template <typename Errors>
StartSection ReadStartSection(KnownSection sec,
                              const Features& features,
                              Errors& errors) {
  return ReadStartSection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

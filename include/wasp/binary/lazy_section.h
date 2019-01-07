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

#ifndef WASP_BINARY_LAZY_SECTION_H_
#define WASP_BINARY_LAZY_SECTION_H_

#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/lazy_sequence.h"
#include "wasp/binary/section.h"
#include "wasp/binary/types.h"

namespace wasp {
namespace binary {

/// ---
template <typename T, typename Errors>
class LazySection {
 public:
  explicit LazySection(SpanU8, Errors&);

  optional<Index> count;
  LazySequence<T, Errors> sequence;
};

#define WASP_DECLARE_LAZY_KNOWN_SECTION(SectionType, ElementType, ReadFunc) \
  template <typename Errors>                                                \
  using SectionType = LazySection<ElementType, Errors>;                     \
                                                                            \
  template <typename Errors>                                                \
  SectionType<Errors> ReadFunc(SpanU8, Errors&);                            \
  template <typename Errors>                                                \
  SectionType<Errors> ReadFunc(KnownSection, Errors&);

WASP_DECLARE_LAZY_KNOWN_SECTION(LazyTypeSection, TypeEntry, ReadTypeSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyImportSection, Import, ReadImportSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyFunctionSection,
                                Function,
                                ReadFunctionSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyTableSection, Table, ReadTableSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyMemorySection, Memory, ReadMemorySection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyGlobalSection, Global, ReadGlobalSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyExportSection, Export, ReadExportSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyElementSection,
                                ElementSegment,
                                ReadElementSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyCodeSection, Code, ReadCodeSection)
WASP_DECLARE_LAZY_KNOWN_SECTION(LazyDataSection, DataSegment, ReadDataSection)

#undef WASP_DECLARE_LAZY_KNOWN_SECTION

/// ---
using StartSection = optional<Start>;

template <typename Errors>
StartSection ReadStartSection(SpanU8, Errors&);
template <typename Errors>
StartSection ReadStartSection(KnownSection, Errors&);


}  // namespace binary
}  // namespace wasp

#include "wasp/binary/lazy_section-inl.h"

#endif // WASP_BINARY_LAZY_SECTION_H_

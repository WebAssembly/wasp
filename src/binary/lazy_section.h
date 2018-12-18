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

#include "src/base/types.h"
#include "src/binary/types.h"
#include "src/binary/lazy_sequence.h"

namespace wasp {
namespace binary {

/// ---
template <typename T, typename Errors>
class LazySection {
 public:
  explicit LazySection(SpanU8, Errors&);
  explicit LazySection(KnownSection, Errors&);
  explicit LazySection(CustomSection, Errors&);

  optional<Index> count;
  LazySequence<T, Errors> sequence;
};

/// ---
template <typename Errors>
using LazyTypeSection = LazySection<TypeEntry, Errors>;

template <typename Data, typename Errors>
LazyTypeSection<Errors> ReadTypeSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyImportSection = LazySection<Import, Errors>;

template <typename Data, typename Errors>
LazyImportSection<Errors> ReadImportSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyFunctionSection = LazySection<Function, Errors>;

template <typename Data, typename Errors>
LazyFunctionSection<Errors> ReadFunctionSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyTableSection = LazySection<Table, Errors>;

template <typename Data, typename Errors>
LazyTableSection<Errors> ReadTableSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyMemorySection = LazySection<Memory, Errors>;

template <typename Data, typename Errors>
LazyMemorySection<Errors> ReadMemorySection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyGlobalSection = LazySection<Global, Errors>;

template <typename Data, typename Errors>
LazyGlobalSection<Errors> ReadGlobalSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyExportSection = LazySection<Export, Errors>;

template <typename Data, typename Errors>
LazyExportSection<Errors> ReadExportSection(Data&&, Errors&);

/// ---
using StartSection = optional<Start>;

template <typename Errors>
StartSection ReadStartSection(SpanU8, Errors&);
template <typename Errors>
StartSection ReadStartSection(KnownSection, Errors&);

/// ---
template <typename Errors>
using LazyElementSection = LazySection<ElementSegment, Errors>;

template <typename Data, typename Errors>
LazyElementSection<Errors> ReadElementSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyCodeSection = LazySection<Code, Errors>;

template <typename Data, typename Errors>
LazyCodeSection<Errors> ReadCodeSection(Data&&, Errors&);

/// ---
template <typename Errors>
using LazyDataSection = LazySection<DataSegment, Errors>;

template <typename Data, typename Errors>
LazyDataSection<Errors> ReadDataSection(Data&&, Errors&);

}  // namespace binary
}  // namespace wasp

#include "src/binary/lazy_section-inl.h"

#endif // WASP_BINARY_LAZY_SECTION_H_

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

#ifndef WASP_BINARY_LAZY_SECTION_H
#define WASP_BINARY_LAZY_SECTION_H

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
  explicit LazySection(KnownSection<>, Errors&);

  optional<Index> count;
  LazySequence<T, Errors> sequence;
};

/// ---
template <typename Errors>
using LazyTypeSection = LazySection<TypeEntry, Errors>;

template <typename Data, typename Errors>
LazyTypeSection<Errors> ReadTypeSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyImportSection = LazySection<Import<>, Errors>;

template <typename Data, typename Errors>
LazyImportSection<Errors> ReadImportSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyFunctionSection = LazySection<Function, Errors>;

template <typename Data, typename Errors>
LazyFunctionSection<Errors> ReadFunctionSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyTableSection = LazySection<Table, Errors>;

template <typename Data, typename Errors>
LazyTableSection<Errors> ReadTableSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyMemorySection = LazySection<Memory, Errors>;

template <typename Data, typename Errors>
LazyMemorySection<Errors> ReadMemorySection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyGlobalSection = LazySection<Global<>, Errors>;

template <typename Data, typename Errors>
LazyGlobalSection<Errors> ReadGlobalSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyExportSection = LazySection<Export<>, Errors>;

template <typename Data, typename Errors>
LazyExportSection<Errors> ReadExportSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
struct StartSection {
  explicit StartSection(SpanU8, Errors&);
  explicit StartSection(KnownSection<>, Errors&);

  optional<Start> start();

 private:
  Errors& errors_;
  optional<Start> start_;
};

template <typename Data, typename Errors>
StartSection<Errors> ReadStartSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyElementSection = LazySection<ElementSegment<>, Errors>;

template <typename Data, typename Errors>
LazyElementSection<Errors> ReadElementSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyCodeSection = LazySection<Code<>, Errors>;

template <typename Data, typename Errors>
LazyCodeSection<Errors> ReadCodeSection(Data&& data, Errors& errors);

/// ---
template <typename Errors>
using LazyDataSection = LazySection<DataSegment<>, Errors>;

template <typename Data, typename Errors>
LazyDataSection<Errors> ReadDataSection(Data&& data, Errors& errors);

}  // namespace binary
}  // namespace wasp

#include "src/binary/lazy_section-inl.h"

#endif // WASP_BINARY_LAZY_SECTION_H

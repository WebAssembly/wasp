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

#include "src/binary/reader.h"

namespace wasp {
namespace binary {

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(SpanU8 data, Errors& errors)
    : count(ReadCount(&data, errors)), sequence(data, errors) {}

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(KnownSection section, Errors& errors)
    : LazySection(section.data, errors) {}

template <typename T, typename Errors>
LazySection<T, Errors>::LazySection(CustomSection section, Errors& errors)
    : LazySection(section.data, errors) {}

template <typename Data, typename Errors>
LazyTypeSection<Errors> ReadTypeSection(Data&& data, Errors& errors) {
  return LazyTypeSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyImportSection<Errors> ReadImportSection(Data&& data, Errors& errors) {
  return LazyImportSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyFunctionSection<Errors> ReadFunctionSection(Data&& data, Errors& errors) {
  return LazyFunctionSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyTableSection<Errors> ReadTableSection(Data&& data, Errors& errors) {
  return LazyTableSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyMemorySection<Errors> ReadMemorySection(Data&& data, Errors& errors) {
  return LazyMemorySection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyGlobalSection<Errors> ReadGlobalSection(Data&& data, Errors& errors) {
  return LazyGlobalSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyExportSection<Errors> ReadExportSection(Data&& data, Errors& errors) {
  return LazyExportSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyElementSection<Errors> ReadElementSection(Data&& data, Errors& errors) {
  return LazyElementSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Errors>
StartSection ReadStartSection(SpanU8 data, Errors& errors) {
  SpanU8 copy = data;
  return Read<Start>(&copy, errors);
}

template <typename Errors>
StartSection ReadStartSection(KnownSection known, Errors& errors) {
  SpanU8 copy = known.data;
  return Read<Start>(&copy, errors);
}

template <typename Data, typename Errors>
LazyCodeSection<Errors> ReadCodeSection(Data&& data, Errors& errors) {
  return LazyCodeSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyDataSection<Errors> ReadDataSection(Data&& data, Errors& errors) {
  return LazyDataSection<Errors>{std::forward<Data>(data), errors};
}

}  // namespace binary
}  // namespace wasp

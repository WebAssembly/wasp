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

#ifndef WASP_BINARY_LAZY_EXPORT_SECTION_H_
#define WASP_BINARY_LAZY_EXPORT_SECTION_H_

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/binary/export.h"
#include "wasp/binary/known_section.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/read/read_export.h"

namespace wasp {
namespace binary {

template <typename Errors>
using LazyExportSection = LazySection<Export, Errors>;

template <typename Errors>
LazyExportSection<Errors> ReadExportSection(SpanU8 data,
                                            const Features& features,
                                            Errors& errors) {
  return LazyExportSection<Errors>{data, features, errors};
}

template <typename Errors>
LazyExportSection<Errors> ReadExportSection(KnownSection sec,
                                            const Features& features,
                                            Errors& errors) {
  return ReadExportSection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_LAZY_EXPORT_SECTION_H_

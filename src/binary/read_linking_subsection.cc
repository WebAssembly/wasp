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

#include "wasp/binary/lazy_comdat_subsection.h"
#include "wasp/binary/lazy_segment_info_subsection.h"
#include "wasp/binary/lazy_symbol_table_subsection.h"

namespace wasp {
namespace binary {

LazyComdatSubsection ReadComdatSubsection(SpanU8 data,
                                          const Features& features,
                                          Errors& errors) {
  return LazyComdatSubsection{data, features, errors};
}

LazyComdatSubsection ReadComdatSubsection(LinkingSubsection sec,
                                          const Features& features,
                                          Errors& errors) {
  return LazyComdatSubsection{sec.data, features, errors};
}

LazySegmentInfoSubsection ReadSegmentInfoSubsection(SpanU8 data,
                                                    const Features& features,
                                                    Errors& errors) {
  return LazySegmentInfoSubsection{data, features, errors};
}

LazySegmentInfoSubsection ReadSegmentInfoSubsection(LinkingSubsection sec,
                                                    const Features& features,
                                                    Errors& errors) {
  return LazySegmentInfoSubsection{sec.data, features, errors};
}

LazySymbolTableSubsection ReadSymbolTableSubsection(SpanU8 data,
                                                    const Features& features,
                                                    Errors& errors) {
  return LazySymbolTableSubsection{data, features, errors};
}

LazySymbolTableSubsection ReadSymbolTableSubsection(LinkingSubsection sec,
                                                    const Features& features,
                                                    Errors& errors) {
  return LazySymbolTableSubsection{sec.data, features, errors};
}

}  // namespace binary
}  // namespace wasp

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

#include "wasp/binary/sections_linking.h"

namespace wasp {
namespace binary {

LazyComdatSubsection ReadComdatSubsection(SpanU8 data, Context& context) {
  return LazyComdatSubsection{data, "comdat subsection", context};
}

LazyComdatSubsection ReadComdatSubsection(LinkingSubsection sec,
                                          Context& context) {
  return ReadComdatSubsection(sec.data, context);
}

LazyInitFunctionsSubsection ReadInitFunctionsSubsection(SpanU8 data,
                                                        Context& context) {
  return LazyInitFunctionsSubsection{data, "init functions subsection",
                                     context};
}

LazyInitFunctionsSubsection ReadInitFunctionsSubsection(LinkingSubsection sec,
                                                        Context& context) {
  return ReadInitFunctionsSubsection(sec.data, context);
}

LazySegmentInfoSubsection ReadSegmentInfoSubsection(SpanU8 data,
                                                    Context& context) {
  return LazySegmentInfoSubsection{data, "segment info subsection", context};
}

LazySegmentInfoSubsection ReadSegmentInfoSubsection(LinkingSubsection sec,
                                                    Context& context) {
  return ReadSegmentInfoSubsection(sec.data, context);
}

LazySymbolTableSubsection ReadSymbolTableSubsection(SpanU8 data,
                                                    Context& context) {
  return LazySymbolTableSubsection{data, "symbol table subsection", context};
}

LazySymbolTableSubsection ReadSymbolTableSubsection(LinkingSubsection sec,
                                                    Context& context) {
  return ReadSymbolTableSubsection(sec.data, context);
}

}  // namespace binary
}  // namespace wasp

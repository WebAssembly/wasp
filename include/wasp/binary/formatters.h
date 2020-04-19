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

#ifndef WASP_BINARY_FORMATTERS_H_
#define WASP_BINARY_FORMATTERS_H_

#include "wasp/base/format.h"
#include "wasp/binary/types.h"

namespace fmt {

#define WASP_DEFINE_FORMATTER(Name)                                   \
  template <>                                                         \
  struct formatter<::wasp::binary::Name> : formatter<string_view> {   \
    template <typename Ctx>                                           \
    typename Ctx::iterator format(const ::wasp::binary::Name&, Ctx&); \
  } /* No semicolon. */

WASP_DEFINE_FORMATTER(BlockType);
WASP_DEFINE_FORMATTER(SectionId);
WASP_DEFINE_FORMATTER(MemArgImmediate);
WASP_DEFINE_FORMATTER(Locals);
WASP_DEFINE_FORMATTER(Section);
WASP_DEFINE_FORMATTER(KnownSection);
WASP_DEFINE_FORMATTER(CustomSection);
WASP_DEFINE_FORMATTER(TypeEntry);
WASP_DEFINE_FORMATTER(FunctionType);
WASP_DEFINE_FORMATTER(TableType);
WASP_DEFINE_FORMATTER(MemoryType);
WASP_DEFINE_FORMATTER(GlobalType);
WASP_DEFINE_FORMATTER(EventType);
WASP_DEFINE_FORMATTER(Import);
WASP_DEFINE_FORMATTER(Export);
WASP_DEFINE_FORMATTER(Expression);
WASP_DEFINE_FORMATTER(ConstantExpression);
WASP_DEFINE_FORMATTER(ElementExpression);
WASP_DEFINE_FORMATTER(CallIndirectImmediate);
WASP_DEFINE_FORMATTER(BrTableImmediate);
WASP_DEFINE_FORMATTER(BrOnExnImmediate);
WASP_DEFINE_FORMATTER(InitImmediate);
WASP_DEFINE_FORMATTER(CopyImmediate);
WASP_DEFINE_FORMATTER(ShuffleImmediate);
WASP_DEFINE_FORMATTER(Instruction);
WASP_DEFINE_FORMATTER(Function);
WASP_DEFINE_FORMATTER(Table);
WASP_DEFINE_FORMATTER(Memory);
WASP_DEFINE_FORMATTER(Global);
WASP_DEFINE_FORMATTER(Start);
WASP_DEFINE_FORMATTER(ElementSegment);
WASP_DEFINE_FORMATTER(ElementSegment::IndexesInit);
WASP_DEFINE_FORMATTER(ElementSegment::ExpressionsInit);
WASP_DEFINE_FORMATTER(Code);
WASP_DEFINE_FORMATTER(DataSegment);
WASP_DEFINE_FORMATTER(DataCount);
WASP_DEFINE_FORMATTER(Event);

#undef WASP_DEFINE_FORMATTER

}  // namespace fmt

#include "wasp/binary/formatters-inl.h"

#endif  // WASP_BINARY_FORMATTERS_H_

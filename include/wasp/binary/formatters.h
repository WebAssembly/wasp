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
#include "wasp/base/formatter_macros.h"
#include "wasp/binary/types.h"

namespace fmt {

WASP_DECLARE_FORMATTER(binary::BlockType);
WASP_DECLARE_FORMATTER(binary::SectionId);
WASP_DECLARE_FORMATTER(binary::MemArgImmediate);
WASP_DECLARE_FORMATTER(binary::Locals);
WASP_DECLARE_FORMATTER(binary::Section);
WASP_DECLARE_FORMATTER(binary::KnownSection);
WASP_DECLARE_FORMATTER(binary::CustomSection);
WASP_DECLARE_FORMATTER(binary::TypeEntry);
WASP_DECLARE_FORMATTER(binary::FunctionType);
WASP_DECLARE_FORMATTER(binary::TableType);
WASP_DECLARE_FORMATTER(binary::MemoryType);
WASP_DECLARE_FORMATTER(binary::GlobalType);
WASP_DECLARE_FORMATTER(binary::EventType);
WASP_DECLARE_FORMATTER(binary::Import);
WASP_DECLARE_FORMATTER(binary::Export);
WASP_DECLARE_FORMATTER(binary::Expression);
WASP_DECLARE_FORMATTER(binary::ConstantExpression);
WASP_DECLARE_FORMATTER(binary::ElementExpression);
WASP_DECLARE_FORMATTER(binary::CallIndirectImmediate);
WASP_DECLARE_FORMATTER(binary::BrTableImmediate);
WASP_DECLARE_FORMATTER(binary::BrOnExnImmediate);
WASP_DECLARE_FORMATTER(binary::InitImmediate);
WASP_DECLARE_FORMATTER(binary::CopyImmediate);
WASP_DECLARE_FORMATTER(binary::ShuffleImmediate);
WASP_DECLARE_FORMATTER(binary::Instruction);
WASP_DECLARE_FORMATTER(binary::Function);
WASP_DECLARE_FORMATTER(binary::Table);
WASP_DECLARE_FORMATTER(binary::Memory);
WASP_DECLARE_FORMATTER(binary::Global);
WASP_DECLARE_FORMATTER(binary::Start);
WASP_DECLARE_FORMATTER(binary::ElementSegment);
WASP_DECLARE_FORMATTER(binary::ElementSegment::IndexesInit);
WASP_DECLARE_FORMATTER(binary::ElementSegment::ExpressionsInit);
WASP_DECLARE_FORMATTER(binary::Code);
WASP_DECLARE_FORMATTER(binary::DataSegment);
WASP_DECLARE_FORMATTER(binary::DataCount);
WASP_DECLARE_FORMATTER(binary::Event);

}  // namespace fmt

#include "wasp/binary/formatters-inl.h"

#endif  // WASP_BINARY_FORMATTERS_H_

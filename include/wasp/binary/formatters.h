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

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/binary/types.h"

namespace wasp {

WASP_DEFINE_VARIANT_NAME(binary::RefType, "ref_type")
WASP_DEFINE_VARIANT_NAME(binary::ReferenceType, "reference_type")
WASP_DEFINE_VARIANT_NAME(binary::Rtt, "rtt")
WASP_DEFINE_VARIANT_NAME(binary::ValueType, "value_type")
WASP_DEFINE_VARIANT_NAME(binary::VoidType, "void")
WASP_DEFINE_VARIANT_NAME(binary::KnownSection, "known_section")
WASP_DEFINE_VARIANT_NAME(binary::CustomSection, "custom_section")
WASP_DEFINE_VARIANT_NAME(binary::BlockType, "block")
WASP_DEFINE_VARIANT_NAME(binary::BrOnExnImmediate, "br_on_exn")
WASP_DEFINE_VARIANT_NAME(binary::BrTableImmediate, "br_table")
WASP_DEFINE_VARIANT_NAME(binary::CallIndirectImmediate, "call_indirect")
WASP_DEFINE_VARIANT_NAME(binary::CopyImmediate, "copy")
WASP_DEFINE_VARIANT_NAME(binary::FuncBindImmediate, "func.bind")
WASP_DEFINE_VARIANT_NAME(binary::InitImmediate, "init")
WASP_DEFINE_VARIANT_NAME(binary::LetImmediate, "let")
WASP_DEFINE_VARIANT_NAME(binary::MemArgImmediate, "mem_arg")
WASP_DEFINE_VARIANT_NAME(binary::HeapType, "heap_type")
WASP_DEFINE_VARIANT_NAME(binary::SelectImmediate, "select")
WASP_DEFINE_VARIANT_NAME(binary::FunctionType, "func")
WASP_DEFINE_VARIANT_NAME(binary::StructType, "struct")
WASP_DEFINE_VARIANT_NAME(binary::ArrayType, "array")
WASP_DEFINE_VARIANT_NAME(binary::TableType, "table")
WASP_DEFINE_VARIANT_NAME(binary::GlobalType, "global")
WASP_DEFINE_VARIANT_NAME(binary::EventType, "event")
WASP_DEFINE_VARIANT_NAME(binary::ElementListWithIndexes, "index")
WASP_DEFINE_VARIANT_NAME(binary::ElementListWithExpressions, "expression")

namespace binary {

using wasp::operator<<;

WASP_BINARY_ENUMS(WASP_DECLARE_FORMATTER)
WASP_BINARY_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_FORMATTER)
WASP_DECLARE_FORMATTER(binary::Module)

WASP_DECLARE_FORMATTER(binary::InstructionList)

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_FORMATTERS_H_

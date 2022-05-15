//
// Copyright 2020 WebAssembly Community Group participants
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

#ifndef WASP_TEXT_FORMATTERS_H_
#define WASP_TEXT_FORMATTERS_H_

#include "wasp/base/formatters.h"
#include "wasp/base/formatter_macros.h"
#include "wasp/text/types.h"

namespace wasp {

// ReferenceType
WASP_DEFINE_VARIANT_NAME(text::RefType, "ref_type")

// ValueType.
WASP_DEFINE_VARIANT_NAME(text::ReferenceType, "reference_type")

// StorageType
WASP_DEFINE_VARIANT_NAME(text::ValueType, "value_type")

// DefinedType
WASP_DEFINE_VARIANT_NAME(text::BoundFunctionType, "func")
WASP_DEFINE_VARIANT_NAME(text::StructType, "struct")
WASP_DEFINE_VARIANT_NAME(text::ArrayType, "array")

// Token.
WASP_DEFINE_VARIANT_NAME(text::OpcodeInfo, "opcode_info")
WASP_DEFINE_VARIANT_NAME(text::LiteralInfo, "literal_info")
WASP_DEFINE_VARIANT_NAME(text::Text, "text")
WASP_DEFINE_VARIANT_NAME(text::SimdShape, "simd_shape")

// Instruction.
WASP_DEFINE_VARIANT_NAME(text::BlockImmediate, "block")
WASP_DEFINE_VARIANT_NAME(text::BrTableImmediate, "br_table")
WASP_DEFINE_VARIANT_NAME(text::BrOnCastImmediate, "br_on_cast")
WASP_DEFINE_VARIANT_NAME(text::CallIndirectImmediate, "call_indirect")
WASP_DEFINE_VARIANT_NAME(text::CopyImmediate, "copy")
WASP_DEFINE_VARIANT_NAME(text::FuncBindImmediate, "func.bind")
WASP_DEFINE_VARIANT_NAME(text::HeapType, "heap_type")
WASP_DEFINE_VARIANT_NAME(text::HeapType2Immediate, "heap_type_2")
WASP_DEFINE_VARIANT_NAME(text::InitImmediate, "init")
WASP_DEFINE_VARIANT_NAME(text::LetImmediate, "let")
WASP_DEFINE_VARIANT_NAME(text::MemArgImmediate, "mem_arg")
WASP_DEFINE_VARIANT_NAME(text::RttSubImmediate, "rtt.sub")
WASP_DEFINE_VARIANT_NAME(text::SelectImmediate, "select")
WASP_DEFINE_VARIANT_NAME(text::StructFieldImmediate, "struct_field")
WASP_DEFINE_VARIANT_NAME(text::SimdMemoryLaneImmediate, "memory_lane")
WASP_DEFINE_VARIANT_NAME(text::Var, "var")

// Import.
WASP_DEFINE_VARIANT_NAME(text::FunctionDesc, "func")
WASP_DEFINE_VARIANT_NAME(text::TableDesc, "table")
WASP_DEFINE_VARIANT_NAME(text::MemoryDesc, "memory")
WASP_DEFINE_VARIANT_NAME(text::GlobalDesc, "global")
WASP_DEFINE_VARIANT_NAME(text::TagDesc, "tag")

// ElementList.
WASP_DEFINE_VARIANT_NAME(text::ElementListWithExpressions, "expression")
WASP_DEFINE_VARIANT_NAME(text::ElementListWithVars, "var")

// DataItem.
WASP_DEFINE_VARIANT_NAME(text::NumericData, "numeric_data")

// ModuleItem.
WASP_DEFINE_VARIANT_NAME(text::DefinedType, "type")
WASP_DEFINE_VARIANT_NAME(text::Import, "import")
WASP_DEFINE_VARIANT_NAME(text::Function, "func")
WASP_DEFINE_VARIANT_NAME(text::Table, "table")
WASP_DEFINE_VARIANT_NAME(text::Memory, "memory")
WASP_DEFINE_VARIANT_NAME(text::Global, "global")
WASP_DEFINE_VARIANT_NAME(text::Export, "export")
WASP_DEFINE_VARIANT_NAME(text::Start, "start")
WASP_DEFINE_VARIANT_NAME(text::ElementSegment, "elem")
WASP_DEFINE_VARIANT_NAME(text::DataSegment, "data")
WASP_DEFINE_VARIANT_NAME(text::Tag, "tag")

// ScriptModule.
WASP_DEFINE_VARIANT_NAME(text::Module, "module")
WASP_DEFINE_VARIANT_NAME(text::TextList, "text_list")

// Const.
WASP_DEFINE_VARIANT_NAME(text::RefNullConst, "ref.null")
WASP_DEFINE_VARIANT_NAME(text::RefExternConst, "ref.extern")

// Action.
WASP_DEFINE_VARIANT_NAME(text::InvokeAction, "invoke")
WASP_DEFINE_VARIANT_NAME(text::GetAction, "get")

// FloatResult.
WASP_DEFINE_VARIANT_NAME(text::NanKind, "nan")

// ReturnResult.
WASP_DEFINE_VARIANT_NAME(text::F32Result, "f32")
WASP_DEFINE_VARIANT_NAME(text::F64Result, "f64")
WASP_DEFINE_VARIANT_NAME(text::F32x4Result, "f32x4")
WASP_DEFINE_VARIANT_NAME(text::F64x2Result, "f64x2")
WASP_DEFINE_VARIANT_NAME(text::RefNullResult, "ref.null")
WASP_DEFINE_VARIANT_NAME(text::RefExternResult, "ref.extern")
WASP_DEFINE_VARIANT_NAME(text::RefFuncResult, "ref.func")

// Assertion.
WASP_DEFINE_VARIANT_NAME(text::ModuleAssertion, "module")
WASP_DEFINE_VARIANT_NAME(text::ActionAssertion, "action")
WASP_DEFINE_VARIANT_NAME(text::ReturnAssertion, "return")

// Command.
WASP_DEFINE_VARIANT_NAME(text::ScriptModule, "module")
WASP_DEFINE_VARIANT_NAME(text::Register, "register")
WASP_DEFINE_VARIANT_NAME(text::Action, "action")
WASP_DEFINE_VARIANT_NAME(text::Assertion, "assertion")

namespace text {

using wasp::operator<<;

WASP_TEXT_ENUMS(WASP_DECLARE_FORMATTER)
WASP_TEXT_STRUCTS(WASP_DECLARE_FORMATTER)
WASP_TEXT_STRUCTS_CUSTOM_FORMAT(WASP_DECLARE_FORMATTER)

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_FORMATTERS_H_

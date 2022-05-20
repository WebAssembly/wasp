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

#ifndef WASP_TEXT_RESOLVE_H_
#define WASP_TEXT_RESOLVE_H_

#include "wasp/text/types.h"

namespace wasp {

class Errors;

namespace text {

struct ResolveCtx;
class NameMap;

// Primary API; resolve either a Module or a Script.

void Resolve(Module&, Errors&);
void Resolve(Script&, Errors&);

// The functions below are used to implement the API above, and not meant to be
// called by most users. They are exposed here primarily for testing purposes.

// DefineTypes() is an initial pass over the Module, to gather all type names.
// This is so a function type can reference type names too, e.g.
//
//   (type $A (func))
//   (type $B (func (param (ref $A)))
//
void DefineTypes(ResolveCtx&, const DefinedType&);
void DefineTypes(ResolveCtx&, const ModuleItem&);
void DefineTypes(ResolveCtx&, const Module&);

// Define() is the second pass over the Module, to handle all non-type names,
// and to create a mapping of function types to indexes.
void Define(ResolveCtx&, const OptAt<BindVar>&, NameMap&);
void Define(ResolveCtx&, const BoundValueTypeList&, NameMap&);
void Define(ResolveCtx&, const FieldType&);
void Define(ResolveCtx&, const FieldTypeList&);
void Define(ResolveCtx&, const DefinedType&);
void Define(ResolveCtx&, const FunctionDesc&);
void Define(ResolveCtx&, const TableDesc&);
void Define(ResolveCtx&, const MemoryDesc&);
void Define(ResolveCtx&, const GlobalDesc&);
void Define(ResolveCtx&, const TagDesc&);
void Define(ResolveCtx&, const Import&);
void Define(ResolveCtx&, const ElementSegment&);
void Define(ResolveCtx&, const DataSegment&);
void Define(ResolveCtx&, const ModuleItem&);
void Define(ResolveCtx&, const Module&);

// Resolve() is the final pass over the Module, which uses all the
// names/function types defined in DefineTypes() and Define() to convert them
// to their respective indexes.
void Resolve(ResolveCtx&, At<Var>&, NameMap&);
void Resolve(ResolveCtx&, OptAt<Var>&, NameMap&);
void Resolve(ResolveCtx&, VarList&, NameMap&);
void Resolve(ResolveCtx&, HeapType&);
void Resolve(ResolveCtx&, RefType&);
void Resolve(ResolveCtx&, ReferenceType&);
void Resolve(ResolveCtx&, Rtt&);
void Resolve(ResolveCtx&, ValueType&);
void Resolve(ResolveCtx&, ValueTypeList&);
void Resolve(ResolveCtx&, StorageType&);
void Resolve(ResolveCtx&, FunctionType&);
void Resolve(ResolveCtx&, FunctionTypeUse&);
void Resolve(ResolveCtx&, BoundValueType&);
void Resolve(ResolveCtx&, BoundValueTypeList&);
void Resolve(ResolveCtx&, BoundFunctionType&);
void Resolve(ResolveCtx&, OptAt<Var>& type_use, At<BoundFunctionType>&);
void Resolve(ResolveCtx&, FieldType&);
void Resolve(ResolveCtx&, FieldTypeList&);
void Resolve(ResolveCtx&, StructType&);
void Resolve(ResolveCtx&, ArrayType&);
void Resolve(ResolveCtx&, DefinedType&);
void Resolve(ResolveCtx&, BlockImmediate&);
void Resolve(ResolveCtx&, BrOnCastImmediate&);
void Resolve(ResolveCtx&, BrTableImmediate&);
void Resolve(ResolveCtx&, CallIndirectImmediate&);
void Resolve(ResolveCtx&, CopyImmediate&, NameMap&);
void Resolve(ResolveCtx&, HeapType2Immediate&);
void Resolve(ResolveCtx&, InitImmediate&, NameMap& segment, NameMap& dst);
void Resolve(ResolveCtx&, LetImmediate&);
void Resolve(ResolveCtx&, MemArgImmediate&);
void Resolve(ResolveCtx&, MemOptImmediate&);
void Resolve(ResolveCtx&, RttSubImmediate&);
void Resolve(ResolveCtx&, SimdMemoryLaneImmediate&);
void Resolve(ResolveCtx&, StructFieldImmediate&);
void Resolve(ResolveCtx&, Instruction&);
void Resolve(ResolveCtx&, InstructionList&);
void Resolve(ResolveCtx&, FunctionDesc&);
void Resolve(ResolveCtx&, TableType&);
void Resolve(ResolveCtx&, TableDesc&);
void Resolve(ResolveCtx&, GlobalType&);
void Resolve(ResolveCtx&, GlobalDesc&);
void Resolve(ResolveCtx&, TagType&);
void Resolve(ResolveCtx&, TagDesc&);
void Resolve(ResolveCtx&, Import&);
void Resolve(ResolveCtx&, Function&);
void Resolve(ResolveCtx&, ConstantExpression&);
void Resolve(ResolveCtx&, ElementExpression&);
void Resolve(ResolveCtx&, ElementExpressionList&);
void Resolve(ResolveCtx&, ElementListWithExpressions&);
void Resolve(ResolveCtx&, ElementListWithVars&);
void Resolve(ResolveCtx&, ElementList&);
void Resolve(ResolveCtx&, Table&);
void Resolve(ResolveCtx&, Global&);
void Resolve(ResolveCtx&, Export&);
void Resolve(ResolveCtx&, Start&);
void Resolve(ResolveCtx&, ElementSegment&);
void Resolve(ResolveCtx&, DataSegment&);
void Resolve(ResolveCtx&, Tag&);
void Resolve(ResolveCtx&, ModuleItem&);
void Resolve(ResolveCtx&, Module&);
void Resolve(ResolveCtx&, ScriptModule&);
void Resolve(ResolveCtx&, ModuleAssertion&);
void Resolve(ResolveCtx&, Assertion&);
void Resolve(ResolveCtx&, Command&);
void Resolve(ResolveCtx&, Script&);

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_RESOLVE_H_

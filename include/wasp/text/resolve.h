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

struct ResolveContext;
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
void DefineTypes(ResolveContext&, const DefinedType&);
void DefineTypes(ResolveContext&, const ModuleItem&);
void DefineTypes(ResolveContext&, const Module&);

// Define() is the second pass over the Module, to handle all non-type names,
// and to create a mapping of function types to indexes.
void Define(ResolveContext&, const OptAt<BindVar>&, NameMap&);
void Define(ResolveContext&, const BoundValueTypeList&, NameMap&);
void Define(ResolveContext&, const DefinedType&);
void Define(ResolveContext&, const FunctionDesc&);
void Define(ResolveContext&, const TableDesc&);
void Define(ResolveContext&, const MemoryDesc&);
void Define(ResolveContext&, const GlobalDesc&);
void Define(ResolveContext&, const EventDesc&);
void Define(ResolveContext&, const Import&);
void Define(ResolveContext&, const ElementSegment&);
void Define(ResolveContext&, const DataSegment&);
void Define(ResolveContext&, const ModuleItem&);
void Define(ResolveContext&, const Module&);

// Resolve() is the final pass over the Module, which uses all the
// names/function types defined in DefineTypes() and Define() to convert them
// to their respective indexes.
void Resolve(ResolveContext&, At<Var>&, NameMap&);
void Resolve(ResolveContext&, OptAt<Var>&, NameMap&);
void Resolve(ResolveContext&, VarList&, NameMap&);
void Resolve(ResolveContext&, HeapType&);
void Resolve(ResolveContext&, RefType&);
void Resolve(ResolveContext&, ReferenceType&);
void Resolve(ResolveContext&, ValueType&);
void Resolve(ResolveContext&, ValueTypeList&);
void Resolve(ResolveContext&, FunctionType&);
void Resolve(ResolveContext&, FunctionTypeUse&);
void Resolve(ResolveContext&, BoundValueType&);
void Resolve(ResolveContext&, BoundValueTypeList&);
void Resolve(ResolveContext&, BoundFunctionType&);
void Resolve(ResolveContext&, OptAt<Var>& type_use, At<BoundFunctionType>&);
void Resolve(ResolveContext&, DefinedType&);
void Resolve(ResolveContext&, BlockImmediate&);
void Resolve(ResolveContext&, BrOnExnImmediate&);
void Resolve(ResolveContext&, BrTableImmediate&);
void Resolve(ResolveContext&, CallIndirectImmediate&);
void Resolve(ResolveContext&, CopyImmediate&, NameMap&);
void Resolve(ResolveContext&, InitImmediate&, NameMap& segment, NameMap& dst);
void Resolve(ResolveContext&, LetImmediate&);
void Resolve(ResolveContext&, Instruction&);
void Resolve(ResolveContext&, InstructionList&);
void Resolve(ResolveContext&, FunctionDesc&);
void Resolve(ResolveContext&, TableType&);
void Resolve(ResolveContext&, TableDesc&);
void Resolve(ResolveContext&, GlobalType&);
void Resolve(ResolveContext&, GlobalDesc&);
void Resolve(ResolveContext&, EventType&);
void Resolve(ResolveContext&, EventDesc&);
void Resolve(ResolveContext&, Import&);
void Resolve(ResolveContext&, Function&);
void Resolve(ResolveContext&, ConstantExpression&);
void Resolve(ResolveContext&, ElementExpression&);
void Resolve(ResolveContext&, ElementExpressionList&);
void Resolve(ResolveContext&, ElementListWithExpressions&);
void Resolve(ResolveContext&, ElementListWithVars&);
void Resolve(ResolveContext&, ElementList&);
void Resolve(ResolveContext&, Table&);
void Resolve(ResolveContext&, Global&);
void Resolve(ResolveContext&, Export&);
void Resolve(ResolveContext&, Start&);
void Resolve(ResolveContext&, ElementSegment&);
void Resolve(ResolveContext&, DataSegment&);
void Resolve(ResolveContext&, Event&);
void Resolve(ResolveContext&, ModuleItem&);
void Resolve(ResolveContext&, Module&);
void Resolve(ResolveContext&, ScriptModule&);
void Resolve(ResolveContext&, ModuleAssertion&);
void Resolve(ResolveContext&, Assertion&);
void Resolve(ResolveContext&, Command&);
void Resolve(ResolveContext&, Script&);

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_RESOLVE_H_

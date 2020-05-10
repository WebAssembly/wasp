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
namespace text {

struct Context;
struct NameMap;

void Resolve(Context&, NameMap&, At<Var>&);
void Resolve(Context&, NameMap&, OptAt<Var>&);
void Resolve(Context&, NameMap&, VarList&);
void Resolve(Context&, FunctionTypeUse&);
void Resolve(Context&, OptAt<Var>& type_use, At<BoundFunctionType>&);
void Resolve(Context&, BlockImmediate&);
void Resolve(Context&, BrOnExnImmediate&);
void Resolve(Context&, BrTableImmediate&);
void Resolve(Context&, CallIndirectImmediate&);
void Resolve(Context&, NameMap&, CopyImmediate&);
void Resolve(Context&, NameMap& segment, NameMap& dst, InitImmediate&);
void Resolve(Context&, Instruction&);
void Resolve(Context&, InstructionList&);
void Resolve(Context&, FunctionDesc&);
void Resolve(Context&, EventType&);
void Resolve(Context&, EventDesc&);
void Resolve(Context&, Import&);
void Resolve(Context&, Function&);
void Resolve(Context&, ConstantExpression&);
void Resolve(Context&, ElementExpression&);
void Resolve(Context&, ElementExpressionList&);
void Resolve(Context&, ElementListWithExpressions&);
void Resolve(Context&, ElementListWithVars&);
void Resolve(Context&, ElementList&);
void Resolve(Context&, Table&);
void Resolve(Context&, Global&);
void Resolve(Context&, Export&);
void Resolve(Context&, Start&);
void Resolve(Context&, ElementSegment&);
void Resolve(Context&, DataSegment&);
void Resolve(Context&, Event&);
void Resolve(Context&, ModuleItem&);
void Resolve(Context&, Module&);
void Resolve(Context&, ScriptModule&);

}  // namespace text
}  // namespace wasp

#endif  // WASP_TEXT_RESOLVE_H_

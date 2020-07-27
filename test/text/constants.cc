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

#include "test/text/constants.h"

namespace wasp {
namespace text {
namespace test {

using HT = text::HeapType;
using RT = text::ReferenceType;
using VT = text::ValueType;

const HT HT_Func{MakeAt("func"_su8, HeapKind::Func)};
const HT HT_Extern{MakeAt("extern"_su8, HeapKind::Extern)};
const HT HT_Exn{MakeAt("exn"_su8, HeapKind::Exn)};
const HT HT_0{MakeAt("0"_su8, Var{MakeAt("0"_su8, Index{0})})};
const HT HT_T{MakeAt("$t"_su8, Var{MakeAt("$t"_su8, "$t"_sv)})};

const RefType RefType_Func{MakeAt("func"_su8, HT_Func), Null::No};
const RefType RefType_NullFunc{MakeAt("func"_su8, HT_Func), Null::Yes};
const RefType RefType_Extern{MakeAt("extern"_su8, HT_Extern), Null::No};
const RefType RefType_NullExtern{MakeAt("extern"_su8, HT_Extern), Null::Yes};
const RefType RefType_Exn{MakeAt("exn"_su8, HT_Exn), Null::No};
const RefType RefType_NullExn{MakeAt("exn"_su8, HT_Exn), Null::Yes};
const RefType RefType_0{MakeAt("0"_su8, HT_0), Null::No};
const RefType RefType_Null0{MakeAt("0"_su8, HT_0), Null::Yes};
const RefType RefType_T{MakeAt("$t"_su8, HT_T), Null::No};
const RefType RefType_NullT{MakeAt("$t"_su8, HT_T), Null::Yes};

const RT RT_Funcref{MakeAt("funcref"_su8, ReferenceKind::Funcref)};
const RT RT_Externref{MakeAt("externref"_su8, ReferenceKind::Externref)};
const RT RT_Exnref{MakeAt("exnref"_su8, ReferenceKind::Exnref)};
const RT RT_RefFunc{MakeAt("func"_su8, RefType_Func)};
const RT RT_RefNullFunc{MakeAt("null func"_su8, RefType_NullFunc)};
const RT RT_RefExtern{MakeAt("extern"_su8, RefType_Extern)};
const RT RT_RefNullExtern{MakeAt("null extern"_su8, RefType_NullExtern)};
const RT RT_RefExn{MakeAt("exn"_su8, RefType_Exn)};
const RT RT_RefNullExn{MakeAt("null exn"_su8, RefType_NullExn)};
const RT RT_Ref0{MakeAt("0"_su8, RefType_0)};
const RT RT_RefNull0{MakeAt("null 0"_su8, RefType_Null0)};
const RT RT_RefT{MakeAt("$t"_su8, RefType_T)};
const RT RT_RefNullT{MakeAt("null $t"_su8, RefType_NullT)};

const VT VT_I32{MakeAt("i32"_su8, NumericType::I32)};
const VT VT_I64{MakeAt("i64"_su8, NumericType::I64)};
const VT VT_F32{MakeAt("f32"_su8, NumericType::F32)};
const VT VT_F64{MakeAt("f64"_su8, NumericType::F64)};
const VT VT_V128{MakeAt("v128"_su8, NumericType::V128)};
const VT VT_Funcref{MakeAt("funcref"_su8, RT_Funcref)};
const VT VT_Externref{MakeAt("externref"_su8, RT_Externref)};
const VT VT_Exnref{MakeAt("exnref"_su8, RT_Exnref)};
const VT VT_RefFunc{MakeAt("(ref func)"_su8, RT_RefFunc)};
const VT VT_RefNullFunc{MakeAt("(ref null func)"_su8, RT_RefNullFunc)};
const VT VT_RefExtern{MakeAt("(ref extern)"_su8, RT_RefExtern)};
const VT VT_RefNullExtern{MakeAt("(ref null extern)"_su8, RT_RefNullExtern)};
const VT VT_RefExn{MakeAt("(ref exn)"_su8, RT_RefExn)};
const VT VT_RefNullExn{MakeAt("(ref null exn)"_su8, RT_RefNullExn)};
const VT VT_Ref0{MakeAt("(ref 0)"_su8, RT_Ref0)};
const VT VT_RefNull0{MakeAt("(ref null 0)"_su8, RT_RefNull0)};
const VT VT_RefT{MakeAt("(ref $t)"_su8, RT_RefT)};
const VT VT_RefNullT{MakeAt("(ref null $t)"_su8, RT_RefNullT)};

}  // namespace test
}  // namespace text
}  // namespace wasp

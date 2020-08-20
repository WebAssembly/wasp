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

const HT HT_Func{At{"func"_su8, HeapKind::Func}};
const HT HT_Extern{At{"extern"_su8, HeapKind::Extern}};
const HT HT_Exn{At{"exn"_su8, HeapKind::Exn}};
const HT HT_0{At{"0"_su8, Var{At{"0"_su8, Index{0}}}}};
const HT HT_T{At{"$t"_su8, Var{At{"$t"_su8, "$t"_sv}}}};

const RefType RefType_Func{At{"func"_su8, HT_Func}, Null::No};
const RefType RefType_NullFunc{At{"func"_su8, HT_Func}, Null::Yes};
const RefType RefType_Extern{At{"extern"_su8, HT_Extern}, Null::No};
const RefType RefType_NullExtern{At{"extern"_su8, HT_Extern}, Null::Yes};
const RefType RefType_Exn{At{"exn"_su8, HT_Exn}, Null::No};
const RefType RefType_NullExn{At{"exn"_su8, HT_Exn}, Null::Yes};
const RefType RefType_0{At{"0"_su8, HT_0}, Null::No};
const RefType RefType_Null0{At{"0"_su8, HT_0}, Null::Yes};
const RefType RefType_T{At{"$t"_su8, HT_T}, Null::No};
const RefType RefType_NullT{At{"$t"_su8, HT_T}, Null::Yes};

const RT RT_Funcref{At{"funcref"_su8, ReferenceKind::Funcref}};
const RT RT_Externref{At{"externref"_su8, ReferenceKind::Externref}};
const RT RT_Exnref{At{"exnref"_su8, ReferenceKind::Exnref}};
const RT RT_RefFunc{At{"func"_su8, RefType_Func}};
const RT RT_RefNullFunc{At{"null func"_su8, RefType_NullFunc}};
const RT RT_RefExtern{At{"extern"_su8, RefType_Extern}};
const RT RT_RefNullExtern{At{"null extern"_su8, RefType_NullExtern}};
const RT RT_RefExn{At{"exn"_su8, RefType_Exn}};
const RT RT_RefNullExn{At{"null exn"_su8, RefType_NullExn}};
const RT RT_Ref0{At{"0"_su8, RefType_0}};
const RT RT_RefNull0{At{"null 0"_su8, RefType_Null0}};
const RT RT_RefT{At{"$t"_su8, RefType_T}};
const RT RT_RefNullT{At{"null $t"_su8, RefType_NullT}};

const VT VT_I32{At{"i32"_su8, NumericType::I32}};
const VT VT_I64{At{"i64"_su8, NumericType::I64}};
const VT VT_F32{At{"f32"_su8, NumericType::F32}};
const VT VT_F64{At{"f64"_su8, NumericType::F64}};
const VT VT_V128{At{"v128"_su8, NumericType::V128}};
const VT VT_Funcref{At{"funcref"_su8, RT_Funcref}};
const VT VT_Externref{At{"externref"_su8, RT_Externref}};
const VT VT_Exnref{At{"exnref"_su8, RT_Exnref}};
const VT VT_RefFunc{At{"(ref func)"_su8, RT_RefFunc}};
const VT VT_RefNullFunc{At{"(ref null func)"_su8, RT_RefNullFunc}};
const VT VT_RefExtern{At{"(ref extern)"_su8, RT_RefExtern}};
const VT VT_RefNullExtern{At{"(ref null extern)"_su8, RT_RefNullExtern}};
const VT VT_RefExn{At{"(ref exn)"_su8, RT_RefExn}};
const VT VT_RefNullExn{At{"(ref null exn)"_su8, RT_RefNullExn}};
const VT VT_Ref0{At{"(ref 0)"_su8, RT_Ref0}};
const VT VT_RefNull0{At{"(ref null 0)"_su8, RT_RefNull0}};
const VT VT_RefT{At{"(ref $t)"_su8, RT_RefT}};
const VT VT_RefNullT{At{"(ref null $t)"_su8, RT_RefNullT}};

}  // namespace test
}  // namespace text
}  // namespace wasp

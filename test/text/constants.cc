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
const HT HT_Eq{At{"eq"_su8, HeapKind::Eq}};
const HT HT_I31{At{"i31"_su8, HeapKind::I31}};
const HT HT_Any{At{"any"_su8, HeapKind::Any}};
const HT HT_0{At{"0"_su8, Var{At{"0"_su8, Index{0}}}}};
const HT HT_T{At{"$t"_su8, Var{At{"$t"_su8, "$t"_sv}}}};

const RefType RefType_Func{At{"func"_su8, HT_Func}, Null::No};
const RefType RefType_NullFunc{At{"func"_su8, HT_Func}, Null::Yes};
const RefType RefType_Extern{At{"extern"_su8, HT_Extern}, Null::No};
const RefType RefType_NullExtern{At{"extern"_su8, HT_Extern}, Null::Yes};
const RefType RefType_Eq{At{"eq"_su8, HT_Eq}, Null::No};
const RefType RefType_NullEq{At{"eq"_su8, HT_Eq}, Null::Yes};
const RefType RefType_I31{At{"i31"_su8, HT_I31}, Null::No};
const RefType RefType_NullI31{At{"i31"_su8, HT_I31}, Null::Yes};
const RefType RefType_Any{At{"any"_su8, HT_Any}, Null::No};
const RefType RefType_NullAny{At{"any"_su8, HT_Any}, Null::Yes};
const RefType RefType_0{At{"0"_su8, HT_0}, Null::No};
const RefType RefType_Null0{At{"0"_su8, HT_0}, Null::Yes};
const RefType RefType_T{At{"$t"_su8, HT_T}, Null::No};
const RefType RefType_NullT{At{"$t"_su8, HT_T}, Null::Yes};

const RT RT_Funcref{At{"funcref"_su8, ReferenceKind::Funcref}};
const RT RT_Externref{At{"externref"_su8, ReferenceKind::Externref}};
const RT RT_Eqref{At{"eqref"_su8, ReferenceKind::Eqref}};
const RT RT_I31ref{At{"i31ref"_su8, ReferenceKind::I31ref}};
const RT RT_Anyref{At{"anyref"_su8, ReferenceKind::Anyref}};
const RT RT_RefFunc{At{"func"_su8, RefType_Func}};
const RT RT_RefNullFunc{At{"null func"_su8, RefType_NullFunc}};
const RT RT_RefExtern{At{"extern"_su8, RefType_Extern}};
const RT RT_RefNullExtern{At{"null extern"_su8, RefType_NullExtern}};
const RT RT_RefEq{At{"eq"_su8, RefType_Eq}};
const RT RT_RefNullEq{At{"null eq"_su8, RefType_NullEq}};
const RT RT_RefI31{At{"i31"_su8, RefType_I31}};
const RT RT_RefNullI31{At{"null i31"_su8, RefType_NullI31}};
const RT RT_RefAny{At{"any"_su8, RefType_Any}};
const RT RT_RefNullAny{At{"null any"_su8, RefType_NullAny}};
const RT RT_Ref0{At{"0"_su8, RefType_0}};
const RT RT_RefNull0{At{"null 0"_su8, RefType_Null0}};
const RT RT_RefT{At{"$t"_su8, RefType_T}};
const RT RT_RefNullT{At{"null $t"_su8, RefType_NullT}};

const Rtt RTT_0_Func{At{"0"_su8, 0u}, At{"func"_su8, HT_Func}};
const Rtt RTT_0_Extern{At{"0"_su8, 0u}, At{"extern"_su8, HT_Extern}};
const Rtt RTT_0_Eq{At{"0"_su8, 0u}, At{"eq"_su8, HT_Eq}};
const Rtt RTT_0_I31{At{"0"_su8, 0u}, At{"i31"_su8, HT_I31}};
const Rtt RTT_0_Any{At{"0"_su8, 0u}, At{"any"_su8, HT_Any}};
const Rtt RTT_0_0{At{"0"_su8, 0u}, At{"0"_su8, HT_0}};
const Rtt RTT_0_T{At{"0"_su8, 0u}, At{"$t"_su8, HT_T}};
const Rtt RTT_1_Func{At{"1"_su8, 1u}, At{"func"_su8, HT_Func}};
const Rtt RTT_1_Extern{At{"1"_su8, 1u}, At{"extern"_su8, HT_Extern}};
const Rtt RTT_1_Eq{At{"1"_su8, 1u}, At{"eq"_su8, HT_Eq}};
const Rtt RTT_1_I31{At{"1"_su8, 1u}, At{"i31"_su8, HT_I31}};
const Rtt RTT_1_Any{At{"1"_su8, 1u}, At{"any"_su8, HT_Any}};
const Rtt RTT_1_0{At{"1"_su8, 1u}, At{"0"_su8, HT_0}};
const Rtt RTT_1_T{At{"1"_su8, 1u}, At{"$t"_su8, HT_T}};

const VT VT_I32{At{"i32"_su8, NumericType::I32}};
const VT VT_I64{At{"i64"_su8, NumericType::I64}};
const VT VT_F32{At{"f32"_su8, NumericType::F32}};
const VT VT_F64{At{"f64"_su8, NumericType::F64}};
const VT VT_V128{At{"v128"_su8, NumericType::V128}};
const VT VT_Funcref{At{"funcref"_su8, RT_Funcref}};
const VT VT_Externref{At{"externref"_su8, RT_Externref}};
const VT VT_Eqref{At{"eqref"_su8, RT_Eqref}};
const VT VT_I31ref{At{"i31ref"_su8, RT_I31ref}};
const VT VT_Anyref{At{"anyref"_su8, RT_Anyref}};
const VT VT_RefFunc{At{"(ref func)"_su8, RT_RefFunc}};
const VT VT_RefNullFunc{At{"(ref null func)"_su8, RT_RefNullFunc}};
const VT VT_RefExtern{At{"(ref extern)"_su8, RT_RefExtern}};
const VT VT_RefNullExtern{At{"(ref null extern)"_su8, RT_RefNullExtern}};
const VT VT_RefEq{At{"(ref eq)"_su8, RT_RefEq}};
const VT VT_RefNullEq{At{"(ref null eq)"_su8, RT_RefNullEq}};
const VT VT_RefI31{At{"(ref i31)"_su8, RT_RefI31}};
const VT VT_RefNullI31{At{"(ref null i31)"_su8, RT_RefNullI31}};
const VT VT_RefAny{At{"(ref any)"_su8, RT_RefAny}};
const VT VT_RefNullAny{At{"(ref null any)"_su8, RT_RefNullAny}};
const VT VT_Ref0{At{"(ref 0)"_su8, RT_Ref0}};
const VT VT_RefNull0{At{"(ref null 0)"_su8, RT_RefNull0}};
const VT VT_RefT{At{"(ref $t)"_su8, RT_RefT}};
const VT VT_RefNullT{At{"(ref null $t)"_su8, RT_RefNullT}};
const VT VT_RTT_0_Func{At{"(rtt 0 func)"_su8, RTT_0_Func}};
const VT VT_RTT_0_Extern{At{"(rtt 0 extern)"_su8, RTT_0_Extern}};
const VT VT_RTT_0_Eq{At{"(rtt 0 eq)"_su8, RTT_0_Eq}};
const VT VT_RTT_0_I31{At{"(rtt 0 i31)"_su8, RTT_0_I31}};
const VT VT_RTT_0_Any{At{"(rtt 0 any)"_su8, RTT_0_Any}};
const VT VT_RTT_0_0{At{"(rtt 0 0)"_su8, RTT_0_0}};
const VT VT_RTT_0_T{At{"(rtt 0 $t)"_su8, RTT_0_T}};
const VT VT_RTT_1_Func{At{"(rtt 1 func)"_su8, RTT_1_Func}};
const VT VT_RTT_1_Extern{At{"(rtt 1 extern)"_su8, RTT_1_Extern}};
const VT VT_RTT_1_Eq{At{"(rtt 1 eq)"_su8, RTT_1_Eq}};
const VT VT_RTT_1_I31{At{"(rtt 1 i31)"_su8, RTT_1_I31}};
const VT VT_RTT_1_Any{At{"(rtt 1 any)"_su8, RTT_1_Any}};
const VT VT_RTT_1_0{At{"(rtt 1 0)"_su8, RTT_1_0}};
const VT VT_RTT_1_T{At{"(rtt 1 $t)"_su8, RTT_1_T}};

}  // namespace test
}  // namespace text
}  // namespace wasp

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

#include "test/binary/constants.h"

namespace wasp {
namespace binary {
namespace test {

using HT = binary::HeapType;
using RT = binary::ReferenceType;
using VT = binary::ValueType;
using BT = binary::BlockType;

const HT HT_Func{At{"\x70"_su8, HeapKind::Func}};
const HT HT_Extern{At{"\x6f"_su8, HeapKind::Extern}};
const HT HT_Any{At{"\x6e"_su8, HeapKind::Any}};
const HT HT_Eq{At{"\x6d"_su8, HeapKind::Eq}};
const HT HT_Exn{At{"\x68"_su8, HeapKind::Exn}};
const HT HT_I31{At{"\x67"_su8, HeapKind::I31}};
const HT HT_0{At{"\x00"_su8, Index{0}}};
const HT HT_1{At{"\x01"_su8, Index{1}}};
const HT HT_2{At{"\x01"_su8, Index{2}}};

const RefType RefType_Func{At{"\x70"_su8, HT_Func}, Null::No};
const RefType RefType_NullFunc{At{"\x70"_su8, HT_Func}, Null::Yes};
const RefType RefType_Extern{At{"\x6f"_su8, HT_Extern}, Null::No};
const RefType RefType_NullExtern{At{"\x6f"_su8, HT_Extern}, Null::Yes};
const RefType RefType_Any{At{"\x6e"_su8, HT_Any}, Null::No};
const RefType RefType_NullAny{At{"\x6e"_su8, HT_Any}, Null::Yes};
const RefType RefType_Eq{At{"\x6d"_su8, HT_Eq}, Null::No};
const RefType RefType_NullEq{At{"\x6d"_su8, HT_Eq}, Null::Yes};
const RefType RefType_Exn{At{"\x68"_su8, HT_Exn}, Null::No};
const RefType RefType_NullExn{At{"\x68"_su8, HT_Exn}, Null::Yes};
const RefType RefType_I31{At{"\x67"_su8, HT_I31}, Null::No};
const RefType RefType_NullI31{At{"\x67"_su8, HT_I31}, Null::Yes};
const RefType RefType_0{At{"\x00"_su8, HT_0}, Null::No};
const RefType RefType_Null0{At{"\x00"_su8, HT_0}, Null::Yes};
const RefType RefType_1{At{"\x01"_su8, HT_1}, Null::No};
const RefType RefType_Null1{At{"\x01"_su8, HT_1}, Null::Yes};
const RefType RefType_2{At{"\x02"_su8, HT_2}, Null::No};
const RefType RefType_Null2{At{"\x02"_su8, HT_2}, Null::Yes};

const RT RT_Funcref{At{"\x70"_su8, ReferenceKind::Funcref}};
const RT RT_Externref{At{"\x6f"_su8, ReferenceKind::Externref}};
const RT RT_Anyref{At{"\x6e"_su8, ReferenceKind::Anyref}};
const RT RT_Eqref{At{"\x6d"_su8, ReferenceKind::Eqref}};
const RT RT_Exnref{At{"\x68"_su8, ReferenceKind::Exnref}};
const RT RT_I31ref{At{"\x67"_su8, ReferenceKind::I31ref}};
const RT RT_RefFunc{At{"\x6b\x70"_su8, RefType_Func}};
const RT RT_RefNullFunc{At{"\x6c\x70"_su8, RefType_NullFunc}};
const RT RT_RefExtern{At{"\x6b\x6f"_su8, RefType_Extern}};
const RT RT_RefNullExtern{At{"\x6c\x6f"_su8, RefType_NullExtern}};
const RT RT_RefAny{At{"\x6b\x6e"_su8, RefType_Any}};
const RT RT_RefNullAny{At{"\x6c\x6e"_su8, RefType_NullAny}};
const RT RT_RefEq{At{"\x6b\x6d"_su8, RefType_Eq}};
const RT RT_RefNullEq{At{"\x6c\x6d"_su8, RefType_NullEq}};
const RT RT_RefExn{At{"\x6b\x68"_su8, RefType_Exn}};
const RT RT_RefNullExn{At{"\x6c\x68"_su8, RefType_NullExn}};
const RT RT_RefI31{At{"\x6b\x67"_su8, RefType_I31}};
const RT RT_RefNullI31{At{"\x6c\x67"_su8, RefType_NullI31}};
const RT RT_Ref0{At{"\x6b\x00"_su8, RefType_0}};
const RT RT_RefNull0{At{"\x6c\x00"_su8, RefType_Null0}};
const RT RT_Ref1{At{"\x6b\x01"_su8, RefType_1}};
const RT RT_RefNull1{At{"\x6c\x01"_su8, RefType_Null1}};
const RT RT_Ref2{At{"\x6b\x02"_su8, RefType_2}};
const RT RT_RefNull2{At{"\x6c\x02"_su8, RefType_Null2}};

const Rtt RTT_0_Func{At{"\x00"_su8, 0u}, At{"\x70"_su8, HT_Func}};
const Rtt RTT_0_Extern{At{"\x00"_su8, 0u}, At{"\x6f"_su8, HT_Extern}};
const Rtt RTT_0_Any{At{"\x00"_su8, 0u}, At{"\x6e"_su8, HT_Any}};
const Rtt RTT_0_Eq{At{"\x00"_su8, 0u}, At{"\x6d"_su8, HT_Eq}};
const Rtt RTT_0_Exn{At{"\x00"_su8, 0u}, At{"\x68"_su8, HT_Exn}};
const Rtt RTT_0_I31{At{"\x00"_su8, 0u}, At{"\x67"_su8, HT_I31}};
const Rtt RTT_0_0{At{"\x00"_su8, 0u}, At{"\x00"_su8, HT_0}};
const Rtt RTT_0_1{At{"\x00"_su8, 0u}, At{"\x01"_su8, HT_1}};
const Rtt RTT_0_2{At{"\x00"_su8, 0u}, At{"\x02"_su8, HT_2}};

const VT VT_I32{At{"\x7f"_su8, NumericType::I32}};
const VT VT_I64{At{"\x7e"_su8, NumericType::I64}};
const VT VT_F32{At{"\x7d"_su8, NumericType::F32}};
const VT VT_F64{At{"\x7c"_su8, NumericType::F64}};
const VT VT_V128{At{"\x7b"_su8, NumericType::V128}};
const VT VT_Funcref{At{"\x70"_su8, RT_Funcref}};
const VT VT_Externref{At{"\x6f"_su8, RT_Externref}};
const VT VT_Anyref{At{"\x6e"_su8, RT_Anyref}};
const VT VT_Eqref{At{"\x6d"_su8, RT_Eqref}};
const VT VT_Exnref{At{"\x68"_su8, RT_Exnref}};
const VT VT_I31ref{At{"\x67"_su8, RT_I31ref}};
const VT VT_RefFunc{At{"\x6b\x70"_su8, RT_RefFunc}};
const VT VT_RefNullFunc{At{"\x6c\x70"_su8, RT_RefNullFunc}};
const VT VT_RefExtern{At{"\x6b\x6f"_su8, RT_RefExtern}};
const VT VT_RefNullExtern{At{"\x6c\x6f"_su8, RT_RefNullExtern}};
const VT VT_RefAny{At{"\x6b\x6e"_su8, RT_RefAny}};
const VT VT_RefNullAny{At{"\x6c\x6e"_su8, RT_RefNullAny}};
const VT VT_RefEq{At{"\x6b\x6d"_su8, RT_RefEq}};
const VT VT_RefNullEq{At{"\x6c\x6d"_su8, RT_RefNullEq}};
const VT VT_RefExn{At{"\x6b\x68"_su8, RT_RefExn}};
const VT VT_RefNullExn{At{"\x6c\x68"_su8, RT_RefNullExn}};
const VT VT_RefI31{At{"\x6b\x67"_su8, RT_RefI31}};
const VT VT_RefNullI31{At{"\x6c\x67"_su8, RT_RefNullI31}};
const VT VT_Ref0{At{"\x6b\x00"_su8, RT_Ref0}};
const VT VT_RefNull0{At{"\x6c\x00"_su8, RT_RefNull0}};
const VT VT_Ref1{At{"\x6b\x01"_su8, RT_Ref1}};
const VT VT_RefNull1{At{"\x6c\x01"_su8, RT_RefNull1}};
const VT VT_Ref2{At{"\x6b\x02"_su8, RT_Ref2}};
const VT VT_RefNull2{At{"\x6c\x02"_su8, RT_RefNull2}};
const VT VT_RTT_0_Func{At{"\x6a\x00\x70"_su8, RTT_0_Func}};
const VT VT_RTT_0_Extern{At{"\x6a\x00\x6f"_su8, RTT_0_Extern}};
const VT VT_RTT_0_Any{At{"\x6a\x00\x6e"_su8, RTT_0_Any}};
const VT VT_RTT_0_Eq{At{"\x6a\x00\x6d"_su8, RTT_0_Eq}};
const VT VT_RTT_0_Exn{At{"\x6a\x00\x68"_su8, RTT_0_Exn}};
const VT VT_RTT_0_I31{At{"\x6a\x00\x67"_su8, RTT_0_I31}};
const VT VT_RTT_0_0{At{"\x6a\x00\x00"_su8, RTT_0_0}};
const VT VT_RTT_0_1{At{"\x6a\x00\x01"_su8, RTT_0_1}};
const VT VT_RTT_0_2{At{"\x6a\x00\x02"_su8, RTT_0_2}};

const BT BT_I32{At{"\x7f"_su8, VT_I32}};
const BT BT_I64{At{"\x7e"_su8, VT_I64}};
const BT BT_F32{At{"\x7d"_su8, VT_F32}};
const BT BT_F64{At{"\x7c"_su8, VT_F64}};
const BT BT_V128{At{"\x7b"_su8, VT_V128}};
const BT BT_Funcref{At{"\x70"_su8, VT_Funcref}};
const BT BT_Externref{At{"\x6f"_su8, VT_Externref}};
const BT BT_Exnref{At{"\x68"_su8, VT_Exnref}};
const BT BT_Void{At{"\x40"_su8, binary::VoidType{}}};
const BT BT_RefFunc{At{"\x6b\x70"_su8, VT_RefFunc}};
const BT BT_RefNullFunc{At{"\x6c\x70"_su8, VT_RefNullFunc}};
const BT BT_RefExtern{At{"\x6b\x6f"_su8, VT_RefExtern}};
const BT BT_RefNullExtern{At{"\x6c\x6f"_su8, VT_RefNullExtern}};
const BT BT_RefExn{At{"\x6b\x68"_su8, VT_RefExn}};
const BT BT_RefNullExn{At{"\x6c\x68"_su8, VT_RefNullExn}};
const BT BT_Ref0{At{"\x6b\x00"_su8, VT_Ref0}};
const BT BT_RefNull0{At{"\x6c\x00"_su8, VT_RefNull0}};
const BT BT_Ref1{At{"\x6b\x01"_su8, VT_Ref1}};
const BT BT_RefNull1{At{"\x6c\x01"_su8, VT_RefNull1}};
const BT BT_Ref2{At{"\x6b\x02"_su8, VT_Ref2}};
const BT BT_RefNull2{At{"\x6c\x02"_su8, VT_RefNull2}};

}  // namespace test
}  // namespace binary
}  // namespace wasp

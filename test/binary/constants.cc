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

const HT HT_Func{MakeAt("\x70"_su8, HeapKind::Func)};
const HT HT_Extern{MakeAt("\x6f"_su8, HeapKind::Extern)};
const HT HT_Exn{MakeAt("\x68"_su8, HeapKind::Exn)};
const HT HT_0{MakeAt("\x00"_su8, Index{0})};
const HT HT_1{MakeAt("\x01"_su8, Index{1})};

const RefType RefType_Func{MakeAt("\x70"_su8, HT_Func), Null::No};
const RefType RefType_NullFunc{MakeAt("\x70"_su8, HT_Func), Null::Yes};
const RefType RefType_Extern{MakeAt("\x6f"_su8, HT_Extern), Null::No};
const RefType RefType_NullExtern{MakeAt("\x6f"_su8, HT_Extern), Null::Yes};
const RefType RefType_Exn{MakeAt("\x68"_su8, HT_Exn), Null::No};
const RefType RefType_NullExn{MakeAt("\x68"_su8, HT_Exn), Null::Yes};
const RefType RefType_0{MakeAt("\x00"_su8, HT_0), Null::No};
const RefType RefType_Null0{MakeAt("\x00"_su8, HT_0), Null::Yes};
const RefType RefType_1{MakeAt("\x01"_su8, HT_1), Null::No};
const RefType RefType_Null1{MakeAt("\x01"_su8, HT_1), Null::Yes};

const RT RT_Funcref{MakeAt("\x70"_su8, ReferenceKind::Funcref)};
const RT RT_Externref{MakeAt("\x6f"_su8, ReferenceKind::Externref)};
const RT RT_Exnref{MakeAt("\x68"_su8, ReferenceKind::Exnref)};
const RT RT_RefFunc{MakeAt("\x6b\x70"_su8, RefType_Func)};
const RT RT_RefNullFunc{MakeAt("\x6c\x70"_su8, RefType_NullFunc)};
const RT RT_RefExtern{MakeAt("\x6b\x6f"_su8, RefType_Extern)};
const RT RT_RefNullExtern{MakeAt("\x6c\x6f"_su8, RefType_NullExtern)};
const RT RT_RefExn{MakeAt("\x6b\x68"_su8, RefType_Exn)};
const RT RT_RefNullExn{MakeAt("\x6c\x68"_su8, RefType_NullExn)};
const RT RT_Ref0{MakeAt("\x6b\x00"_su8, RefType_0)};
const RT RT_RefNull0{MakeAt("\x6c\x00"_su8, RefType_Null0)};
const RT RT_Ref1{MakeAt("\x6b\x01"_su8, RefType_1)};
const RT RT_RefNull1{MakeAt("\x6c\x01"_su8, RefType_Null1)};

const VT VT_I32{MakeAt("\x7f"_su8, NumericType::I32)};
const VT VT_I64{MakeAt("\x7e"_su8, NumericType::I64)};
const VT VT_F32{MakeAt("\x7d"_su8, NumericType::F32)};
const VT VT_F64{MakeAt("\x7c"_su8, NumericType::F64)};
const VT VT_V128{MakeAt("\x7b"_su8, NumericType::V128)};
const VT VT_Funcref{MakeAt("\x70"_su8, RT_Funcref)};
const VT VT_Externref{MakeAt("\x6f"_su8, RT_Externref)};
const VT VT_Exnref{MakeAt("\x68"_su8, RT_Exnref)};
const VT VT_RefFunc{MakeAt("\x6b\x70"_su8, RT_RefFunc)};
const VT VT_RefNullFunc{MakeAt("\x6c\x70"_su8, RT_RefNullFunc)};
const VT VT_RefExtern{MakeAt("\x6b\x6f"_su8, RT_RefExtern)};
const VT VT_RefNullExtern{MakeAt("\x6c\x6f"_su8, RT_RefNullExtern)};
const VT VT_RefExn{MakeAt("\x6b\x68"_su8, RT_RefExn)};
const VT VT_RefNullExn{MakeAt("\x6c\x68"_su8, RT_RefNullExn)};
const VT VT_Ref0{MakeAt("\x6b\x00"_su8, RT_Ref0)};
const VT VT_RefNull0{MakeAt("\x6c\x00"_su8, RT_RefNull0)};
const VT VT_Ref1{MakeAt("\x6b\x01"_su8, RT_Ref1)};
const VT VT_RefNull1{MakeAt("\x6c\x01"_su8, RT_RefNull1)};

const BT BT_I32{MakeAt("\x7f"_su8, VT_I32)};
const BT BT_I64{MakeAt("\x7e"_su8, VT_I64)};
const BT BT_F32{MakeAt("\x7d"_su8, VT_F32)};
const BT BT_F64{MakeAt("\x7c"_su8, VT_F64)};
const BT BT_V128{MakeAt("\x7b"_su8, VT_V128)};
const BT BT_Funcref{MakeAt("\x70"_su8, VT_Funcref)};
const BT BT_Externref{MakeAt("\x6f"_su8, VT_Externref)};
const BT BT_Exnref{MakeAt("\x68"_su8, VT_Exnref)};
const BT BT_Void{MakeAt("\x40"_su8, binary::VoidType{})};
const BT BT_RefFunc{MakeAt("\x6b\x70"_su8, VT_RefFunc)};
const BT BT_RefNullFunc{MakeAt("\x6c\x70"_su8, VT_RefNullFunc)};
const BT BT_RefExtern{MakeAt("\x6b\x6f"_su8, VT_RefExtern)};
const BT BT_RefNullExtern{MakeAt("\x6c\x6f"_su8, VT_RefNullExtern)};
const BT BT_RefExn{MakeAt("\x6b\x68"_su8, VT_RefExn)};
const BT BT_RefNullExn{MakeAt("\x6c\x68"_su8, VT_RefNullExn)};
const BT BT_Ref0{MakeAt("\x6b\x00"_su8, VT_Ref0)};
const BT BT_RefNull0{MakeAt("\x6c\x00"_su8, VT_RefNull0)};
const BT BT_Ref1{MakeAt("\x6b\x01"_su8, VT_Ref1)};
const BT BT_RefNull1{MakeAt("\x6c\x01"_su8, VT_RefNull1)};

}  // namespace test
}  // namespace binary
}  // namespace wasp
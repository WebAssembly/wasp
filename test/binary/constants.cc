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

const RefType RefType_Func{MakeAt("\x70"_su8, HT_Func), Null::No};
const RefType RefType_NullFunc{MakeAt("\x6f"_su8, HT_Func), Null::Yes};
const RefType RefType_Extern{MakeAt("\x6f"_su8, HT_Extern), Null::No};
const RefType RefType_NullExtern{MakeAt("\x68"_su8, HT_Extern), Null::Yes};
const RefType RefType_Exn{MakeAt("\x68"_su8, HT_Exn), Null::No};
const RefType RefType_NullExn{MakeAt("\x68"_su8, HT_Exn), Null::Yes};

const RT RT_Funcref{MakeAt("\x70"_su8, ReferenceKind::Funcref)};
const RT RT_Externref{MakeAt("\x6f"_su8, ReferenceKind::Externref)};
const RT RT_Exnref{MakeAt("\x68"_su8, ReferenceKind::Exnref)};
const RT RT_RefFunc{MakeAt("\x70"_su8, RefType_Func)};
const RT RT_RefNullFunc{MakeAt("\x6f"_su8, RefType_NullFunc)};
const RT RT_RefExtern{MakeAt("\x6f"_su8, RefType_Extern)};
const RT RT_RefNullExtern{MakeAt("\x68"_su8, RefType_NullExtern)};
const RT RT_RefExn{MakeAt("\x68"_su8, RefType_Exn)};
const RT RT_RefNullExn{MakeAt("\x68"_su8, RefType_NullExn)};

const VT VT_I32{MakeAt("\x7f"_su8, NumericType::I32)};
const VT VT_I64{MakeAt("\x7e"_su8, NumericType::I64)};
const VT VT_F32{MakeAt("\x7d"_su8, NumericType::F32)};
const VT VT_F64{MakeAt("\x7c"_su8, NumericType::F64)};
const VT VT_V128{MakeAt("\x7b"_su8, NumericType::V128)};
const VT VT_Funcref{MakeAt("\x70"_su8, RT_Funcref)};
const VT VT_Externref{MakeAt("\x6f"_su8, RT_Externref)};
const VT VT_Exnref{MakeAt("\x68"_su8, RT_Exnref)};
const VT VT_RefFunc{MakeAt("\x70"_su8, RT_RefFunc)};
const VT VT_RefNullFunc{MakeAt("\x6f"_su8, RT_RefNullFunc)};
const VT VT_RefExtern{MakeAt("\x6f"_su8, RT_RefExtern)};
const VT VT_RefNullExtern{MakeAt("\x68"_su8, RT_RefNullExtern)};
const VT VT_RefExn{MakeAt("\x68"_su8, RT_RefExn)};
const VT VT_RefNullExn{MakeAt("\x68"_su8, RT_RefNullExn)};

const BT BT_I32{MakeAt("\x7f"_su8, VT_I32)};
const BT BT_I64{MakeAt("\x7e"_su8, VT_I64)};
const BT BT_F32{MakeAt("\x7d"_su8, VT_F32)};
const BT BT_F64{MakeAt("\x7c"_su8, VT_F64)};
const BT BT_V128{MakeAt("\x7b"_su8, VT_V128)};
const BT BT_Funcref{MakeAt("\x70"_su8, VT_Funcref)};
const BT BT_Externref{MakeAt("\x6f"_su8, VT_Externref)};
const BT BT_Exnref{MakeAt("\x68"_su8, VT_Exnref)};
const BT BT_Void{MakeAt("\x40"_su8, binary::VoidType{})};

}  // namespace test
}  // namespace binary
}  // namespace wasp

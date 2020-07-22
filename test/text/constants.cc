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

const HT HT_Func = HT{MakeAt("func"_su8, HeapKind::Func)};
const HT HT_Extern = HT{MakeAt("extern"_su8, HeapKind::Extern)};
const HT HT_Exn = HT{MakeAt("exn"_su8, HeapKind::Exn)};

const RT RT_Funcref = RT{MakeAt("funcref"_su8, ReferenceKind::Funcref)};
const RT RT_Externref = RT{MakeAt("externref"_su8, ReferenceKind::Externref)};
const RT RT_Exnref = RT{MakeAt("exnref"_su8, ReferenceKind::Exnref)};

const VT VT_I32 = VT{MakeAt("i32"_su8, NumericType::I32)};
const VT VT_I64 = VT{MakeAt("i64"_su8, NumericType::I64)};
const VT VT_F32 = VT{MakeAt("f32"_su8, NumericType::F32)};
const VT VT_F64 = VT{MakeAt("f64"_su8, NumericType::F64)};
const VT VT_V128 = VT{MakeAt("v128"_su8, NumericType::V128)};
const VT VT_Funcref = VT{MakeAt("funcref"_su8, RT_Funcref)};
const VT VT_Externref = VT{MakeAt("externref"_su8, RT_Externref)};
const VT VT_Exnref = VT{MakeAt("exnref"_su8, RT_Exnref)};

}  // namespace test
}  // namespace text
}  // namespace wasp

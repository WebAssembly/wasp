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

const HT HT_Func = HT{MakeAt("\x70"_su8, HeapKind::Func)};
const HT HT_Extern = HT{MakeAt("\x6f"_su8, HeapKind::Extern)};
const HT HT_Exn = HT{MakeAt("\x68"_su8, HeapKind::Exn)};

const RT RT_Funcref = RT{MakeAt("\x70"_su8, ReferenceKind::Funcref)};
const RT RT_Externref = RT{MakeAt("\x6f"_su8, ReferenceKind::Externref)};
const RT RT_Exnref = RT{MakeAt("\x68"_su8, ReferenceKind::Exnref)};

const VT VT_I32 = VT{MakeAt("\x7f"_su8, NumericType::I32)};
const VT VT_I64 = VT{MakeAt("\x7e"_su8, NumericType::I64)};
const VT VT_F32 = VT{MakeAt("\x7d"_su8, NumericType::F32)};
const VT VT_F64 = VT{MakeAt("\x7c"_su8, NumericType::F64)};
const VT VT_V128 = VT{MakeAt("\x7b"_su8, NumericType::V128)};
const VT VT_Funcref = VT{MakeAt("\x70"_su8, RT_Funcref)};
const VT VT_Externref = VT{MakeAt("\x6f"_su8, RT_Externref)};
const VT VT_Exnref = VT{MakeAt("\x68"_su8, RT_Exnref)};

const BT BT_I32 = BT{MakeAt("\x7f"_su8, VT_I32)};
const BT BT_I64 = BT{MakeAt("\x7e"_su8, VT_I64)};
const BT BT_F32 = BT{MakeAt("\x7d"_su8, VT_F32)};
const BT BT_F64 = BT{MakeAt("\x7c"_su8, VT_F64)};
const BT BT_V128 = BT{MakeAt("\x7b"_su8, VT_V128)};
const BT BT_Funcref = BT{MakeAt("\x70"_su8, VT_Funcref)};
const BT BT_Externref = BT{MakeAt("\x6f"_su8, VT_Externref)};
const BT BT_Exnref = BT{MakeAt("\x68"_su8, VT_Exnref)};
const BT BT_Void = BT{MakeAt("\x40"_su8, binary::VoidType{})};

}  // namespace test
}  // namespace binary
}  // namespace wasp

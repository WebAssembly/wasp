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

#ifndef WASP_TEST_BINARY_CONSTANTS_H_
#define WASP_TEST_BINARY_CONSTANTS_H_

#include "wasp/binary/types.h"

namespace wasp {
namespace binary {
namespace test {

extern const HeapType HT_Func;
extern const HeapType HT_Extern;
extern const HeapType HT_Exn;
extern const HeapType HT_0;

extern const RefType RefType_Func;
extern const RefType RefType_NullFunc;
extern const RefType RefType_Extern;
extern const RefType RefType_NullExtern;
extern const RefType RefType_Exn;
extern const RefType RefType_NullExn;
extern const RefType RefType_0;
extern const RefType RefType_Null0;

extern const ReferenceType RT_Funcref;
extern const ReferenceType RT_Externref;
extern const ReferenceType RT_Exnref;
extern const ReferenceType RT_RefFunc;
extern const ReferenceType RT_RefNullFunc;
extern const ReferenceType RT_RefExtern;
extern const ReferenceType RT_RefNullExtern;
extern const ReferenceType RT_RefExn;
extern const ReferenceType RT_RefNullExn;
extern const ReferenceType RT_Ref0;
extern const ReferenceType RT_RefNull0;

extern const ValueType VT_I32;
extern const ValueType VT_I64;
extern const ValueType VT_F32;
extern const ValueType VT_F64;
extern const ValueType VT_V128;
extern const ValueType VT_Funcref;
extern const ValueType VT_Externref;
extern const ValueType VT_Exnref;
extern const ValueType VT_RefFunc;
extern const ValueType VT_RefNullFunc;
extern const ValueType VT_RefExtern;
extern const ValueType VT_RefNullExtern;
extern const ValueType VT_RefExn;
extern const ValueType VT_RefNullExn;
extern const ValueType VT_Ref0;
extern const ValueType VT_RefNull0;

extern const BlockType BT_I32;
extern const BlockType BT_I64;
extern const BlockType BT_F32;
extern const BlockType BT_F64;
extern const BlockType BT_V128;
extern const BlockType BT_Funcref;
extern const BlockType BT_Externref;
extern const BlockType BT_Exnref;
extern const BlockType BT_Void;

}  // namespace test
}  // namespace binary
}  // namespace wasp

#endif // WASP_TEST_BINARY_CONSTANTS_H_

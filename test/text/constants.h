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

#ifndef WASP_TEST_TEXT_CONSTANTS_H_
#define WASP_TEST_TEXT_CONSTANTS_H_

#include "wasp/text/types.h"

namespace wasp::text::test {

extern const HeapType HT_Func;
extern const HeapType HT_Extern;
extern const HeapType HT_Eq;
extern const HeapType HT_I31;
extern const HeapType HT_Any;
extern const HeapType HT_0;
extern const HeapType HT_T;

extern const RefType RefType_Func;
extern const RefType RefType_NullFunc;
extern const RefType RefType_Extern;
extern const RefType RefType_NullExtern;
extern const RefType RefType_Eq;
extern const RefType RefType_NullEq;
extern const RefType RefType_I31;
extern const RefType RefType_NullI31;
extern const RefType RefType_Any;
extern const RefType RefType_NullAny;
extern const RefType RefType_0;
extern const RefType RefType_Null0;
extern const RefType RefType_T;
extern const RefType RefType_NullT;

extern const ReferenceType RT_Funcref;
extern const ReferenceType RT_Externref;
extern const ReferenceType RT_Eqref;
extern const ReferenceType RT_I31ref;
extern const ReferenceType RT_Anyref;
extern const ReferenceType RT_RefFunc;
extern const ReferenceType RT_RefNullFunc;
extern const ReferenceType RT_RefExtern;
extern const ReferenceType RT_RefNullExtern;
extern const ReferenceType RT_RefEq;
extern const ReferenceType RT_RefNullEq;
extern const ReferenceType RT_RefI31;
extern const ReferenceType RT_RefNullI31;
extern const ReferenceType RT_RefAny;
extern const ReferenceType RT_RefNullAny;
extern const ReferenceType RT_Ref0;
extern const ReferenceType RT_RefNull0;
extern const ReferenceType RT_RefT;
extern const ReferenceType RT_RefNullT;

extern const Rtt RTT_0_Func;
extern const Rtt RTT_0_Extern;
extern const Rtt RTT_0_Eq;
extern const Rtt RTT_0_I31;
extern const Rtt RTT_0_Any;
extern const Rtt RTT_0_0;
extern const Rtt RTT_0_T;
extern const Rtt RTT_1_Func;
extern const Rtt RTT_1_Extern;
extern const Rtt RTT_1_Eq;
extern const Rtt RTT_1_I31;
extern const Rtt RTT_1_Any;
extern const Rtt RTT_1_0;
extern const Rtt RTT_1_T;


extern const ValueType VT_I32;
extern const ValueType VT_I64;
extern const ValueType VT_F32;
extern const ValueType VT_F64;
extern const ValueType VT_V128;
extern const ValueType VT_Funcref;
extern const ValueType VT_Externref;
extern const ValueType VT_Eqref;
extern const ValueType VT_I31ref;
extern const ValueType VT_Anyref;
extern const ValueType VT_RefFunc;
extern const ValueType VT_RefNullFunc;
extern const ValueType VT_RefExtern;
extern const ValueType VT_RefNullExtern;
extern const ValueType VT_RefEq;
extern const ValueType VT_RefNullEq;
extern const ValueType VT_RefI31;
extern const ValueType VT_RefNullI31;
extern const ValueType VT_RefAny;
extern const ValueType VT_RefNullAny;
extern const ValueType VT_Ref0;
extern const ValueType VT_RefNull0;
extern const ValueType VT_RefT;
extern const ValueType VT_RefNullT;
extern const ValueType VT_RTT_0_Func;
extern const ValueType VT_RTT_0_Extern;
extern const ValueType VT_RTT_0_Eq;
extern const ValueType VT_RTT_0_I31;
extern const ValueType VT_RTT_0_Any;
extern const ValueType VT_RTT_0_0;
extern const ValueType VT_RTT_0_T;
extern const ValueType VT_RTT_1_Func;
extern const ValueType VT_RTT_1_Extern;
extern const ValueType VT_RTT_1_Eq;
extern const ValueType VT_RTT_1_I31;
extern const ValueType VT_RTT_1_Any;
extern const ValueType VT_RTT_1_0;
extern const ValueType VT_RTT_1_T;


}  // namespace wasp::text::test

#endif // WASP_TEST_TEXT_CONSTANTS_H_

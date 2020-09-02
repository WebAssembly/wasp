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

#include "wasp/valid/match.h"

#include <cassert>

#include "gtest/gtest.h"

#include "test/binary/constants.h"
#include "test/valid/test_utils.h"
#include "wasp/base/concat.h"
#include "wasp/base/enumerate.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"

using namespace ::wasp;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;
using namespace ::wasp::binary::test;

class ValidMatchTest : public ::testing::Test {
 protected:
  void PushFunctionType(const binary::ValueTypeList& params,
                        const binary::ValueTypeList& results) {
    context.types.push_back(
        binary::DefinedType{binary::FunctionType{params, results}});
  }

  void PushStructType(const binary::StructType& struct_type) {
    context.types.push_back(binary::DefinedType{struct_type});
  }

  void PushArrayType(const binary::ArrayType& array_type) {
    context.types.push_back(binary::DefinedType{array_type});
  }

  TestErrors errors;
  Context context{errors};
};

enum Comparison {
  SAME,
  DIFF,
  SKIP,

  // Nice for tables below
  MTCH = SAME,
  ____ = DIFF,

  // Nice for small tables below
  S = SAME,
  M = MTCH,
  _ = ____,
};

template <typename T>
void DoTable(Context& context,
             std::vector<T> ivalues,
             std::vector<T> jvalues,
             std::vector<Comparison> results,
             bool (&func)(Context&, const T&, const T&)) {
  assert(ivalues.size() * jvalues.size() == results.size());

  int r = 0;
  for (auto [j, vj] : enumerate(jvalues)) {
    for (auto [i, vi] : enumerate(ivalues)) {
      Comparison cmp = results[r++];
      if (cmp == SKIP) {
        continue;
      }
      bool result = cmp == SAME;
      EXPECT_EQ(result, func(context, vi, vj))
          << concat("i:", vi, " j:", vj, " should be ", result);
    }
  }
}

template <typename T>
void IsSameTable(Context& context,
                 std::vector<T> ivalues,
                 std::vector<T> jvalues,
                 std::vector<Comparison> results) {
  DoTable(context, ivalues, jvalues, results, IsSame);
}

template <typename T>
void IsSameTable(Context& context,
                 std::vector<T> values,
                 std::vector<Comparison> results) {
  IsSameTable(context, values, values, results);
}

template <typename T>
void IsSameDiagonal(Context& context, std::vector<T> values) {
  // Build a table where only the values on the diagonal are SAME.
  size_t size = values.size();
  std::vector<Comparison> results(size * size, DIFF);
  for (size_t i = 0; i < size; ++i) {
    results[i * size + i] = SAME;
  }

  IsSameTable(context, values, values, results);
}

template <typename T>
void IsMatchTable(Context& context,
                  std::vector<T> ivalues,
                  std::vector<T> jvalues,
                  std::vector<Comparison> results) {
  DoTable(context, ivalues, jvalues, results, IsMatch);
}

template <typename T>
void IsMatchTable(Context& context,
                  std::vector<T> values,
                  std::vector<Comparison> results) {
  IsMatchTable(context, values, values, results);
}

TEST_F(ValidMatchTest, IsSame_HeapType_Simple) {
  std::vector<binary::HeapType> heap_types = {HT_Func, HT_Extern, HT_Any, HT_Eq,
                                              HT_I31,  HT_Exn,    HT_0};
  IsSameDiagonal(context, heap_types);
}

TEST_F(ValidMatchTest, IsSame_RefType_Simple) {
  std::vector<binary::RefType> ref_types = {
      RefType_Func,   RefType_NullFunc,    //
      RefType_Extern, RefType_NullExtern,  //
      RefType_Any,    RefType_NullAny,     //
      RefType_Eq,     RefType_NullEq,      //
      RefType_I31,    RefType_NullI31,     //
      RefType_Exn,    RefType_NullExn,     //
      RefType_0,      RefType_Null0,       //
  };
  IsSameDiagonal(context, ref_types);
}

TEST_F(ValidMatchTest, IsSame_ReferenceType_Simple) {
  std::vector<binary::ReferenceType> reference_types = {
      RT_Funcref,   RT_Externref,     RT_Anyref, RT_Eqref, RT_I31ref, RT_Exnref,

      RT_RefFunc,   RT_RefNullFunc,    //
      RT_RefExtern, RT_RefNullExtern,  //
      RT_RefAny,    RT_RefNullAny,     //
      RT_RefEq,     RT_RefNullEq,      //
      RT_RefI31,    RT_RefNullI31,     //
      RT_RefExn,    RT_RefNullExn,     //
      RT_Ref0,      RT_RefNull0,       //
  };
  IsSameTable(
      context, reference_types,
      {
          /*                   n     n     n           n     n
          F  E                 u     u     u     n     u     u
          u  x  A     I  E     l     l     l     u     l     l     n
          n  t  n  E  3  x     l     l     l     l     l     l     u
          c  .  y  q  1  n  f  f                 l                 l
          r  r  r  r  r  r  u  u  e  e  a  a        i  i  e  e     l
          e  e  e  e  e  e  n  n  x  x  n  n  e  e  3  3  x  x
          f  f  f  f  f  f  c  c  t  t  y  y  q  q  1  1  n  n  0  0   */
          S, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _,  // Funcref
          _, S, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _,  // Externref
          _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _,  // Anyref
          _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _,  // Eqref
          _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _,  // I31ref
          _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, S, _, _,  // Exnref
          _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _,  // ref func
          S, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _,  // ref null func
          _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _,  // ref extern
          _, S, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _,  // ref null extern
          _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _,  // ref any
          _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _,  // ref null any
          _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _,  // ref eq
          _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _,  // ref null eq
          _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _,  // ref i31
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _,  // ref null i31
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _,  // ref exn
          _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, S, _, _,  // ref null exn
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _,  // ref 0
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S,  // ref null 0
      });
}

TEST_F(ValidMatchTest, IsSame_Rtt) {
  std::vector<binary::ValueType> rtts = {
      VT_RTT_0_Func, VT_RTT_0_Extern, VT_RTT_0_Exn, VT_RTT_0_Any,
      VT_RTT_0_Eq,   VT_RTT_0_I31,    VT_RTT_0_0,
  };

  IsSameTable(context, rtts,
              {
                  /*
                  Func  Ext.  Exn   Any   Eq    I31   0     */
                  SAME, ____, ____, ____, ____, ____, ____,  // Func
                  ____, SAME, ____, ____, ____, ____, ____,  // Extern
                  ____, ____, SAME, ____, ____, ____, ____,  // Exn
                  ____, ____, ____, SAME, ____, ____, ____,  // Any
                  ____, ____, ____, ____, SAME, ____, ____,  // Eq
                  ____, ____, ____, ____, ____, SAME, ____,  // I31
                  ____, ____, ____, ____, ____, ____, SAME,  // 0
              });
}

TEST_F(ValidMatchTest, IsSame_ValueType_Simple) {
  std::vector<binary::ValueType> value_types{
      VT_I32,       VT_I64,           VT_F32,       VT_F64,
      VT_V128,      VT_Funcref,       VT_Externref, VT_Anyref,
      VT_Eqref,     VT_I31ref,        VT_Exnref,

      VT_RefFunc,   VT_RefNullFunc,    //
      VT_RefExtern, VT_RefNullExtern,  //
      VT_RefAny,    VT_RefNullAny,     //
      VT_RefEq,     VT_RefNullEq,      //
      VT_RefI31,    VT_RefNullI31,     //
      VT_RefExn,    VT_RefNullExn,     //
      VT_Ref0,      VT_RefNull0,       //
  };
  IsSameTable(
      context, value_types,
      {
          /*                                  n     n     n           n     n
                         F  E                 u     u     u     n     u     u
                         u  x  A     I  E     l     l     l     u     l     l     n
                         n  t  n  E  3  x     l     l     l     l     l     l     u
                      v  c  .  y  q  1  n  f  f                 l                 l
          i  i  f  f  1  r  r  r  r  r  r  u  u  e  e  a  a        i  i  e  e     l
          3  6  3  6  2  e  e  e  e  e  e  n  n  x  x  n  n  e  e  3  3  x  x
          2  4  2  4  8  f  f  f  f  f  f  c  c  t  t  y  y  q  q  1  1  n  n  0  0   */
          S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // I32
          _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // I64
          _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F32
          _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F64
          _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // V128
          _, _, _, _, _, S, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, // Funcref
          _, _, _, _, _, _, S, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // Externref
          _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, // Anyref
          _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, // Eqref
          _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, // I31ref
          _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, S, _, _, // Exnref
          _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, // ref func
          _, _, _, _, _, S, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, // ref null func
          _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, // ref extern
          _, _, _, _, _, _, S, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // ref null extern
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, // ref any
          _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, // ref null any
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, // ref eq
          _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, // ref null eq
          _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, // ref i31
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, // ref null i31
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, _, _, // ref exn
          _, _, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, S, _, _, // ref null exn
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, _, // ref 0
          _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, S, // ref null 0
      });
}

TEST_F(ValidMatchTest, IsMatch_HeapType_Simple) {
  std::vector<binary::HeapType> heap_types = {HT_Func, HT_Extern, HT_Any, HT_Eq,
                                              HT_I31,  HT_Exn,    HT_0,   HT_1};

  PushFunctionType({}, {});              // type 0
  PushStructType(binary::StructType{});  // type 1

  IsMatchTable(context, heap_types,
               {
                   /*
                   Func  Ext.  Any   Eq    I31   Exn.  0     1     */
                   SAME, ____, MTCH, ____, ____, ____, ____, ____,  // Func
                   ____, SAME, MTCH, ____, ____, ____, ____, ____,  // Extern
                   ____, ____, SAME, ____, ____, ____, ____, ____,  // Any
                   ____, ____, MTCH, SAME, ____, ____, ____, ____,  // Eq
                   ____, ____, MTCH, MTCH, SAME, ____, ____, ____,  // I31
                   ____, ____, MTCH, ____, ____, SAME, ____, ____,  // Exn
                   MTCH, ____, MTCH, ____, ____, ____, SAME, ____,  // 0
                   ____, ____, MTCH, MTCH, ____, ____, ____, SAME,  // 1
               });
}

TEST_F(ValidMatchTest, IsMatch_EqFuncType) {
  PushFunctionType({}, {});
  // Function types are not subtypes of eq.
  EXPECT_FALSE(IsMatch(context, VT_RefEq, VT_Ref0));
}

TEST_F(ValidMatchTest, IsMatch_RefType_Simple) {
  std::vector<binary::RefType> ref_types = {
      RefType_Func,   RefType_NullFunc,    //
      RefType_Extern, RefType_NullExtern,  //
      RefType_Any,    RefType_NullAny,     //
      RefType_Eq,     RefType_NullEq,      //
      RefType_I31,    RefType_NullI31,     //
      RefType_Exn,    RefType_NullExn,     //
      RefType_0,      RefType_Null0,       //
      RefType_1,      RefType_Null1,       //
  };

  PushFunctionType({}, {});              // type 0
  PushStructType(binary::StructType{});  // type 1

  IsMatchTable(
      context, ref_types,
      {
          /* n     n     n           n     n
             u     u     u     n     u     u
             l     l     l     u     l     l     n     n
             l     l     l     l     l     l     u     u
          f  f                 l                 l     l
          u  u  e  e  a  a        i  i  e  e     l     l
          n  n  x  x  n  n  e  e  3  3  x  x
          c  c  t  t  y  y  q  q  1  1  n  n  0  0  1  1  */
          S, M, _, _, M, M, _, _, _, _, _, _, _, _, _, _,  // ref func
          _, S, _, _, _, M, _, _, _, _, _, _, _, _, _, _,  // ref null func
          _, _, S, M, M, M, _, _, _, _, _, _, _, _, _, _,  // ref extern
          _, _, _, S, _, M, _, _, _, _, _, _, _, _, _, _,  // ref null extern
          _, _, _, _, S, M, _, _, _, _, _, _, _, _, _, _,  // ref any
          _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _,  // ref null any
          _, _, _, _, M, M, S, M, _, _, _, _, _, _, _, _,  // ref eq
          _, _, _, _, _, M, _, S, _, _, _, _, _, _, _, _,  // ref null eq
          _, _, _, _, M, M, M, M, S, M, _, _, _, _, _, _,  // ref i31
          _, _, _, _, _, M, _, M, _, S, _, _, _, _, _, _,  // ref null i31
          _, _, _, _, M, M, _, _, _, _, S, M, _, _, _, _,  // ref exn
          _, _, _, _, _, M, _, _, _, _, _, S, _, _, _, _,  // ref null exn
          M, M, _, _, M, M, _, _, _, _, _, _, S, M, _, _,  // ref 0
          _, M, _, _, _, M, _, _, _, _, _, _, _, S, _, _,  // ref null 0
          _, _, _, _, M, M, M, M, _, _, _, _, _, _, S, M,  // ref 1
          _, _, _, _, _, M, _, M, _, _, _, _, _, _, _, S,  // ref null 1
      });
}

TEST_F(ValidMatchTest, IsMatch_ReferenceType_Simple) {
  std::vector<binary::ReferenceType> reference_types = {
      RT_Funcref,   RT_Externref,     RT_Anyref, RT_Eqref, RT_I31ref, RT_Exnref,

      RT_RefFunc,   RT_RefNullFunc,    //
      RT_RefExtern, RT_RefNullExtern,  //
      RT_RefAny,    RT_RefNullAny,     //
      RT_RefEq,     RT_RefNullEq,      //
      RT_RefI31,    RT_RefNullI31,     //
      RT_RefExn,    RT_RefNullExn,     //
      RT_Ref0,      RT_RefNull0,       //
      RT_Ref1,      RT_RefNull1,       //
  };

  PushFunctionType({}, {});              // type 0
  PushStructType(binary::StructType{});  // type 1

  IsMatchTable(
      context, reference_types,
      {
          /*                   n     n     n           n     n
          F  E                 u     u     u     n     u     u
          u  x  A     I  E     l     l     l     u     l     l     n     n
          n  t  n  E  3  x     l     l     l     l     l     l     u     u
          c  .  y  q  1  n  f  f                 l                 l     l
          r  r  r  r  r  r  u  u  e  e  a  a        i  i  e  e     l     l
          e  e  e  e  e  e  n  n  x  x  n  n  e  e  3  3  x  x
          f  f  f  f  f  f  c  c  t  t  y  y  q  q  1  1  n  n  0  0  1  1  */
          S, _, M, _, _, _, _, S, _, _, _, M, _, _, _, _, _, _, _, _, _, _, // Funcref
          _, S, M, _, _, _, _, _, _, S, _, M, _, _, _, _, _, _, _, _, _, _, // Externref
          _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // Anyref
          _, _, M, S, _, _, _, _, _, _, _, M, _, S, _, _, _, _, _, _, _, _, // Eqref
          _, _, M, M, S, _, _, _, _, _, M, M, M, M, S, M, _, _, _, _, _, _, // I31ref
          _, _, M, _, _, S, _, _, _, _, _, M, _, _, _, _, _, S, _, _, _, _, // Exnref
          M, _, M, _, _, _, S, M, _, _, M, M, _, _, _, _, _, _, _, _, _, _, // ref func
          S, _, M, _, _, _, _, S, _, _, _, M, _, _, _, _, _, _, _, _, _, _, // ref null func
          _, M, M, _, _, _, _, _, S, M, M, M, _, _, _, _, _, _, _, _, _, _, // ref extern
          _, S, M, _, _, _, _, _, _, S, _, M, _, _, _, _, _, _, _, _, _, _, // ref null extern
          _, _, M, _, _, _, _, _, _, _, S, M, _, _, _, _, _, _, _, _, _, _, // ref any
          _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // ref null any
          _, _, M, M, _, _, _, _, _, _, M, M, S, M, _, _, _, _, _, _, _, _, // ref eq
          _, _, M, S, _, _, _, _, _, _, _, M, _, S, _, _, _, _, _, _, _, _, // ref null eq
          _, _, M, M, S, _, _, _, _, _, M, M, M, M, S, M, _, _, _, _, _, _, // ref i31
          _, _, M, M, _, _, _, _, _, _, _, M, _, M, _, S, _, _, _, _, _, _, // ref null i31
          _, _, M, _, _, M, _, _, _, _, M, M, _, _, _, _, S, M, _, _, _, _, // ref exn
          _, _, M, _, _, S, _, _, _, _, _, M, _, _, _, _, _, S, _, _, _, _, // ref null exn
          M, _, M, _, _, _, M, M, _, _, M, M, _, _, _, _, _, _, S, M, _, _, // ref 0
          M, _, M, _, _, _, _, M, _, _, _, M, _, _, _, _, _, _, _, S, _, _, // ref null 0
          _, _, M, M, _, _, _, _, _, _, M, M, M, M, _, _, _, _, _, _, S, M, // ref 1
          _, _, M, M, _, _, _, _, _, _, _, M, _, M, _, _, _, _, _, _, _, S, // ref null 1
      });
}

TEST_F(ValidMatchTest, IsMatch_Rtt) {
  std::vector<binary::ValueType> rtts = {
      VT_RTT_0_Func, VT_RTT_0_Extern, VT_RTT_0_Exn, VT_RTT_0_Any,
      VT_RTT_0_Eq,   VT_RTT_0_I31,    VT_RTT_0_0,
  };

  IsMatchTable(context, rtts,
               {
                   /*
                   Func  Ext.  Exn   Any   Eq    I31   0     */
                   SAME, ____, ____, ____, ____, ____, ____,  // Func
                   ____, SAME, ____, ____, ____, ____, ____,  // Extern
                   ____, ____, SAME, ____, ____, ____, ____,  // Exn
                   ____, ____, ____, SAME, ____, ____, ____,  // Any
                   ____, ____, ____, ____, SAME, ____, ____,  // Eq
                   ____, ____, ____, ____, ____, SAME, ____,  // I31
                   ____, ____, ____, ____, ____, ____, SAME,  // 0
               });
}

TEST_F(ValidMatchTest, IsMatch_ValueType_Simple) {
  std::vector<binary::ValueType> value_types{
      VT_I32,       VT_I64,           VT_F32,       VT_F64,
      VT_V128,      VT_Funcref,       VT_Externref, VT_Anyref,
      VT_Eqref,     VT_I31ref,        VT_Exnref,

      VT_RefFunc,   VT_RefNullFunc,    //
      VT_RefExtern, VT_RefNullExtern,  //
      VT_RefAny,    VT_RefNullAny,     //
      VT_RefEq,     VT_RefNullEq,      //
      VT_RefI31,    VT_RefNullI31,     //
      VT_RefExn,    VT_RefNullExn,     //
      VT_Ref0,      VT_RefNull0,       //
      VT_Ref1,      VT_RefNull1,       //
  };

  PushFunctionType({}, {});              // type 0
  PushStructType(binary::StructType{});  // type 1

  IsMatchTable(
      context, value_types,
      {
          /*                                  n     n     n           n     n
                         F  E                 u     u     u     n     u     u
                         u  x  A     I  E     l     l     l     u     l     l     n     n
                         n  t  n  E  3  x     l     l     l     l     l     l     u     u
                      v  c  .  y  q  1  n  f  f                 l                 l     l
          i  i  f  f  1  r  r  r  r  r  r  u  u  e  e  a  a        i  i  e  e     l     l
          3  6  3  6  2  e  e  e  e  e  e  n  n  x  x  n  n  e  e  3  3  x  x
          2  4  2  4  8  f  f  f  f  f  f  c  c  t  t  y  y  q  q  1  1  n  n  0  0  1  1   */
          S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // I32
          _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // I64
          _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F32
          _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // F64
          _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, // V128
          _, _, _, _, _, S, _, M, _, _, _, _, S, _, _, _, M, _, _, _, _, _, _, _, _, _, _, // Funcref
          _, _, _, _, _, _, S, M, _, _, _, _, _, _, S, _, M, _, _, _, _, _, _, _, _, _, _, // Externref
          _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // Anyref
          _, _, _, _, _, _, _, M, S, _, _, _, _, _, _, _, M, _, S, _, _, _, _, _, _, _, _, // Eqref
          _, _, _, _, _, _, _, M, M, S, _, _, _, _, _, M, M, M, M, S, M, _, _, _, _, _, _, // I31ref
          _, _, _, _, _, _, _, M, _, _, S, _, _, _, _, _, M, _, _, _, _, _, S, _, _, _, _, // Exnref
          _, _, _, _, _, M, _, M, _, _, _, S, M, _, _, M, M, _, _, _, _, _, _, _, _, _, _, // ref func
          _, _, _, _, _, S, _, M, _, _, _, _, S, _, _, _, M, _, _, _, _, _, _, _, _, _, _, // ref null func
          _, _, _, _, _, _, M, M, _, _, _, _, _, S, M, M, M, _, _, _, _, _, _, _, _, _, _, // ref extern
          _, _, _, _, _, _, S, M, _, _, _, _, _, _, S, _, M, _, _, _, _, _, _, _, _, _, _, // ref null extern
          _, _, _, _, _, _, _, M, _, _, _, _, _, _, _, S, M, _, _, _, _, _, _, _, _, _, _, // ref any
          _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, S, _, _, _, _, _, _, _, _, _, _, // ref null any
          _, _, _, _, _, _, _, M, M, _, _, _, _, _, _, M, M, S, M, _, _, _, _, _, _, _, _, // ref eq
          _, _, _, _, _, _, _, M, S, _, _, _, _, _, _, _, M, _, S, _, _, _, _, _, _, _, _, // ref null eq
          _, _, _, _, _, _, _, M, M, S, _, _, _, _, _, M, M, M, M, S, M, _, _, _, _, _, _, // ref i31
          _, _, _, _, _, _, _, M, M, _, _, _, _, _, _, _, M, _, M, _, S, _, _, _, _, _, _, // ref null i31
          _, _, _, _, _, _, _, M, _, _, M, _, _, _, _, M, M, _, _, _, _, S, M, _, _, _, _, // ref exn
          _, _, _, _, _, _, _, M, _, _, S, _, _, _, _, _, M, _, _, _, _, _, S, _, _, _, _, // ref null exn
          _, _, _, _, _, M, _, M, _, _, _, M, M, _, _, M, M, _, _, _, _, _, _, S, M, _, _, // ref 0
          _, _, _, _, _, M, _, M, _, _, _, _, M, _, _, _, M, _, _, _, _, _, _, _, S, _, _, // ref null 0
          _, _, _, _, _, _, _, M, M, _, _, _, _, _, _, M, M, M, M, _, _, _, _, _, _, S, M, // ref 1
          _, _, _, _, _, _, _, M, M, _, _, _, _, _, _, _, M, _, M, _, _, _, _, _, _, _, S, // ref null 1
      });
}

TEST_F(ValidMatchTest, IsSame_ValueType_Var) {
  context.equivalent_types.Reset(3);
  PushFunctionType({VT_F32}, {});  // 0
  PushFunctionType({VT_F32}, {});  // 1
  PushFunctionType({VT_I32}, {});  // 2

  EXPECT_TRUE(IsSame(context, VT_Ref0, VT_Ref1));
  EXPECT_FALSE(IsSame(context, VT_Ref0, VT_Ref2));
  EXPECT_FALSE(IsSame(context, VT_Ref1, VT_Ref2));
}

TEST_F(ValidMatchTest, IsSame_ValueType_VarRecursive) {
  context.equivalent_types.Reset(3);
  PushFunctionType({}, {VT_Ref0});        // 0
  PushFunctionType({}, {VT_Ref1});        // 1
  PushFunctionType({VT_I32}, {VT_Ref0});  // 2

  EXPECT_TRUE(IsSame(context, VT_Ref0, VT_Ref1));
  EXPECT_FALSE(IsSame(context, VT_Ref0, VT_Ref2));
  EXPECT_FALSE(IsSame(context, VT_Ref1, VT_Ref2));
}

TEST_F(ValidMatchTest, IsSame_ValueType_VarMutuallyRecursive) {
  context.equivalent_types.Reset(3);
  PushFunctionType({VT_I32}, {VT_Ref0});  // 0
  PushFunctionType({VT_I32}, {VT_Ref2});  // 1
  PushFunctionType({VT_I32}, {VT_Ref1});  // 2

  EXPECT_TRUE(IsSame(context, VT_Ref0, VT_Ref1));
  EXPECT_TRUE(IsSame(context, VT_Ref0, VT_Ref2));
  EXPECT_TRUE(IsSame(context, VT_Ref1, VT_Ref2));
}

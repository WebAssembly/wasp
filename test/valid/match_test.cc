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
#include "wasp/valid/valid_ctx.h"

using namespace ::wasp;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

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

class ValidMatchTest : public ::testing::Test {
 protected:
  void PushFunctionType(const ValueTypeList& params,
                        const ValueTypeList& results) {
    ctx.types.push_back(DefinedType{FunctionType{params, results}});
  }

  void PushStructType(const StructType& struct_type) {
    ctx.types.push_back(DefinedType{struct_type});
  }

  void PushArrayType(const ArrayType& array_type) {
    ctx.types.push_back(DefinedType{array_type});
  }

  auto MakeDiagonalMatrix(size_t size) -> std::vector<Comparison>  {
    std::vector<Comparison> results(size * size, DIFF);
    for (size_t i = 0; i < size; ++i) {
      results[i * size + i] = SAME;
    }
    return results;
  }

  template <typename T>
  void DoTable(std::vector<T> ivalues,
               std::vector<T> jvalues,
               std::vector<Comparison> results,
               bool (&func)(ValidCtx&, const T&, const T&)) {
    assert(ivalues.size() * jvalues.size() == results.size());

    int r = 0;
    for (auto [j, vj] : enumerate(jvalues)) {
      for (auto [i, vi] : enumerate(ivalues)) {
        Comparison cmp = results[r++];
        if (cmp == SKIP) {
          continue;
        }
        bool result = cmp == SAME;
        EXPECT_EQ(result, func(ctx, vi, vj))
            << concat("i:", vi, " j:", vj, " should be ", result);
      }
    }
  }

  template <typename T>
  void IsSameTable(std::vector<T> ivalues,
                   std::vector<T> jvalues,
                   std::vector<Comparison> results) {
    DoTable(ivalues, jvalues, results, IsSame);
  }

  template <typename T>
  void IsSameTable(std::vector<T> values, std::vector<Comparison> results) {
    IsSameTable(values, values, results);
  }

  template <typename T>
  void IsSameDistinct(std::vector<T> values) {
    IsSameTable(values, values, MakeDiagonalMatrix(values.size()));
  }

  template <typename T>
  void IsMatchTable(std::vector<T> ivalues,
                    std::vector<T> jvalues,
                    std::vector<Comparison> results) {
    DoTable(ivalues, jvalues, results, IsMatch);
  }

  template <typename T>
  void IsMatchDistinct(std::vector<T> ivalues, std::vector<T> jvalues) {
    std::vector<Comparison> results(ivalues.size() * jvalues.size(), DIFF);
    // Check in both directions.
    DoTable(ivalues, jvalues, results, IsMatch);
    DoTable(jvalues, ivalues, results, IsMatch);
  }

  template <typename T>
  void IsMatchTable(std::vector<T> values, std::vector<Comparison> results) {
    IsMatchTable(values, values, results);
  }

  template <typename T>
  void IsMatchDistinct(std::vector<T> values) {
    IsMatchTable(values, MakeDiagonalMatrix(values.size()));
  }

  TestErrors errors;
  ValidCtx ctx{errors};
};

TEST_F(ValidMatchTest, IsSame_HeapType_Simple) {
  std::vector<HeapType> types{HT_Func, HT_Extern, HT_Any, HT_Eq,
                              HT_I31,  HT_Exn,    HT_0};
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_RefType_Simple) {
  std::vector<RefType> types{
      RefType_Func,   RefType_NullFunc,    //
      RefType_Extern, RefType_NullExtern,  //
      RefType_Any,    RefType_NullAny,     //
      RefType_Eq,     RefType_NullEq,      //
      RefType_I31,    RefType_NullI31,     //
      RefType_Exn,    RefType_NullExn,     //
      RefType_0,      RefType_Null0,       //
  };
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_ReferenceType_Simple) {
  std::vector<ReferenceType> types{
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
      types, {
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
  std::vector<ValueType> rtts{
      VT_RTT_0_Func, VT_RTT_0_Extern, VT_RTT_0_Exn, VT_RTT_0_Any,
      VT_RTT_0_Eq,   VT_RTT_0_I31,    VT_RTT_0_0,
  };

  IsSameDistinct(rtts);
}

TEST_F(ValidMatchTest, IsSame_ValueType_Simple) {
  std::vector<ValueType> types{
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
      types,
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

TEST_F(ValidMatchTest, IsSame_ValueType_Var) {
  ctx.same_types.Reset(3);
  PushFunctionType({VT_F32}, {});  // 0
  PushFunctionType({VT_F32}, {});  // 1
  PushFunctionType({VT_I32}, {});  // 2

  EXPECT_TRUE(IsSame(ctx, VT_Ref0, VT_Ref1));
  EXPECT_FALSE(IsSame(ctx, VT_Ref0, VT_Ref2));
  EXPECT_FALSE(IsSame(ctx, VT_Ref1, VT_Ref2));
}

TEST_F(ValidMatchTest, IsSame_ValueType_VarRecursive) {
  ctx.same_types.Reset(3);
  PushFunctionType({}, {VT_Ref0});        // 0
  PushFunctionType({}, {VT_Ref1});        // 1
  PushFunctionType({VT_I32}, {VT_Ref0});  // 2

  EXPECT_TRUE(IsSame(ctx, VT_Ref0, VT_Ref1));
  EXPECT_FALSE(IsSame(ctx, VT_Ref0, VT_Ref2));
  EXPECT_FALSE(IsSame(ctx, VT_Ref1, VT_Ref2));
}

TEST_F(ValidMatchTest, IsSame_ValueType_VarMutuallyRecursive) {
  ctx.same_types.Reset(3);
  PushFunctionType({VT_I32}, {VT_Ref0});  // 0
  PushFunctionType({VT_I32}, {VT_Ref2});  // 1
  PushFunctionType({VT_I32}, {VT_Ref1});  // 2

  EXPECT_TRUE(IsSame(ctx, VT_Ref0, VT_Ref1));
  EXPECT_TRUE(IsSame(ctx, VT_Ref0, VT_Ref2));
  EXPECT_TRUE(IsSame(ctx, VT_Ref1, VT_Ref2));
}

TEST_F(ValidMatchTest, IsSame_StorageType) {
  std::vector<StorageType> types{
      StorageType{VT_I32},
      StorageType{PackedType::I8},
      StorageType{PackedType::I16},
  };
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_FieldType) {
  std::vector<FieldType> types{
      FieldType{StorageType{VT_I32}, Mutability::Const},
      FieldType{StorageType{VT_Ref0}, Mutability::Const},
      FieldType{StorageType{VT_I32}, Mutability::Var},
      FieldType{StorageType{VT_Ref0}, Mutability::Var},
  };
  PushFunctionType({}, {});  // 0
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_FunctionType) {
  std::vector<FunctionType> types{
      FunctionType{{}, {}},
      FunctionType{{VT_I32}, {VT_F32}},
      FunctionType{{VT_Ref0, VT_Ref0}, {VT_Ref0}},
  };
  PushFunctionType({}, {});  // 0
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_StructType) {
  std::vector<StructType> types{
      StructType{},
      StructType{
          FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const}}},
      StructType{
          FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Var}}},
      StructType{
          FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Var}}},
      StructType{FieldTypeList{
          FieldType{StorageType{VT_I32}, Mutability::Const},
          FieldType{StorageType{PackedType::I8}, Mutability::Var}}},
  };
  PushFunctionType({}, {});  // 0
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_ArrayType) {
  std::vector<ArrayType> types{
      ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}},
      ArrayType{FieldType{StorageType{VT_I32}, Mutability::Var}},
      ArrayType{FieldType{StorageType{VT_Ref0}, Mutability::Const}},
  };
  PushFunctionType({}, {});  // 0
  IsSameDistinct(types);
}

TEST_F(ValidMatchTest, IsSame_DefinedType) {
  std::vector<DefinedType> types{
      DefinedType{FunctionType{}},
      DefinedType{StructType{}},
      DefinedType{ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}}},
  };
  IsSameDistinct(types);
}


TEST_F(ValidMatchTest, IsMatch_HeapType_Simple) {
  std::vector<HeapType> types{HT_Func, HT_Extern, HT_Any, HT_Eq,
                              HT_I31,  HT_Exn,    HT_0,   HT_1};

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchTable(types,
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

TEST_F(ValidMatchTest, IsMatch_RefType_Simple) {
  std::vector<RefType> types{
      RefType_Func,   RefType_NullFunc,    //
      RefType_Extern, RefType_NullExtern,  //
      RefType_Any,    RefType_NullAny,     //
      RefType_Eq,     RefType_NullEq,      //
      RefType_I31,    RefType_NullI31,     //
      RefType_Exn,    RefType_NullExn,     //
      RefType_0,      RefType_Null0,       //
      RefType_1,      RefType_Null1,       //
  };

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchTable(
      types,
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
  std::vector<ReferenceType> types{
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
  PushStructType(StructType{});  // type 1

  IsMatchTable(
      types,
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
  std::vector<ValueType> rtts{
      VT_RTT_0_Func, VT_RTT_0_Extern, VT_RTT_0_Exn, VT_RTT_0_Any,
      VT_RTT_0_Eq,   VT_RTT_0_I31,    VT_RTT_0_0,
  };

  IsMatchDistinct(rtts);
}

TEST_F(ValidMatchTest, IsMatch_ValueType_Simple) {
  std::vector<ValueType> numeric_types{
      VT_I32, VT_I64, VT_F32, VT_F64, VT_V128,
  };

  std::vector<ValueType> reference_types{
      VT_Funcref,   VT_Externref,     VT_Anyref, VT_Eqref, VT_I31ref, VT_Exnref,

      VT_RefFunc,   VT_RefNullFunc,    //
      VT_RefExtern, VT_RefNullExtern,  //
      VT_RefAny,    VT_RefNullAny,     //
      VT_RefEq,     VT_RefNullEq,      //
      VT_RefI31,    VT_RefNullI31,     //
      VT_RefExn,    VT_RefNullExn,     //
      VT_Ref0,      VT_RefNull0,       //
      VT_Ref1,      VT_RefNull1,       //
  };

  std::vector<ValueType> rtts{
      VT_RTT_0_Func, VT_RTT_0_Extern, VT_RTT_0_Any, VT_RTT_0_Eq,
      VT_RTT_0_I31,  VT_RTT_0_Exn,    VT_RTT_0_0,
  };

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchDistinct(numeric_types, reference_types);
  IsMatchDistinct(numeric_types, rtts);
  IsMatchDistinct(reference_types, rtts);

  IsMatchDistinct(rtts);
  IsMatchDistinct(numeric_types);
}

TEST_F(ValidMatchTest, IsMatch_StorageType) {
  std::vector<StorageType> types{
      StorageType{VT_I32},          StorageType{VT_Ref0},
      StorageType{VT_Ref1},         StorageType{PackedType::I8},
      StorageType{PackedType::I16},
  };

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_FieldType_Simple) {
  std::vector<FieldType> types{
      FieldType{StorageType{VT_I32}, Mutability::Const},
      FieldType{StorageType{VT_I32}, Mutability::Var},
      FieldType{StorageType{VT_Ref0}, Mutability::Const},
      FieldType{StorageType{VT_Ref0}, Mutability::Var},
      FieldType{StorageType{VT_Ref1}, Mutability::Const},
      FieldType{StorageType{VT_Ref1}, Mutability::Var},
      FieldType{StorageType{PackedType::I8}, Mutability::Const},
      FieldType{StorageType{PackedType::I8}, Mutability::Var},
  };

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_FieldType_Subtyping) {
  PushFunctionType({}, {});      // type 0

  // Only const fields are covariant.
  EXPECT_TRUE(IsMatch(ctx,
                      FieldType{StorageType{VT_Funcref}, Mutability::Const},
                      FieldType{StorageType{VT_Ref0}, Mutability::Const}));
  EXPECT_FALSE(IsMatch(ctx,
                       FieldType{StorageType{VT_Funcref}, Mutability::Var},
                       FieldType{StorageType{VT_Ref0}, Mutability::Var}));
}

TEST_F(ValidMatchTest, IsMatch_FieldTypeList) {
  std::vector<FieldTypeList> types{
      // (field i32)
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const}},
      // (field (mut i32))
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Var}},
      // (field i64 i32)
      FieldTypeList{FieldType{StorageType{VT_I64}, Mutability::Const},
                    FieldType{StorageType{VT_I32}, Mutability::Const}},
      // (field (mut i64) i32)
      FieldTypeList{FieldType{StorageType{VT_I64}, Mutability::Var},
                    FieldType{StorageType{VT_I32}, Mutability::Const}},
      // (field (mut (ref 0)))
      FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Var}},
      // (field (mut (ref 1)))
      FieldTypeList{FieldType{StorageType{VT_Ref1}, Mutability::Var}},
  };

  PushFunctionType({}, {});      // type 0
  PushStructType(StructType{});  // type 1

  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_FieldTypeList_Subtyping) {
  const auto FTL_i32 =
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const}};
  const auto FTL_i32_i64 =
      FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const},
                    FieldType{StorageType{VT_I64}, Mutability::Const}};

  // Width subtyping, with the same field type.
  EXPECT_TRUE(IsMatch(ctx, FTL_i32, FTL_i32_i64));
  EXPECT_FALSE(IsMatch(ctx, FTL_i32_i64, FTL_i32));

  PushFunctionType({}, {});      // type 0

  // Width subtyping and field covariance.
  EXPECT_TRUE(IsMatch(
      ctx,
      // (field funcref)
      FieldTypeList{FieldType{StorageType{VT_Funcref}, Mutability::Const}},
      // (field (ref 0) i32)
      FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Const},
                    FieldType{StorageType{VT_I32}, Mutability::Const}}));

  // Width subtyping, but non-const field inhibits covariance.
  EXPECT_FALSE(IsMatch(
      ctx,
      // (field funcref)
      FieldTypeList{FieldType{StorageType{VT_Funcref}, Mutability::Var}},
      // (field (ref 0) i32)
      FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Var},
                    FieldType{StorageType{VT_I32}, Mutability::Const}}));
}

TEST_F(ValidMatchTest, IsMatch_FunctionType) {
  std::vector<FunctionType> types{
      FunctionType{{}, {}},
      FunctionType{{VT_I32}, {VT_F32}},
      FunctionType{{VT_Ref0, VT_Ref0}, {VT_Ref0}},
  };
  PushFunctionType({}, {});  // 0
  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_StructType_Simple) {
  std::vector<StructType> types{
      StructType{
          FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Const}}},
      StructType{
          FieldTypeList{FieldType{StorageType{VT_I32}, Mutability::Var}}},
      StructType{
          FieldTypeList{FieldType{StorageType{VT_Ref0}, Mutability::Var}}},
      StructType{FieldTypeList{
          FieldType{StorageType{VT_I64}, Mutability::Const},
          FieldType{StorageType{PackedType::I8}, Mutability::Var}}},
  };
  PushFunctionType({}, {});  // 0
  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_StructType_Subtyping) {
  PushFunctionType({}, {});  // 0

  // Allow width subtyping (e.g. empty struct is supertype of all structs).
  EXPECT_TRUE(IsMatch(ctx, StructType{},
                      StructType{FieldTypeList{
                          FieldType{StorageType{VT_I32}, Mutability::Const}}}));

  // Allow depth subtyping, given a const field.
  EXPECT_TRUE(IsMatch(ctx,
                      StructType{FieldTypeList{FieldType{
                          StorageType{VT_Funcref}, Mutability::Const}}},
                      StructType{FieldTypeList{FieldType{StorageType{VT_Ref0},
                                                         Mutability::Const}}}));

  // Width and depth subtyping.
  EXPECT_TRUE(IsMatch(ctx,
                      StructType{FieldTypeList{FieldType{
                          StorageType{VT_Funcref}, Mutability::Const}}},
                      StructType{FieldTypeList{
                          FieldType{StorageType{VT_Ref0}, Mutability::Const},
                          FieldType{StorageType{VT_I32}, Mutability::Var}}}));
}

TEST_F(ValidMatchTest, IsMatch_ArrayType_Simple) {
  std::vector<ArrayType> types{
      ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}},
      ArrayType{FieldType{StorageType{VT_I32}, Mutability::Var}},
      ArrayType{FieldType{StorageType{VT_Ref0}, Mutability::Const}},
  };
  PushFunctionType({}, {});  // 0
  IsMatchDistinct(types);
}

TEST_F(ValidMatchTest, IsMatch_ArrayType_Subtyping) {
  PushFunctionType({}, {});  // 0

  // Allow depth subtyping, given a const field.
  EXPECT_TRUE(IsMatch(
      ctx, ArrayType{FieldType{StorageType{VT_Funcref}, Mutability::Const}},
      ArrayType{FieldType{StorageType{VT_Ref0}, Mutability::Const}}));
}

TEST_F(ValidMatchTest, IsMatch_DefinedType) {
  std::vector<DefinedType> types{
      DefinedType{FunctionType{}},
      DefinedType{StructType{}},
      DefinedType{ArrayType{FieldType{StorageType{VT_I32}, Mutability::Const}}},
  };
  IsMatchDistinct(types);
}

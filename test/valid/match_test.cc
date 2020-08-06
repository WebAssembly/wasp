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
#include "wasp/base/enumerate.h"
#include "wasp/base/format.h"
#include "wasp/binary/formatters.h"
#include "wasp/valid/context.h"

using namespace ::wasp;
using namespace ::wasp::valid;
using namespace ::wasp::valid::test;
using namespace ::wasp::binary::test;

enum Comparison {
  SAME,
  DIFF,
  SKIP,

  // Nice for tables below
  MTCH = SAME,
  ____ = DIFF,
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
          << format("i:{} j:{} should be {}", vi, vj, result);
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

TEST(ValidMatchTest, IsSame_HeapType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::HeapType> heap_types = {HT_Func, HT_Extern, HT_Exn, HT_0};
  IsSameTable(context, heap_types,
              {
                  /*
                  Func  Ext.  Exn.  0      */
                  SAME, ____, ____, ____,  // Func
                  ____, SAME, ____, ____,  // Extern
                  ____, ____, SAME, ____,  // Exn
                  ____, ____, ____, SAME,  // 0
              });
}

TEST(ValidMatchTest, IsSame_RefType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::RefType> ref_types = {
      RefType_Func, RefType_NullFunc, RefType_Extern, RefType_NullExtern,
      RefType_Exn,  RefType_NullExn,  RefType_0,      RefType_Null0,
  };
  IsSameTable(context, ref_types,
              {
                  /*
                        Null        Null        Null        Null
                  Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
                  SAME, ____, ____, ____, ____, ____, ____, ____,  // Func
                  ____, SAME, ____, ____, ____, ____, ____, ____,  // NullFunc
                  ____, ____, SAME, ____, ____, ____, ____, ____,  // Extern
                  ____, ____, ____, SAME, ____, ____, ____, ____,  // NullExtern
                  ____, ____, ____, ____, SAME, ____, ____, ____,  // Exn
                  ____, ____, ____, ____, ____, SAME, ____, ____,  // NullExn
                  ____, ____, ____, ____, ____, ____, SAME, ____,  // 0
                  ____, ____, ____, ____, ____, ____, ____, SAME,  // Null0
              });
}

TEST(ValidMatchTest, IsSame_ReferenceType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::ReferenceType> reference_types = {
      RT_Funcref,     RT_Externref, RT_Exnref,        RT_RefFunc,
      RT_RefNullFunc, RT_RefExtern, RT_RefNullExtern, RT_RefExn,
      RT_RefNullExn,  RT_Ref0,      RT_RefNull0,
  };
  IsSameTable(
      context,
      reference_types,
      {
          /*
          Func  Ext.  Exn         Null        Null        Null        Null
          Ref   Ref   Ref   Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // Funcref
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // Externref
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // Exnref
          ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____, ____,  // RefFunc
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // RefNullFunc
          ____, ____, ____, ____, ____, SAME, ____, ____, ____, ____, ____,  // RefExtern
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // RefNullExtern
          ____, ____, ____, ____, ____, ____, ____, SAME, ____, ____, ____,  // RefExn
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // RefNullExn
          ____, ____, ____, ____, ____, ____, ____, ____, ____, SAME, ____,  // Ref0
          ____, ____, ____, ____, ____, ____, ____, ____, ____, ____, SAME,  // RefNull0
      });
}

TEST(ValidMatchTest, IsSame_ValueType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::ValueType> value_num_types = {
      VT_I32, VT_I64, VT_F32, VT_F64, VT_V128,
  };
  std::vector<binary::ValueType> value_ref_types = {
      VT_Funcref,     VT_Externref, VT_Exnref,        VT_RefFunc,
      VT_RefNullFunc, VT_RefExtern, VT_RefNullExtern, VT_RefExn,
      VT_RefNullExn,  VT_Ref0,      VT_RefNull0,
  };

  IsSameTable(context, value_num_types,
              {
                  /*
                  i32   i64   f32   f64   v128 */
                  SAME, ____, ____, ____, ____,  // i32
                  ____, SAME, ____, ____, ____,  // i64
                  ____, ____, SAME, ____, ____,  // f32
                  ____, ____, ____, SAME, ____,  // f64
                  ____, ____, ____, ____, SAME,  // v128
              });

  IsSameTable(
      context,
      value_ref_types,
      {
          /*
          Func  Ext.  Exn         Null        Null        Null        Null
          Ref   Ref   Ref   Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // Funcref
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // Externref
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // Exnref
          ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____, ____,  // RefFunc
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // RefNullFunc
          ____, ____, ____, ____, ____, SAME, ____, ____, ____, ____, ____,  // RefExtern
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // RefNullExtern
          ____, ____, ____, ____, ____, ____, ____, SAME, ____, ____, ____,  // RefExn
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // RefNullExn
          ____, ____, ____, ____, ____, ____, ____, ____, ____, SAME, ____,  // Ref0
          ____, ____, ____, ____, ____, ____, ____, ____, ____, ____, SAME,  // RefNull0
      });

  // Numeric types are never the same as reference types.
  IsSameTable(
      context, value_num_types, value_ref_types,
      std::vector(value_num_types.size() * value_ref_types.size(), DIFF));

  IsSameTable(
      context, value_ref_types, value_num_types,
      std::vector(value_num_types.size() * value_ref_types.size(), DIFF));
}

TEST(ValidMatchTest, IsMatch_HeapType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::HeapType> heap_types = {HT_Func, HT_Extern, HT_Exn, HT_0};
  IsMatchTable(context, heap_types,
               {
                   /*
                   Func  Ext.  Exn.  0      */
                   SAME, ____, ____, ____,  // Func
                   ____, SAME, ____, ____,  // Extern
                   ____, ____, SAME, ____,  // Exn
                   MTCH, ____, ____, SAME,  // 0
               });
}

TEST(ValidMatchTest, IsMatch_RefType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::RefType> ref_types = {
      RefType_Func, RefType_NullFunc, RefType_Extern, RefType_NullExtern,
      RefType_Exn,  RefType_NullExn,  RefType_0,      RefType_Null0,
  };
  IsMatchTable(
      context, ref_types,
      {
          /*
                Null        Null        Null        Null
          Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
          SAME, MTCH, ____, ____, ____, ____, ____, ____,  // Func
          ____, SAME, ____, ____, ____, ____, ____, ____,  // NullFunc
          ____, ____, SAME, MTCH, ____, ____, ____, ____,  // Extern
          ____, ____, ____, SAME, ____, ____, ____, ____,  // NullExtern
          ____, ____, ____, ____, SAME, MTCH, ____, ____,  // Exn
          ____, ____, ____, ____, ____, SAME, ____, ____,  // NullExn
          MTCH, MTCH, ____, ____, ____, ____, SAME, MTCH,  // 0
          ____, MTCH, ____, ____, ____, ____, ____, SAME,  // Null0
      });
}

TEST(ValidMatchTest, Ismatch_ReferenceType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::ReferenceType> reference_types = {
      RT_Funcref,     RT_Externref, RT_Exnref,        RT_RefFunc,
      RT_RefNullFunc, RT_RefExtern, RT_RefNullExtern, RT_RefExn,
      RT_RefNullExn,  RT_Ref0,      RT_RefNull0,
  };
  IsMatchTable(
      context,
      reference_types,
      {
          /*
          Func  Ext.  Exn         Null        Null        Null        Null
          Ref   Ref   Ref   Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // Funcref
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // Externref
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // Exnref
          MTCH, ____, ____, SAME, MTCH, ____, ____, ____, ____, ____, ____,  // RefFunc
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // RefNullFunc
          ____, MTCH, ____, ____, ____, SAME, MTCH, ____, ____, ____, ____,  // RefExtern
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // RefNullExtern
          ____, ____, MTCH, ____, ____, ____, ____, SAME, MTCH, ____, ____,  // RefExn
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // RefNullExn
          MTCH, ____, ____, MTCH, MTCH, ____, ____, ____, ____, SAME, MTCH,  // Ref0
          MTCH, ____, ____, ____, MTCH, ____, ____, ____, ____, ____, SAME,  // RefNull0
      });
}

TEST(ValidMatchTest, IsMatch_ValueType_Simple) {
  TestErrors errors;
  Context context{errors};

  std::vector<binary::ValueType> value_num_types = {
      VT_I32, VT_I64, VT_F32, VT_F64, VT_V128,
  };
  std::vector<binary::ValueType> value_ref_types = {
      VT_Funcref,     VT_Externref, VT_Exnref,        VT_RefFunc,
      VT_RefNullFunc, VT_RefExtern, VT_RefNullExtern, VT_RefExn,
      VT_RefNullExn,  VT_Ref0,      VT_RefNull0,
  };

  IsMatchTable(context, value_num_types,
               {
                   /*
                   i32   i64   f32   f64   v128 */
                   SAME, ____, ____, ____, ____,  // i32
                   ____, SAME, ____, ____, ____,  // i64
                   ____, ____, SAME, ____, ____,  // f32
                   ____, ____, ____, SAME, ____,  // f64
                   ____, ____, ____, ____, SAME,  // v128
               });

  IsMatchTable(
      context,
      value_ref_types,
      {
          /*
          Func  Ext.  Exn         Null        Null        Null        Null
          Ref   Ref   Ref   Func  Func  Ext.  Ext.  Exn   Exn   0     0      */
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // Funcref
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // Externref
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // Exnref
          MTCH, ____, ____, SAME, MTCH, ____, ____, ____, ____, ____, ____,  // RefFunc
          SAME, ____, ____, ____, SAME, ____, ____, ____, ____, ____, ____,  // RefNullFunc
          ____, MTCH, ____, ____, ____, SAME, MTCH, ____, ____, ____, ____,  // RefExtern
          ____, SAME, ____, ____, ____, ____, SAME, ____, ____, ____, ____,  // RefNullExtern
          ____, ____, MTCH, ____, ____, ____, ____, SAME, MTCH, ____, ____,  // RefExn
          ____, ____, SAME, ____, ____, ____, ____, ____, SAME, ____, ____,  // RefNullExn
          MTCH, ____, ____, MTCH, MTCH, ____, ____, ____, ____, SAME, MTCH,  // Ref0
          MTCH, ____, ____, ____, MTCH, ____, ____, ____, ____, ____, SAME,  // RefNull0
      });

  // Numeric types never match reference types.
  IsMatchTable(
      context, value_num_types, value_ref_types,
      std::vector(value_num_types.size() * value_ref_types.size(), DIFF));

  IsMatchTable(
      context, value_ref_types, value_num_types,
      std::vector(value_num_types.size() * value_ref_types.size(), DIFF));
}

//
// Copyright 2018 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_DEFS_H_
#define WASP_BINARY_DEFS_H_

#define WASP_FOREACH_VALUE_TYPE(V) \
  V(0x7f, I32, "i32")              \
  V(0x7e, I64, "i64")              \
  V(0x7d, F32, "f32")              \
  V(0x7c, F64, "f64")

#define WASP_FOREACH_BLOCK_TYPE(V) \
  WASP_FOREACH_VALUE_TYPE(V)       \
  V(0x40, Void, "")

#define WASP_FOREACH_ELEMENT_TYPE(V) \
  V(0x70, Funcref, "funcref")

#define WASP_FOREACH_EXTERNAL_KIND(V) \
  V(0, Function, "func")              \
  V(1, Table, "table")                \
  V(2, Memory, "mem")                 \
  V(3, Global, "global")

#define WASP_FOREACH_MUTABILITY(V) \
  V(0, Const, "const")             \
  V(1, Var, "var")

#define WASP_FOREACH_SECTION(V) \
  V(0, Custom, "custom")        \
  V(1, Type, "type")            \
  V(2, Import, "import")        \
  V(3, Function, "function")    \
  V(4, Table, "table")          \
  V(5, Memory, "memory")        \
  V(6, Global, "global")        \
  V(7, Export, "export")        \
  V(8, Start, "start")          \
  V(9, Element, "element")      \
  V(10, Code, "code")           \
  V(11, Data, "data")

#define WASP_FOREACH_OPCODE(V)                            \
  V(0x00, 0x00, Unreachable, "unreachable")               \
  V(0x00, 0x01, Nop, "nop")                               \
  V(0x00, 0x02, Block, "block")                           \
  V(0x00, 0x03, Loop, "loop")                             \
  V(0x00, 0x04, If, "if")                                 \
  V(0x00, 0x05, Else, "else")                             \
  V(0x00, 0x0b, End, "end")                               \
  V(0x00, 0x0c, Br, "br")                                 \
  V(0x00, 0x0d, BrIf, "br_if")                            \
  V(0x00, 0x0e, BrTable, "br_table")                      \
  V(0x00, 0x0f, Return, "return")                         \
  V(0x00, 0x10, Call, "call")                             \
  V(0x00, 0x11, CallIndirect, "call_indirect")            \
  V(0x00, 0x1a, Drop, "drop")                             \
  V(0x00, 0x1b, Select, "select")                         \
  V(0x00, 0x20, GetLocal, "get_local")                    \
  V(0x00, 0x21, SetLocal, "set_local")                    \
  V(0x00, 0x22, TeeLocal, "tee_local")                    \
  V(0x00, 0x23, GetGlobal, "get_global")                  \
  V(0x00, 0x24, SetGlobal, "set_global")                  \
  V(0x00, 0x28, I32Load, "i32.load")                      \
  V(0x00, 0x29, I64Load, "i64.load")                      \
  V(0x00, 0x2a, F32Load, "f32.load")                      \
  V(0x00, 0x2b, F64Load, "f64.load")                      \
  V(0x00, 0x2c, I32Load8S, "i32.load8_s")                 \
  V(0x00, 0x2d, I32Load8U, "i32.load8_u")                 \
  V(0x00, 0x2e, I32Load16S, "i32.load16_s")               \
  V(0x00, 0x2f, I32Load16U, "i32.load16_u")               \
  V(0x00, 0x30, I64Load8S, "i64.load8_s")                 \
  V(0x00, 0x31, I64Load8U, "i64.load8_u")                 \
  V(0x00, 0x32, I64Load16S, "i64.load16_s")               \
  V(0x00, 0x33, I64Load16U, "i64.load16_u")               \
  V(0x00, 0x34, I64Load32S, "i64.load32_s")               \
  V(0x00, 0x35, I64Load32U, "i64.load32_u")               \
  V(0x00, 0x36, I32Store, "i32.store")                    \
  V(0x00, 0x37, I64Store, "i64.store")                    \
  V(0x00, 0x38, F32Store, "f32.store")                    \
  V(0x00, 0x39, F64Store, "f64.store")                    \
  V(0x00, 0x3a, I32Store8, "i32.store8")                  \
  V(0x00, 0x3b, I32Store16, "i32.store16")                \
  V(0x00, 0x3c, I64Store8, "i64.store8")                  \
  V(0x00, 0x3d, I64Store16, "i64.store16")                \
  V(0x00, 0x3e, I64Store32, "i64.store32")                \
  V(0x00, 0x3f, MemorySize, "memory.size")                \
  V(0x00, 0x40, MemoryGrow, "memory.grow")                \
  V(0x00, 0x41, I32Const, "i32.const")                    \
  V(0x00, 0x42, I64Const, "i64.const")                    \
  V(0x00, 0x43, F32Const, "f32.const")                    \
  V(0x00, 0x44, F64Const, "f64.const")                    \
  V(0x00, 0x45, I32Eqz, "i32.eqz")                        \
  V(0x00, 0x46, I32Eq, "i32.eq")                          \
  V(0x00, 0x47, I32Ne, "i32.ne")                          \
  V(0x00, 0x48, I32LtS, "i32.lt_s")                       \
  V(0x00, 0x49, I32LtU, "i32.lt_u")                       \
  V(0x00, 0x4a, I32GtS, "i32.gt_s")                       \
  V(0x00, 0x4b, I32GtU, "i32.gt_u")                       \
  V(0x00, 0x4c, I32LeS, "i32.le_s")                       \
  V(0x00, 0x4d, I32LeU, "i32.le_u")                       \
  V(0x00, 0x4e, I32GeS, "i32.ge_s")                       \
  V(0x00, 0x4f, I32GeU, "i32.ge_u")                       \
  V(0x00, 0x50, I64Eqz, "i64.eqz")                        \
  V(0x00, 0x51, I64Eq, "i64.eq")                          \
  V(0x00, 0x52, I64Ne, "i64.ne")                          \
  V(0x00, 0x53, I64LtS, "i64.lt_s")                       \
  V(0x00, 0x54, I64LtU, "i64.lt_u")                       \
  V(0x00, 0x55, I64GtS, "i64.gt_s")                       \
  V(0x00, 0x56, I64GtU, "i64.gt_u")                       \
  V(0x00, 0x57, I64LeS, "i64.le_s")                       \
  V(0x00, 0x58, I64LeU, "i64.le_u")                       \
  V(0x00, 0x59, I64GeS, "i64.ge_s")                       \
  V(0x00, 0x5a, I64GeU, "i64.ge_u")                       \
  V(0x00, 0x5b, F32Eq, "f32.eq")                          \
  V(0x00, 0x5c, F32Ne, "f32.ne")                          \
  V(0x00, 0x5d, F32Lt, "f32.lt")                          \
  V(0x00, 0x5e, F32Gt, "f32.gt")                          \
  V(0x00, 0x5f, F32Le, "f32.le")                          \
  V(0x00, 0x60, F32Ge, "f32.ge")                          \
  V(0x00, 0x61, F64Eq, "f64.eq")                          \
  V(0x00, 0x62, F64Ne, "f64.ne")                          \
  V(0x00, 0x63, F64Lt, "f64.lt")                          \
  V(0x00, 0x64, F64Gt, "f64.gt")                          \
  V(0x00, 0x65, F64Le, "f64.le")                          \
  V(0x00, 0x66, F64Ge, "f64.ge")                          \
  V(0x00, 0x67, I32Clz, "i32.clz")                        \
  V(0x00, 0x68, I32Ctz, "i32.ctz")                        \
  V(0x00, 0x69, I32Popcnt, "i32.popcnt")                  \
  V(0x00, 0x6a, I32Add, "i32.add")                        \
  V(0x00, 0x6b, I32Sub, "i32.sub")                        \
  V(0x00, 0x6c, I32Mul, "i32.mul")                        \
  V(0x00, 0x6d, I32DivS, "i32.div_s")                     \
  V(0x00, 0x6e, I32DivU, "i32.div_u")                     \
  V(0x00, 0x6f, I32RemS, "i32.rem_s")                     \
  V(0x00, 0x70, I32RemU, "i32.rem_u")                     \
  V(0x00, 0x71, I32And, "i32.and")                        \
  V(0x00, 0x72, I32Or, "i32.or")                          \
  V(0x00, 0x73, I32Xor, "i32.xor")                        \
  V(0x00, 0x74, I32Shl, "i32.shl")                        \
  V(0x00, 0x75, I32ShrS, "i32.shr_s")                     \
  V(0x00, 0x76, I32ShrU, "i32.shr_u")                     \
  V(0x00, 0x77, I32Rotl, "i32.rotl")                      \
  V(0x00, 0x78, I32Rotr, "i32.rotr")                      \
  V(0x00, 0x79, I64Clz, "i64.clz")                        \
  V(0x00, 0x7a, I64Ctz, "i64.ctz")                        \
  V(0x00, 0x7b, I64Popcnt, "i64.popcnt")                  \
  V(0x00, 0x7c, I64Add, "i64.add")                        \
  V(0x00, 0x7d, I64Sub, "i64.sub")                        \
  V(0x00, 0x7e, I64Mul, "i64.mul")                        \
  V(0x00, 0x7f, I64DivS, "i64.div_s")                     \
  V(0x00, 0x80, I64DivU, "i64.div_u")                     \
  V(0x00, 0x81, I64RemS, "i64.rem_s")                     \
  V(0x00, 0x82, I64RemU, "i64.rem_u")                     \
  V(0x00, 0x83, I64And, "i64.and")                        \
  V(0x00, 0x84, I64Or, "i64.or")                          \
  V(0x00, 0x85, I64Xor, "i64.xor")                        \
  V(0x00, 0x86, I64Shl, "i64.shl")                        \
  V(0x00, 0x87, I64ShrS, "i64.shr_s")                     \
  V(0x00, 0x88, I64ShrU, "i64.shr_u")                     \
  V(0x00, 0x89, I64Rotl, "i64.rotl")                      \
  V(0x00, 0x8a, I64Rotr, "i64.rotr")                      \
  V(0x00, 0x8b, F32Abs, "f32.abs")                        \
  V(0x00, 0x8c, F32Neg, "f32.neg")                        \
  V(0x00, 0x8d, F32Ceil, "f32.ceil")                      \
  V(0x00, 0x8e, F32Floor, "f32.floor")                    \
  V(0x00, 0x8f, F32Trunc, "f32.trunc")                    \
  V(0x00, 0x90, F32Nearest, "f32.nearest")                \
  V(0x00, 0x91, F32Sqrt, "f32.sqrt")                      \
  V(0x00, 0x92, F32Add, "f32.add")                        \
  V(0x00, 0x93, F32Sub, "f32.sub")                        \
  V(0x00, 0x94, F32Mul, "f32.mul")                        \
  V(0x00, 0x95, F32Div, "f32.div")                        \
  V(0x00, 0x96, F32Min, "f32.min")                        \
  V(0x00, 0x97, F32Max, "f32.max")                        \
  V(0x00, 0x98, F32Copysign, "f32.copysign")              \
  V(0x00, 0x99, F64Abs, "f64.abs")                        \
  V(0x00, 0x9a, F64Neg, "f64.neg")                        \
  V(0x00, 0x9b, F64Ceil, "f64.ceil")                      \
  V(0x00, 0x9c, F64Floor, "f64.floor")                    \
  V(0x00, 0x9d, F64Trunc, "f64.trunc")                    \
  V(0x00, 0x9e, F64Nearest, "f64.nearest")                \
  V(0x00, 0x9f, F64Sqrt, "f64.sqrt")                      \
  V(0x00, 0xa0, F64Add, "f64.add")                        \
  V(0x00, 0xa1, F64Sub, "f64.sub")                        \
  V(0x00, 0xa2, F64Mul, "f64.mul")                        \
  V(0x00, 0xa3, F64Div, "f64.div")                        \
  V(0x00, 0xa4, F64Min, "f64.min")                        \
  V(0x00, 0xa5, F64Max, "f64.max")                        \
  V(0x00, 0xa6, F64Copysign, "f64.copysign")              \
  V(0x00, 0xa7, I32WrapI64, "i32.wrap_i64")               \
  V(0x00, 0xa8, I32TruncSF32, "i32.trunc_s/f32")          \
  V(0x00, 0xa9, I32TruncUF32, "i32.trunc_u/f32")          \
  V(0x00, 0xaa, I32TruncSF64, "i32.trunc_s/f64")          \
  V(0x00, 0xab, I32TruncUF64, "i32.trunc_u/f64")          \
  V(0x00, 0xac, I64ExtendSI32, "i64.extend_s/i32")        \
  V(0x00, 0xad, I64ExtendUI32, "i64.extend_u/i32")        \
  V(0x00, 0xae, I64TruncSF32, "i64.trunc_s/f32")          \
  V(0x00, 0xaf, I64TruncUF32, "i64.trunc_u/f32")          \
  V(0x00, 0xb0, I64TruncSF64, "i64.trunc_s/f64")          \
  V(0x00, 0xb1, I64TruncUF64, "i64.trunc_u/f64")          \
  V(0x00, 0xb2, F32ConvertSI32, "f32.convert_s/i32")      \
  V(0x00, 0xb3, F32ConvertUI32, "f32.convert_u/i32")      \
  V(0x00, 0xb4, F32ConvertSI64, "f32.convert_s/i64")      \
  V(0x00, 0xb5, F32ConvertUI64, "f32.convert_u/i64")      \
  V(0x00, 0xb6, F32DemoteF64, "f32.demote/f64")           \
  V(0x00, 0xb7, F64ConvertSI32, "f64.convert_s/i32")      \
  V(0x00, 0xb8, F64ConvertUI32, "f64.convert_u/i32")      \
  V(0x00, 0xb9, F64ConvertSI64, "f64.convert_s/i64")      \
  V(0x00, 0xba, F64ConvertUI64, "f64.convert_u/i64")      \
  V(0x00, 0xbb, F64PromoteF32, "f64.promote/f32")         \
  V(0x00, 0xbc, I32ReinterpretF32, "i32.reinterpret/f32") \
  V(0x00, 0xbd, I64ReinterpretF64, "i64.reinterpret/f64") \
  V(0x00, 0xbe, F32ReinterpretI32, "f32.reinterpret/i32") \
  V(0x00, 0xbf, F64ReinterpretI64, "f64.reinterpret/i64")

#endif  // WASP_BINARY_DEFS_H_

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

// Invalid or EOF
WASP_V(Eof)
WASP_V(InvalidBlockComment)
WASP_V(InvalidChar)
WASP_V(InvalidLineComment)
WASP_V(InvalidText)
WASP_V(Reserved)

// Whitespace or comments
WASP_V(BlockComment)
WASP_V(LineComment)
WASP_V(Whitespace)

// .wat symbols
WASP_V(AlignEqNat)         // "align=8"
WASP_V(Array)              // "array"
WASP_V(Binary)             // "binary"
WASP_V(Catch)              // "catch"
WASP_V(Data)               // "data"
WASP_V(Declare)            // "declare"
WASP_V(Elem)               // "elem"
WASP_V(Else)               // "else"
WASP_V(End)                // "end"
WASP_V(Event)              // "event"
WASP_V(Export)             // "export"
WASP_V(Field)              // "field"
WASP_V(Float)              // "3.14159"
WASP_V(Func)               // "func"
WASP_V(Global)             // "global"
WASP_V(HeapKind)           // "any", "extern", "exn", "i31", etc.
WASP_V(Id)                 // "$hello"
WASP_V(I31)                // "i31"
WASP_V(Import)             // "import"
WASP_V(Int)                // "+1234"
WASP_V(Item)               // "item"
WASP_V(Local)              // "local"
WASP_V(Lpar)               // "("
WASP_V(LparAnn)            // "(@keyword"
WASP_V(Memory)             // "memory"
WASP_V(Module)             // "module"
WASP_V(Mut)                // "mut"
WASP_V(Nat)                // "1234"
WASP_V(Null)               // "null"
WASP_V(NumericType)        // "i32", "f32", etc.
WASP_V(Offset)             // "offset"
WASP_V(OffsetEqNat)        // "offset=1234"
WASP_V(PackedType)         // "i8", "i16"
WASP_V(Param)              // "param"
WASP_V(Quote)              // "quote"
WASP_V(Ref)                // "ref"
WASP_V(ReferenceKind)      // "funcref", "externref", etc.
WASP_V(Result)             // "result"
WASP_V(Rpar)               // ")"
WASP_V(Rtt)                // "rtt"
WASP_V(Shared)             // "shared"
WASP_V(SimdShape)          // "i8x16", "i16x8", "f32x4", etc.
WASP_V(Start)              // "start"
WASP_V(Struct)             // "struct"
WASP_V(Table)              // "table"
WASP_V(Text)               // "\"hello world\""
WASP_V(Then)               // "then"
WASP_V(Type)               // "type"

// Plain Instructions.
WASP_V(BareInstr)            // "i32.add", etc.
WASP_V(BrOnCastInstr)        // "br_on_cast"
WASP_V(BrOnExnInstr)         // "br_on_exn"
WASP_V(BrTableInstr)         // "br_table"
WASP_V(CallIndirectInstr)    // "call_indirect", "return_call_indirect"
WASP_V(F32ConstInstr)        // "f32.const"
WASP_V(F64ConstInstr)        // "f64.const"
WASP_V(FuncBindInstr)        // "func.bind"
WASP_V(HeapTypeInstr)        // "rtt.canon", etc.
WASP_V(HeapType2Instr)       // "rtt.test", "rtt.cast"
WASP_V(I32ConstInstr)        // "i32.const"
WASP_V(I64ConstInstr)        // "i64.const"
WASP_V(LetInstr)             // "let"
WASP_V(MemoryInstr)          // "i32.load", "i64.atomic.rmw8.add_u", etc.
WASP_V(MemoryCopyInstr)      // "memory.copy"
WASP_V(MemoryInitInstr)      // "memory.init"
WASP_V(RefFuncInstr)         // "ref.func"
WASP_V(RefNullInstr)         // "rtt.canon"
WASP_V(RttSubInstr)          // "rtt.sub"
WASP_V(SelectInstr)          // "select"
WASP_V(SimdConstInstr)       // "v128.const"
WASP_V(SimdLaneInstr)        // "i8x16.extract_lane_s", "f32.replace_lane", etc.
WASP_V(SimdMemoryLaneInstr)  // "v128.load8_lane"
WASP_V(SimdShuffleInstr)     // "v128.shuffle"
WASP_V(StructFieldInstr)     // "struct.get", "struct.set", etc.
WASP_V(TableCopyInstr)       // "table.copy"
WASP_V(TableInitInstr)       // "table.init"
WASP_V(VarInstr)             // "br", "table.get", "local.set", etc.

WASP_V(BlockInstr)         // "block", "loop", etc.

// .wast symbols
WASP_V(AssertExhaustion)  // "assert_exhaustion"
WASP_V(AssertInvalid)     // "assert_invalid"
WASP_V(AssertMalformed)   // "assert_malformed"
WASP_V(AssertReturn)      // "assert_return"
WASP_V(AssertTrap)        // "assert_trap"
WASP_V(AssertUnlinkable)  // "assert_unlinkable"
WASP_V(Get)               // "get"
WASP_V(Invoke)            // "invoke"
WASP_V(NanArithmetic)     // "nan:arithmetic"
WASP_V(NanCanonical)      // "nan:canonical"
WASP_V(RefExtern)         // "ref.extern"
WASP_V(Register)          // "register"

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

#ifndef WASP_BINARY_ENCODING_H
#define WASP_BINARY_ENCODING_H

#include "src/base/types.h"

namespace wasp {
namespace binary {
namespace encoding {

constexpr u8 Magic[] = {0, 'a', 's', 'm'};
constexpr u8 Version[] = {1, 0, 0, 0};

struct ValType {
  static constexpr s32 I32 = -0x01;
  static constexpr s32 I64 = -0x02;
  static constexpr s32 F32 = -0x03;
  static constexpr s32 F64 = -0x04;
  static constexpr s32 Anyfunc = -0x10;
  static constexpr s32 Func = -0x20;
  static constexpr s32 Void = -0x40;
};

struct ExternalKind {
  static constexpr u8 Func = 0;
  static constexpr u8 Table = 1;
  static constexpr u8 Memory = 2;
  static constexpr u8 Global = 3;
};

struct Section {
  static constexpr u32 Custom = 0;
  static constexpr u32 Type = 1;
  static constexpr u32 Import = 2;
  static constexpr u32 Function = 3;
  static constexpr u32 Table = 4;
  static constexpr u32 Memory = 5;
  static constexpr u32 Global = 6;
  static constexpr u32 Export = 7;
  static constexpr u32 Start = 8;
  static constexpr u32 Element = 9;
  static constexpr u32 Code = 10;
  static constexpr u32 Data = 11;
};

struct Opcode {
  static constexpr u8 Unreachable = 0x00;
  static constexpr u8 Nop = 0x01;
  static constexpr u8 Block = 0x02;
  static constexpr u8 Loop = 0x03;
  static constexpr u8 If = 0x04;
  static constexpr u8 Else = 0x05;
  static constexpr u8 Try = 0x06;
  static constexpr u8 Catch = 0x07;
  static constexpr u8 Throw = 0x08;
  static constexpr u8 Rethrow = 0x09;
  static constexpr u8 IfExcept = 0x0a;
  static constexpr u8 End = 0x0b;
  static constexpr u8 Br = 0x0c;
  static constexpr u8 BrIf = 0x0d;
  static constexpr u8 BrTable = 0x0e;
  static constexpr u8 Return = 0x0f;
  static constexpr u8 Call = 0x10;
  static constexpr u8 CallIndirect = 0x11;
  static constexpr u8 ReturnCall = 0x12;
  static constexpr u8 ReturnCallIndirect = 0x13;
  static constexpr u8 Drop = 0x1a;
  static constexpr u8 Select = 0x1b;
  static constexpr u8 GetLocal = 0x20;
  static constexpr u8 SetLocal = 0x21;
  static constexpr u8 TeeLocal = 0x22;
  static constexpr u8 GetGlobal = 0x23;
  static constexpr u8 SetGlobal = 0x24;
  static constexpr u8 I32Load = 0x28;
  static constexpr u8 I64Load = 0x29;
  static constexpr u8 F32Load = 0x2a;
  static constexpr u8 F64Load = 0x2b;
  static constexpr u8 I32Load8S = 0x2c;
  static constexpr u8 I32Load8U = 0x2d;
  static constexpr u8 I32Load16S = 0x2e;
  static constexpr u8 I32Load16U = 0x2f;
  static constexpr u8 I64Load8S = 0x30;
  static constexpr u8 I64Load8U = 0x31;
  static constexpr u8 I64Load16S = 0x32;
  static constexpr u8 I64Load16U = 0x33;
  static constexpr u8 I64Load32S = 0x34;
  static constexpr u8 I64Load32U = 0x35;
  static constexpr u8 I32Store = 0x36;
  static constexpr u8 I64Store = 0x37;
  static constexpr u8 F32Store = 0x38;
  static constexpr u8 F64Store = 0x39;
  static constexpr u8 I32Store8 = 0x3a;
  static constexpr u8 I32Store16 = 0x3b;
  static constexpr u8 I64Store8 = 0x3c;
  static constexpr u8 I64Store16 = 0x3d;
  static constexpr u8 I64Store32 = 0x3e;
  static constexpr u8 MemorySize = 0x3f;
  static constexpr u8 MemoryGrow = 0x40;
  static constexpr u8 I32Const = 0x41;
  static constexpr u8 I64Const = 0x42;
  static constexpr u8 F32Const = 0x43;
  static constexpr u8 F64Const = 0x44;
  static constexpr u8 I32Eqz = 0x45;
  static constexpr u8 I32Eq = 0x46;
  static constexpr u8 I32Ne = 0x47;
  static constexpr u8 I32LtS = 0x48;
  static constexpr u8 I32LtU = 0x49;
  static constexpr u8 I32GtS = 0x4a;
  static constexpr u8 I32GtU = 0x4b;
  static constexpr u8 I32LeS = 0x4c;
  static constexpr u8 I32LeU = 0x4d;
  static constexpr u8 I32GeS = 0x4e;
  static constexpr u8 I32GeU = 0x4f;
  static constexpr u8 I64Eqz = 0x50;
  static constexpr u8 I64Eq = 0x51;
  static constexpr u8 I64Ne = 0x52;
  static constexpr u8 I64LtS = 0x53;
  static constexpr u8 I64LtU = 0x54;
  static constexpr u8 I64GtS = 0x55;
  static constexpr u8 I64GtU = 0x56;
  static constexpr u8 I64LeS = 0x57;
  static constexpr u8 I64LeU = 0x58;
  static constexpr u8 I64GeS = 0x59;
  static constexpr u8 I64GeU = 0x5a;
  static constexpr u8 F32Eq = 0x5b;
  static constexpr u8 F32Ne = 0x5c;
  static constexpr u8 F32Lt = 0x5d;
  static constexpr u8 F32Gt = 0x5e;
  static constexpr u8 F32Le = 0x5f;
  static constexpr u8 F32Ge = 0x60;
  static constexpr u8 F64Eq = 0x61;
  static constexpr u8 F64Ne = 0x62;
  static constexpr u8 F64Lt = 0x63;
  static constexpr u8 F64Gt = 0x64;
  static constexpr u8 F64Le = 0x65;
  static constexpr u8 F64Ge = 0x66;
  static constexpr u8 I32Clz = 0x67;
  static constexpr u8 I32Ctz = 0x68;
  static constexpr u8 I32Popcnt = 0x69;
  static constexpr u8 I32Add = 0x6a;
  static constexpr u8 I32Sub = 0x6b;
  static constexpr u8 I32Mul = 0x6c;
  static constexpr u8 I32DivS = 0x6d;
  static constexpr u8 I32DivU = 0x6e;
  static constexpr u8 I32RemS = 0x6f;
  static constexpr u8 I32RemU = 0x70;
  static constexpr u8 I32And = 0x71;
  static constexpr u8 I32Or = 0x72;
  static constexpr u8 I32Xor = 0x73;
  static constexpr u8 I32Shl = 0x74;
  static constexpr u8 I32ShrS = 0x75;
  static constexpr u8 I32ShrU = 0x76;
  static constexpr u8 I32Rotl = 0x77;
  static constexpr u8 I32Rotr = 0x78;
  static constexpr u8 I64Clz = 0x79;
  static constexpr u8 I64Ctz = 0x7a;
  static constexpr u8 I64Popcnt = 0x7b;
  static constexpr u8 I64Add = 0x7c;
  static constexpr u8 I64Sub = 0x7d;
  static constexpr u8 I64Mul = 0x7e;
  static constexpr u8 I64DivS = 0x7f;
  static constexpr u8 I64DivU = 0x80;
  static constexpr u8 I64RemS = 0x81;
  static constexpr u8 I64RemU = 0x82;
  static constexpr u8 I64And = 0x83;
  static constexpr u8 I64Or = 0x84;
  static constexpr u8 I64Xor = 0x85;
  static constexpr u8 I64Shl = 0x86;
  static constexpr u8 I64ShrS = 0x87;
  static constexpr u8 I64ShrU = 0x88;
  static constexpr u8 I64Rotl = 0x89;
  static constexpr u8 I64Rotr = 0x8a;
  static constexpr u8 F32Abs = 0x8b;
  static constexpr u8 F32Neg = 0x8c;
  static constexpr u8 F32Ceil = 0x8d;
  static constexpr u8 F32Floor = 0x8e;
  static constexpr u8 F32Trunc = 0x8f;
  static constexpr u8 F32Nearest = 0x90;
  static constexpr u8 F32Sqrt = 0x91;
  static constexpr u8 F32Add = 0x92;
  static constexpr u8 F32Sub = 0x93;
  static constexpr u8 F32Mul = 0x94;
  static constexpr u8 F32Div = 0x95;
  static constexpr u8 F32Min = 0x96;
  static constexpr u8 F32Max = 0x97;
  static constexpr u8 F32Copysign = 0x98;
  static constexpr u8 F64Abs = 0x99;
  static constexpr u8 F64Neg = 0x9a;
  static constexpr u8 F64Ceil = 0x9b;
  static constexpr u8 F64Floor = 0x9c;
  static constexpr u8 F64Trunc = 0x9d;
  static constexpr u8 F64Nearest = 0x9e;
  static constexpr u8 F64Sqrt = 0x9f;
  static constexpr u8 F64Add = 0xa0;
  static constexpr u8 F64Sub = 0xa1;
  static constexpr u8 F64Mul = 0xa2;
  static constexpr u8 F64Div = 0xa3;
  static constexpr u8 F64Min = 0xa4;
  static constexpr u8 F64Max = 0xa5;
  static constexpr u8 F64Copysign = 0xa6;
  static constexpr u8 I32WrapI64 = 0xa7;
  static constexpr u8 I32TruncSF32 = 0xa8;
  static constexpr u8 I32TruncUF32 = 0xa9;
  static constexpr u8 I32TruncSF64 = 0xaa;
  static constexpr u8 I32TruncUF64 = 0xab;
  static constexpr u8 I64ExtendSI32 = 0xac;
  static constexpr u8 I64ExtendUI32 = 0xad;
  static constexpr u8 I64TruncSF32 = 0xae;
  static constexpr u8 I64TruncUF32 = 0xaf;
  static constexpr u8 I64TruncSF64 = 0xb0;
  static constexpr u8 I64TruncUF64 = 0xb1;
  static constexpr u8 F32ConvertSI32 = 0xb2;
  static constexpr u8 F32ConvertUI32 = 0xb3;
  static constexpr u8 F32ConvertSI64 = 0xb4;
  static constexpr u8 F32ConvertUI64 = 0xb5;
  static constexpr u8 F32DemoteF64 = 0xb6;
  static constexpr u8 F64ConvertSI32 = 0xb7;
  static constexpr u8 F64ConvertUI32 = 0xb8;
  static constexpr u8 F64ConvertSI64 = 0xb9;
  static constexpr u8 F64ConvertUI64 = 0xba;
  static constexpr u8 F64PromoteF32 = 0xbb;
  static constexpr u8 I32ReinterpretF32 = 0xbc;
  static constexpr u8 I64ReinterpretF64 = 0xbd;
  static constexpr u8 F32ReinterpretI32 = 0xbe;
  static constexpr u8 F64ReinterpretI64 = 0xbf;
};

}  // namespace encoding
}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_ENCODING_H

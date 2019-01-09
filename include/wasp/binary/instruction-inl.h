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

namespace wasp {
namespace binary {

inline bool Instruction::has_empty_immediate() const {
  return holds_alternative<EmptyImmediate>(immediate);
}

inline bool Instruction::has_block_type_immediate() const {
  return holds_alternative<BlockType>(immediate);
}

inline bool Instruction::has_index_immediate() const {
  return holds_alternative<Index>(immediate);
}

inline bool Instruction::has_call_indirect_immediate() const {
  return holds_alternative<CallIndirectImmediate>(immediate);
}

inline bool Instruction::has_br_table_immediate() const {
  return holds_alternative<BrTableImmediate>(immediate);
}

inline bool Instruction::has_u8_immediate() const {
  return holds_alternative<u8>(immediate);
}

inline bool Instruction::has_mem_arg_immediate() const {
  return holds_alternative<MemArgImmediate>(immediate);
}

inline bool Instruction::has_s32_immediate() const {
  return holds_alternative<s32>(immediate);
}

inline bool Instruction::has_s64_immediate() const {
  return holds_alternative<s64>(immediate);
}

inline bool Instruction::has_f32_immediate() const {
  return holds_alternative<f32>(immediate);
}

inline bool Instruction::has_f64_immediate() const {
  return holds_alternative<f64>(immediate);
}

inline bool Instruction::has_init_immediate() const {
  return holds_alternative<InitImmediate>(immediate);
}

inline bool Instruction::has_copy_immediate() const {
  return holds_alternative<CopyImmediate>(immediate);
}

inline EmptyImmediate& Instruction::empty_immediate() {
  return get<EmptyImmediate>(immediate);
}

inline const EmptyImmediate& Instruction::empty_immediate() const {
  return get<EmptyImmediate>(immediate);
}

inline BlockType& Instruction::block_type_immediate() {
  return get<BlockType>(immediate);
}

inline const BlockType& Instruction::block_type_immediate() const {
  return get<BlockType>(immediate);
}

inline Index& Instruction::index_immediate() {
  return get<Index>(immediate);
}

inline const Index& Instruction::index_immediate() const {
  return get<Index>(immediate);
}

inline CallIndirectImmediate& Instruction::call_indirect_immediate() {
  return get<CallIndirectImmediate>(immediate);
}

inline const CallIndirectImmediate& Instruction::call_indirect_immediate()
    const {
  return get<CallIndirectImmediate>(immediate);
}

inline BrTableImmediate& Instruction::br_table_immediate() {
  return get<BrTableImmediate>(immediate);
}

inline const BrTableImmediate& Instruction::br_table_immediate() const {
  return get<BrTableImmediate>(immediate);
}

inline u8& Instruction::u8_immediate() {
  return get<u8>(immediate);
}

inline const u8& Instruction::u8_immediate() const {
  return get<u8>(immediate);
}

inline MemArgImmediate& Instruction::mem_arg_immediate() {
  return get<MemArgImmediate>(immediate);
}

inline const MemArgImmediate& Instruction::mem_arg_immediate() const {
  return get<MemArgImmediate>(immediate);
}

inline s32& Instruction::s32_immediate() {
  return get<s32>(immediate);
}

inline const s32& Instruction::s32_immediate() const {
  return get<s32>(immediate);
}

inline s64& Instruction::s64_immediate() {
  return get<s64>(immediate);
}

inline const s64& Instruction::s64_immediate() const {
  return get<s64>(immediate);
}

inline f32& Instruction::f32_immediate() {
  return get<f32>(immediate);
}

inline const f32& Instruction::f32_immediate() const {
  return get<f32>(immediate);
}

inline f64& Instruction::f64_immediate() {
  return get<f64>(immediate);
}

inline const f64& Instruction::f64_immediate() const {
  return get<f64>(immediate);
}

inline InitImmediate& Instruction::init_immediate() {
  return get<InitImmediate>(immediate);
}

inline const InitImmediate& Instruction::init_immediate() const {
  return get<InitImmediate>(immediate);
}

inline CopyImmediate& Instruction::copy_immediate() {
  return get<CopyImmediate>(immediate);
}

inline const CopyImmediate& Instruction::copy_immediate() const {
  return get<CopyImmediate>(immediate);
}

}  // namespace binary
}  // namespace wasp

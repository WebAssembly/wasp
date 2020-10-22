switch (PeekChar(data, 2)) {
  default:
    switch (PeekChar(data, 1)) {
      case '8': return LexKeyword(data, "i8", PackedType::I8);
      case 'f': return LexKeyword(data, "if", TokenType::BlockInstr, Opcode::If);
      case 'q': return LexKeyword(data, "eq", TokenType::HeapKind, HeapKind::Eq);
      case 'r': return LexKeyword(data, "br", TokenType::VarInstr, Opcode::Br);
      default: break;
    }
    break;
  case '1':
    switch (PeekChar(data, 3)) {
      default: return LexKeyword(data, "i31", TokenType::HeapKind, HeapKind::I31);
      case '.':
        switch (PeekChar(data, 7)) {
          default: return LexKeyword(data, "i31.new", Opcode::I31New, Features::GC);
          case '_':
            switch (PeekChar(data, 8)) {
              case 's': return LexKeyword(data, "i31.get_s", Opcode::I31GetS, Features::GC);
              case 'u': return LexKeyword(data, "i31.get_u", Opcode::I31GetU, Features::GC);
              default: break;
            }
            break;
        }
        break;
      case 'r': return LexKeyword(data, "i31ref", ReferenceKind::I31ref);
    }
    break;
  case '2':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 0)) {
          case 'f': return LexKeyword(data, "f32", NumericType::F32);
          case 'i': return LexKeyword(data, "i32", NumericType::I32);
          default: break;
        }
        break;
      case '.':
        switch (PeekChar(data, 6)) {
          default:
            switch (PeekChar(data, 4)) {
              case 'e':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.eq", Opcode::F32Eq);
                  case 'i': return LexKeyword(data, "i32.eq", Opcode::I32Eq);
                  default: break;
                }
                break;
              case 'g':
                switch (PeekChar(data, 5)) {
                  case 'e': return LexKeyword(data, "f32.ge", Opcode::F32Ge);
                  case 't': return LexKeyword(data, "f32.gt", Opcode::F32Gt);
                  default: break;
                }
                break;
              case 'l':
                switch (PeekChar(data, 5)) {
                  case 'e': return LexKeyword(data, "f32.le", Opcode::F32Le);
                  case 't': return LexKeyword(data, "f32.lt", Opcode::F32Lt);
                  default: break;
                }
                break;
              case 'n':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.ne", Opcode::F32Ne);
                  case 'i': return LexKeyword(data, "i32.ne", Opcode::I32Ne);
                  default: break;
                }
                break;
              case 'o': return LexKeyword(data, "i32.or", Opcode::I32Or);
              default: break;
            }
            break;
          case '_':
            switch (PeekChar(data, 7)) {
              case 's':
                switch (PeekChar(data, 5)) {
                  case 'e':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i32.ge_s", Opcode::I32GeS);
                      case 'l': return LexKeyword(data, "i32.le_s", Opcode::I32LeS);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i32.gt_s", Opcode::I32GtS);
                      case 'l': return LexKeyword(data, "i32.lt_s", Opcode::I32LtS);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'u':
                switch (PeekChar(data, 5)) {
                  case 'e':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i32.ge_u", Opcode::I32GeU);
                      case 'l': return LexKeyword(data, "i32.le_u", Opcode::I32LeU);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i32.gt_u", Opcode::I32GtU);
                      case 'l': return LexKeyword(data, "i32.lt_u", Opcode::I32LtU);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              default: break;
            }
            break;
          case 'a':
            switch (PeekChar(data, 8)) {
              default:
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.load", TokenType::MemoryInstr, Opcode::F32Load);
                  case 'i': return LexKeyword(data, "i32.load", TokenType::MemoryInstr, Opcode::I32Load);
                  default: break;
                }
                break;
              case '/': return LexKeyword(data, "i32.wrap/i64", Opcode::I32WrapI64);
              case '1':
                switch (PeekChar(data, 11)) {
                  case 's': return LexKeyword(data, "i32.load16_s", TokenType::MemoryInstr, Opcode::I32Load16S);
                  case 'u': return LexKeyword(data, "i32.load16_u", TokenType::MemoryInstr, Opcode::I32Load16U);
                  default: break;
                }
                break;
              case '8':
                switch (PeekChar(data, 10)) {
                  case 's': return LexKeyword(data, "i32.load8_s", TokenType::MemoryInstr, Opcode::I32Load8S);
                  case 'u': return LexKeyword(data, "i32.load8_u", TokenType::MemoryInstr, Opcode::I32Load8U);
                  default: break;
                }
                break;
              case '_': return LexKeyword(data, "i32.wrap_i64", Opcode::I32WrapI64);
              case 'e': return LexKeyword(data, "f32.nearest", Opcode::F32Nearest);
            }
            break;
          case 'b':
            switch (PeekChar(data, 0)) {
              case 'f': return LexKeyword(data, "f32.sub", Opcode::F32Sub);
              case 'i': return LexKeyword(data, "i32.sub", Opcode::I32Sub);
              default: break;
            }
            break;
          case 'd':
            switch (PeekChar(data, 5)) {
              case 'd':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.add", Opcode::F32Add);
                  case 'i': return LexKeyword(data, "i32.add", Opcode::I32Add);
                  default: break;
                }
                break;
              case 'n': return LexKeyword(data, "i32.and", Opcode::I32And);
              default: break;
            }
            break;
          case 'g': return LexKeyword(data, "f32.neg", Opcode::F32Neg);
          case 'i':
            switch (PeekChar(data, 8)) {
              default: return LexKeyword(data, "f32.ceil", Opcode::F32Ceil);
              case 't':
                switch (PeekChar(data, 16)) {
                  case 'f':
                    switch (PeekChar(data, 15)) {
                      case '/': return LexKeyword(data, "i32.reinterpret/f32", Opcode::I32ReinterpretF32);
                      case '_': return LexKeyword(data, "i32.reinterpret_f32", Opcode::I32ReinterpretF32);
                      default: break;
                    }
                    break;
                  case 'i':
                    switch (PeekChar(data, 15)) {
                      case '/': return LexKeyword(data, "f32.reinterpret/i32", Opcode::F32ReinterpretI32);
                      case '_': return LexKeyword(data, "f32.reinterpret_i32", Opcode::F32ReinterpretI32);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'l':
            switch (PeekChar(data, 5)) {
              case 'h': return LexKeyword(data, "i32.shl", Opcode::I32Shl);
              case 'u':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.mul", Opcode::F32Mul);
                  case 'i': return LexKeyword(data, "i32.mul", Opcode::I32Mul);
                  default: break;
                }
                break;
              default: break;
            }
            break;
          case 'm':
            switch (PeekChar(data, 8)) {
              case 's': return LexKeyword(data, "i32.rem_s", Opcode::I32RemS);
              case 't':
                switch (PeekChar(data, 10)) {
                  case '/': return LexKeyword(data, "f32.demote/f64", Opcode::F32DemoteF64);
                  case '_': return LexKeyword(data, "f32.demote_f64", Opcode::F32DemoteF64);
                  default: break;
                }
                break;
              case 'u': return LexKeyword(data, "i32.rem_u", Opcode::I32RemU);
              default: break;
            }
            break;
          case 'n':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "f32.min", Opcode::F32Min);
              case 's':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32.const", TokenType::F32ConstInstr, Opcode::F32Const);
                  case 'i': return LexKeyword(data, "i32.const", TokenType::I32ConstInstr, Opcode::I32Const);
                  default: break;
                }
                break;
              case 'v':
                switch (PeekChar(data, 16)) {
                  case '2':
                    switch (PeekChar(data, 12)) {
                      case 's': return LexKeyword(data, "f32.convert_s/i32", Opcode::F32ConvertI32S);
                      case 'u': return LexKeyword(data, "f32.convert_u/i32", Opcode::F32ConvertI32U);
                      default: break;
                    }
                    break;
                  case '4':
                    switch (PeekChar(data, 12)) {
                      case 's': return LexKeyword(data, "f32.convert_s/i64", Opcode::F32ConvertI64S);
                      case 'u': return LexKeyword(data, "f32.convert_u/i64", Opcode::F32ConvertI64U);
                      default: break;
                    }
                    break;
                  case 's':
                    switch (PeekChar(data, 14)) {
                      case '2': return LexKeyword(data, "f32.convert_i32_s", Opcode::F32ConvertI32S);
                      case '4': return LexKeyword(data, "f32.convert_i64_s", Opcode::F32ConvertI64S);
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 14)) {
                      case '2': return LexKeyword(data, "f32.convert_i32_u", Opcode::F32ConvertI32U);
                      case '4': return LexKeyword(data, "f32.convert_i64_u", Opcode::F32ConvertI64U);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'o':
            switch (PeekChar(data, 9)) {
              default:
                switch (PeekChar(data, 8)) {
                  case 'e':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32.store", TokenType::MemoryInstr, Opcode::F32Store);
                      case 'i': return LexKeyword(data, "i32.store", TokenType::MemoryInstr, Opcode::I32Store);
                      default: break;
                    }
                    break;
                  case 'r': return LexKeyword(data, "f32.floor", Opcode::F32Floor);
                  default: break;
                }
                break;
              case '1': return LexKeyword(data, "i32.store16", TokenType::MemoryInstr, Opcode::I32Store16);
              case '8': return LexKeyword(data, "i32.store8", TokenType::MemoryInstr, Opcode::I32Store8);
              case 'c':
                switch (PeekChar(data, 15)) {
                  default: return LexKeyword(data, "i32.atomic.load", TokenType::MemoryInstr, Opcode::I32AtomicLoad, Features::Threads);
                  case '.':
                    switch (PeekChar(data, 17)) {
                      case 'c': return LexKeyword(data, "i32.atomic.rmw8.xchg_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8XchgU, Features::Threads);
                      case 'd': return LexKeyword(data, "i32.atomic.rmw8.add_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8AddU, Features::Threads);
                      case 'm': return LexKeyword(data, "i32.atomic.rmw8.cmpxchg_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8CmpxchgU, Features::Threads);
                      case 'n': return LexKeyword(data, "i32.atomic.rmw8.and_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8AndU, Features::Threads);
                      case 'o': return LexKeyword(data, "i32.atomic.rmw8.xor_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8XorU, Features::Threads);
                      case 'r': return LexKeyword(data, "i32.atomic.rmw8.or_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8OrU, Features::Threads);
                      case 'u': return LexKeyword(data, "i32.atomic.rmw8.sub_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw8SubU, Features::Threads);
                      default: break;
                    }
                    break;
                  case '1': return LexKeyword(data, "i32.atomic.load16_u", TokenType::MemoryInstr, Opcode::I32AtomicLoad16U, Features::Threads);
                  case '6':
                    switch (PeekChar(data, 18)) {
                      case 'c': return LexKeyword(data, "i32.atomic.rmw16.xchg_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16XchgU, Features::Threads);
                      case 'd': return LexKeyword(data, "i32.atomic.rmw16.add_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16AddU, Features::Threads);
                      case 'm': return LexKeyword(data, "i32.atomic.rmw16.cmpxchg_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16CmpxchgU, Features::Threads);
                      case 'n': return LexKeyword(data, "i32.atomic.rmw16.and_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16AndU, Features::Threads);
                      case 'o': return LexKeyword(data, "i32.atomic.rmw16.xor_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16XorU, Features::Threads);
                      case 'r': return LexKeyword(data, "i32.atomic.rmw16.or_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16OrU, Features::Threads);
                      case 'u': return LexKeyword(data, "i32.atomic.rmw16.sub_u", TokenType::MemoryInstr, Opcode::I32AtomicRmw16SubU, Features::Threads);
                      default: break;
                    }
                    break;
                  case '8': return LexKeyword(data, "i32.atomic.load8_u", TokenType::MemoryInstr, Opcode::I32AtomicLoad8U, Features::Threads);
                  case 'a':
                    switch (PeekChar(data, 16)) {
                      case 'd': return LexKeyword(data, "i32.atomic.rmw.add", TokenType::MemoryInstr, Opcode::I32AtomicRmwAdd, Features::Threads);
                      case 'n': return LexKeyword(data, "i32.atomic.rmw.and", TokenType::MemoryInstr, Opcode::I32AtomicRmwAnd, Features::Threads);
                      default: break;
                    }
                    break;
                  case 'c': return LexKeyword(data, "i32.atomic.rmw.cmpxchg", TokenType::MemoryInstr, Opcode::I32AtomicRmwCmpxchg, Features::Threads);
                  case 'e':
                    switch (PeekChar(data, 16)) {
                      default: return LexKeyword(data, "i32.atomic.store", TokenType::MemoryInstr, Opcode::I32AtomicStore, Features::Threads);
                      case '1': return LexKeyword(data, "i32.atomic.store16", TokenType::MemoryInstr, Opcode::I32AtomicStore16, Features::Threads);
                      case '8': return LexKeyword(data, "i32.atomic.store8", TokenType::MemoryInstr, Opcode::I32AtomicStore8, Features::Threads);
                    }
                    break;
                  case 'o': return LexKeyword(data, "i32.atomic.rmw.or", TokenType::MemoryInstr, Opcode::I32AtomicRmwOr, Features::Threads);
                  case 's': return LexKeyword(data, "i32.atomic.rmw.sub", TokenType::MemoryInstr, Opcode::I32AtomicRmwSub, Features::Threads);
                  case 'x':
                    switch (PeekChar(data, 18)) {
                      default: return LexKeyword(data, "i32.atomic.rmw.xor", TokenType::MemoryInstr, Opcode::I32AtomicRmwXor, Features::Threads);
                      case 'g': return LexKeyword(data, "i32.atomic.rmw.xchg", TokenType::MemoryInstr, Opcode::I32AtomicRmwXchg, Features::Threads);
                    }
                    break;
                }
                break;
            }
            break;
          case 'p':
            switch (PeekChar(data, 10)) {
              default: return LexKeyword(data, "i32.popcnt", Opcode::I32Popcnt);
              case 'g': return LexKeyword(data, "f32.copysign", Opcode::F32Copysign);
            }
            break;
          case 'r':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "i32.xor", Opcode::I32Xor);
              case '_':
                switch (PeekChar(data, 8)) {
                  case 's': return LexKeyword(data, "i32.shr_s", Opcode::I32ShrS);
                  case 'u': return LexKeyword(data, "i32.shr_u", Opcode::I32ShrU);
                  default: break;
                }
                break;
              case 't': return LexKeyword(data, "f32.sqrt", Opcode::F32Sqrt);
            }
            break;
          case 's': return LexKeyword(data, "f32.abs", Opcode::F32Abs);
          case 't':
            switch (PeekChar(data, 7)) {
              case 'e':
                switch (PeekChar(data, 13)) {
                  default: return LexKeyword(data, "i32.extend8_s", Opcode::I32Extend8S, Features::SignExtension);
                  case 's': return LexKeyword(data, "i32.extend16_s", Opcode::I32Extend16S, Features::SignExtension);
                }
                break;
              case 'l': return LexKeyword(data, "i32.rotl", Opcode::I32Rotl);
              case 'r': return LexKeyword(data, "i32.rotr", Opcode::I32Rotr);
              default: break;
            }
            break;
          case 'u':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "f32.trunc", Opcode::F32Trunc);
              case '_':
                switch (PeekChar(data, 14)) {
                  case '2':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i32.trunc_s/f32", Opcode::I32TruncF32S);
                      case 'u': return LexKeyword(data, "i32.trunc_u/f32", Opcode::I32TruncF32U);
                      default: break;
                    }
                    break;
                  case '4':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i32.trunc_s/f64", Opcode::I32TruncF64S);
                      case 'u': return LexKeyword(data, "i32.trunc_u/f64", Opcode::I32TruncF64U);
                      default: break;
                    }
                    break;
                  case 'f':
                    switch (PeekChar(data, 18)) {
                      case 's':
                        switch (PeekChar(data, 16)) {
                          case '2': return LexKeyword(data, "i32.trunc_sat_f32_s", Opcode::I32TruncSatF32S, Features::SaturatingFloatToInt);
                          case '4': return LexKeyword(data, "i32.trunc_sat_f64_s", Opcode::I32TruncSatF64S, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      case 'u':
                        switch (PeekChar(data, 16)) {
                          case '2': return LexKeyword(data, "i32.trunc_sat_f32_u", Opcode::I32TruncSatF32U, Features::SaturatingFloatToInt);
                          case '4': return LexKeyword(data, "i32.trunc_sat_f64_u", Opcode::I32TruncSatF64U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 's':
                    switch (PeekChar(data, 12)) {
                      case '2': return LexKeyword(data, "i32.trunc_f32_s", Opcode::I32TruncF32S);
                      case '4': return LexKeyword(data, "i32.trunc_f64_s", Opcode::I32TruncF64S);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 18)) {
                      case '2':
                        switch (PeekChar(data, 10)) {
                          case 's': return LexKeyword(data, "i32.trunc_s:sat/f32", Opcode::I32TruncSatF32S, Features::SaturatingFloatToInt);
                          case 'u': return LexKeyword(data, "i32.trunc_u:sat/f32", Opcode::I32TruncSatF32U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      case '4':
                        switch (PeekChar(data, 10)) {
                          case 's': return LexKeyword(data, "i32.trunc_s:sat/f64", Opcode::I32TruncSatF64S, Features::SaturatingFloatToInt);
                          case 'u': return LexKeyword(data, "i32.trunc_u:sat/f64", Opcode::I32TruncSatF64U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 12)) {
                      case '2': return LexKeyword(data, "i32.trunc_f32_u", Opcode::I32TruncF32U);
                      case '4': return LexKeyword(data, "i32.trunc_f64_u", Opcode::I32TruncF64U);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'v':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "f32.div", Opcode::F32Div);
              case '_':
                switch (PeekChar(data, 8)) {
                  case 's': return LexKeyword(data, "i32.div_s", Opcode::I32DivS);
                  case 'u': return LexKeyword(data, "i32.div_u", Opcode::I32DivU);
                  default: break;
                }
                break;
            }
            break;
          case 'x': return LexKeyword(data, "f32.max", Opcode::F32Max);
          case 'z':
            switch (PeekChar(data, 5)) {
              case 'l': return LexKeyword(data, "i32.clz", Opcode::I32Clz);
              case 'q': return LexKeyword(data, "i32.eqz", Opcode::I32Eqz);
              case 't': return LexKeyword(data, "i32.ctz", Opcode::I32Ctz);
              default: break;
            }
            break;
        }
        break;
      case '8':
        switch (PeekChar(data, 4)) {
          default: return LexKeyword(data, "v128", NumericType::V128);
          case '.':
            switch (PeekChar(data, 5)) {
              case 'a':
                switch (PeekChar(data, 8)) {
                  default: return LexKeyword(data, "v128.and", Opcode::V128And, Features::Simd);
                  case 'n': return LexKeyword(data, "v128.andnot", Opcode::V128Andnot, Features::Simd);
                }
                break;
              case 'b': return LexKeyword(data, "v128.bitselect", Opcode::V128BitSelect, Features::Simd);
              case 'c': return LexKeyword(data, "v128.const", TokenType::SimdConstInstr, Opcode::V128Const, Features::Simd);
              case 'l':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "v128.load", TokenType::MemoryInstr, Opcode::V128Load, Features::Simd);
                  case '1':
                    switch (PeekChar(data, 14)) {
                      case 'l': return LexKeyword(data, "v128.load16_splat", TokenType::MemoryInstr, Opcode::V128Load16Splat, Features::Simd);
                      case 's': return LexKeyword(data, "v128.load16x4_s", TokenType::MemoryInstr, Opcode::V128Load16X4S, Features::Simd);
                      case 'u': return LexKeyword(data, "v128.load16x4_u", TokenType::MemoryInstr, Opcode::V128Load16X4U, Features::Simd);
                      default: break;
                    }
                    break;
                  case '3':
                    switch (PeekChar(data, 14)) {
                      case 'l': return LexKeyword(data, "v128.load32_splat", TokenType::MemoryInstr, Opcode::V128Load32Splat, Features::Simd);
                      case 's': return LexKeyword(data, "v128.load32x2_s", TokenType::MemoryInstr, Opcode::V128Load32X2S, Features::Simd);
                      case 'u': return LexKeyword(data, "v128.load32x2_u", TokenType::MemoryInstr, Opcode::V128Load32X2U, Features::Simd);
                      default: break;
                    }
                    break;
                  case '6': return LexKeyword(data, "v128.load64_splat", TokenType::MemoryInstr, Opcode::V128Load64Splat, Features::Simd);
                  case '8':
                    switch (PeekChar(data, 13)) {
                      case 'l': return LexKeyword(data, "v128.load8_splat", TokenType::MemoryInstr, Opcode::V128Load8Splat, Features::Simd);
                      case 's': return LexKeyword(data, "v128.load8x8_s", TokenType::MemoryInstr, Opcode::V128Load8X8S, Features::Simd);
                      case 'u': return LexKeyword(data, "v128.load8x8_u", TokenType::MemoryInstr, Opcode::V128Load8X8U, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'n': return LexKeyword(data, "v128.not", Opcode::V128Not, Features::Simd);
              case 'o': return LexKeyword(data, "v128.or", Opcode::V128Or, Features::Simd);
              case 's': return LexKeyword(data, "v128.store", TokenType::MemoryInstr, Opcode::V128Store, Features::Simd);
              case 'x': return LexKeyword(data, "v128.xor", Opcode::V128Xor, Features::Simd);
              default: break;
            }
            break;
        }
        break;
      case 'x':
        switch (PeekChar(data, 5)) {
          default:
            switch (PeekChar(data, 0)) {
              case 'f': return LexKeyword(data, "f32x4", TokenType::F32X4);
              case 'i': return LexKeyword(data, "i32x4", TokenType::I32X4);
              default: break;
            }
            break;
          case '.':
            switch (PeekChar(data, 8)) {
              default:
                switch (PeekChar(data, 6)) {
                  case 'e':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32x4.eq", Opcode::F32X4Eq, Features::Simd);
                      case 'i': return LexKeyword(data, "i32x4.eq", Opcode::I32X4Eq, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'g':
                    switch (PeekChar(data, 7)) {
                      case 'e': return LexKeyword(data, "f32x4.ge", Opcode::F32X4Ge, Features::Simd);
                      case 't': return LexKeyword(data, "f32x4.gt", Opcode::F32X4Gt, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'l':
                    switch (PeekChar(data, 7)) {
                      case 'e': return LexKeyword(data, "f32x4.le", Opcode::F32X4Le, Features::Simd);
                      case 't': return LexKeyword(data, "f32x4.lt", Opcode::F32X4Lt, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'n':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32x4.ne", Opcode::F32X4Ne, Features::Simd);
                      case 'i': return LexKeyword(data, "i32x4.ne", Opcode::I32X4Ne, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case '_':
                switch (PeekChar(data, 9)) {
                  case 's':
                    switch (PeekChar(data, 7)) {
                      case 'e':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i32x4.ge_s", Opcode::I32X4GeS, Features::Simd);
                          case 'l': return LexKeyword(data, "i32x4.le_s", Opcode::I32X4LeS, Features::Simd);
                          default: break;
                        }
                        break;
                      case 't':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i32x4.gt_s", Opcode::I32X4GtS, Features::Simd);
                          case 'l': return LexKeyword(data, "i32x4.lt_s", Opcode::I32X4LtS, Features::Simd);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 7)) {
                      case 'e':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i32x4.ge_u", Opcode::I32X4GeU, Features::Simd);
                          case 'l': return LexKeyword(data, "i32x4.le_u", Opcode::I32X4LeU, Features::Simd);
                          default: break;
                        }
                        break;
                      case 't':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i32x4.gt_u", Opcode::I32X4GtU, Features::Simd);
                          case 'l': return LexKeyword(data, "i32x4.lt_u", Opcode::I32X4LtU, Features::Simd);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'b':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32x4.sub", Opcode::F32X4Sub, Features::Simd);
                  case 'i': return LexKeyword(data, "i32x4.sub", Opcode::I32X4Sub, Features::Simd);
                  default: break;
                }
                break;
              case 'd':
                switch (PeekChar(data, 9)) {
                  default:
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32x4.add", Opcode::F32X4Add, Features::Simd);
                      case 'i': return LexKeyword(data, "i32x4.add", Opcode::I32X4Add, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'e':
                    switch (PeekChar(data, 23)) {
                      default:
                        switch (PeekChar(data, 22)) {
                          case 's': return LexKeyword(data, "i32x4.widen_low_i16x8_s", Opcode::I32X4WidenLowI16X8S, Features::Simd);
                          case 'u': return LexKeyword(data, "i32x4.widen_low_i16x8_u", Opcode::I32X4WidenLowI16X8U, Features::Simd);
                          default: break;
                        }
                        break;
                      case 's': return LexKeyword(data, "i32x4.widen_high_i16x8_s", Opcode::I32X4WidenHighI16X8S, Features::Simd);
                      case 'u': return LexKeyword(data, "i32x4.widen_high_i16x8_u", Opcode::I32X4WidenHighI16X8U, Features::Simd);
                    }
                    break;
                }
                break;
              case 'g':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32x4.neg", Opcode::F32X4Neg, Features::Simd);
                  case 'i': return LexKeyword(data, "i32x4.neg", Opcode::I32X4Neg, Features::Simd);
                  default: break;
                }
                break;
              case 'l':
                switch (PeekChar(data, 7)) {
                  case 'h': return LexKeyword(data, "i32x4.shl", Opcode::I32X4Shl, Features::Simd);
                  case 'l': return LexKeyword(data, "i32x4.all_true", Opcode::I32X4AllTrue, Features::Simd);
                  case 'p':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32x4.splat", Opcode::F32X4Splat, Features::Simd);
                      case 'i': return LexKeyword(data, "i32x4.splat", Opcode::I32X4Splat, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f32x4.mul", Opcode::F32X4Mul, Features::Simd);
                      case 'i': return LexKeyword(data, "i32x4.mul", Opcode::I32X4Mul, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'n':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "f32x4.min", Opcode::F32X4Min, Features::Simd);
                  case '_':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i32x4.min_s", Opcode::I32X4MinS, Features::Simd);
                      case 'u': return LexKeyword(data, "i32x4.min_u", Opcode::I32X4MinU, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'v':
                    switch (PeekChar(data, 20)) {
                      case 's': return LexKeyword(data, "f32x4.convert_i32x4_s", Opcode::F32X4ConvertI32X4S, Features::Simd);
                      case 'u': return LexKeyword(data, "f32x4.convert_i32x4_u", Opcode::F32X4ConvertI32X4U, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'p':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32x4.replace_lane", TokenType::SimdLaneInstr, Opcode::F32X4ReplaceLane, Features::Simd);
                  case 'i': return LexKeyword(data, "i32x4.replace_lane", TokenType::SimdLaneInstr, Opcode::I32X4ReplaceLane, Features::Simd);
                  default: break;
                }
                break;
              case 'r':
                switch (PeekChar(data, 10)) {
                  default: return LexKeyword(data, "f32x4.sqrt", Opcode::F32X4Sqrt, Features::Simd);
                  case 's': return LexKeyword(data, "i32x4.shr_s", Opcode::I32X4ShrS, Features::Simd);
                  case 'u': return LexKeyword(data, "i32x4.shr_u", Opcode::I32X4ShrU, Features::Simd);
                }
                break;
              case 's':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32x4.abs", Opcode::F32X4Abs, Features::Simd);
                  case 'i': return LexKeyword(data, "i32x4.abs", Opcode::I32X4Abs, Features::Simd);
                  default: break;
                }
                break;
              case 't':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f32x4.extract_lane", TokenType::SimdLaneInstr, Opcode::F32X4ExtractLane, Features::Simd);
                  case 'i': return LexKeyword(data, "i32x4.extract_lane", TokenType::SimdLaneInstr, Opcode::I32X4ExtractLane, Features::Simd);
                  default: break;
                }
                break;
              case 'u':
                switch (PeekChar(data, 22)) {
                  case 's': return LexKeyword(data, "i32x4.trunc_sat_f32x4_s", Opcode::I32X4TruncSatF32X4S, Features::Simd);
                  case 'u': return LexKeyword(data, "i32x4.trunc_sat_f32x4_u", Opcode::I32X4TruncSatF32X4U, Features::Simd);
                  default: break;
                }
                break;
              case 'v': return LexKeyword(data, "f32x4.div", Opcode::F32X4Div, Features::Simd);
              case 'x':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "f32x4.max", Opcode::F32X4Max, Features::Simd);
                  case '_':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i32x4.max_s", Opcode::I32X4MaxS, Features::Simd);
                      case 'u': return LexKeyword(data, "i32x4.max_u", Opcode::I32X4MaxU, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'y': return LexKeyword(data, "i32x4.any_true", Opcode::I32X4AnyTrue, Features::Simd);
            }
            break;
        }
        break;
    }
    break;
  case '4':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 0)) {
          case 'f': return LexKeyword(data, "f64", NumericType::F64);
          case 'i': return LexKeyword(data, "i64", NumericType::I64);
          default: break;
        }
        break;
      case '.':
        switch (PeekChar(data, 6)) {
          default:
            switch (PeekChar(data, 4)) {
              case 'e':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.eq", Opcode::F64Eq);
                  case 'i': return LexKeyword(data, "i64.eq", Opcode::I64Eq);
                  default: break;
                }
                break;
              case 'g':
                switch (PeekChar(data, 5)) {
                  case 'e': return LexKeyword(data, "f64.ge", Opcode::F64Ge);
                  case 't': return LexKeyword(data, "f64.gt", Opcode::F64Gt);
                  default: break;
                }
                break;
              case 'l':
                switch (PeekChar(data, 5)) {
                  case 'e': return LexKeyword(data, "f64.le", Opcode::F64Le);
                  case 't': return LexKeyword(data, "f64.lt", Opcode::F64Lt);
                  default: break;
                }
                break;
              case 'n':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.ne", Opcode::F64Ne);
                  case 'i': return LexKeyword(data, "i64.ne", Opcode::I64Ne);
                  default: break;
                }
                break;
              case 'o': return LexKeyword(data, "i64.or", Opcode::I64Or);
              default: break;
            }
            break;
          case '_':
            switch (PeekChar(data, 7)) {
              case 's':
                switch (PeekChar(data, 5)) {
                  case 'e':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i64.ge_s", Opcode::I64GeS);
                      case 'l': return LexKeyword(data, "i64.le_s", Opcode::I64LeS);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i64.gt_s", Opcode::I64GtS);
                      case 'l': return LexKeyword(data, "i64.lt_s", Opcode::I64LtS);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'u':
                switch (PeekChar(data, 5)) {
                  case 'e':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i64.ge_u", Opcode::I64GeU);
                      case 'l': return LexKeyword(data, "i64.le_u", Opcode::I64LeU);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 4)) {
                      case 'g': return LexKeyword(data, "i64.gt_u", Opcode::I64GtU);
                      case 'l': return LexKeyword(data, "i64.lt_u", Opcode::I64LtU);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              default: break;
            }
            break;
          case 'a':
            switch (PeekChar(data, 8)) {
              default:
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.load", TokenType::MemoryInstr, Opcode::F64Load);
                  case 'i': return LexKeyword(data, "i64.load", TokenType::MemoryInstr, Opcode::I64Load);
                  default: break;
                }
                break;
              case '1':
                switch (PeekChar(data, 11)) {
                  case 's': return LexKeyword(data, "i64.load16_s", TokenType::MemoryInstr, Opcode::I64Load16S);
                  case 'u': return LexKeyword(data, "i64.load16_u", TokenType::MemoryInstr, Opcode::I64Load16U);
                  default: break;
                }
                break;
              case '3':
                switch (PeekChar(data, 11)) {
                  case 's': return LexKeyword(data, "i64.load32_s", TokenType::MemoryInstr, Opcode::I64Load32S);
                  case 'u': return LexKeyword(data, "i64.load32_u", TokenType::MemoryInstr, Opcode::I64Load32U);
                  default: break;
                }
                break;
              case '8':
                switch (PeekChar(data, 10)) {
                  case 's': return LexKeyword(data, "i64.load8_s", TokenType::MemoryInstr, Opcode::I64Load8S);
                  case 'u': return LexKeyword(data, "i64.load8_u", TokenType::MemoryInstr, Opcode::I64Load8U);
                  default: break;
                }
                break;
              case 'e': return LexKeyword(data, "f64.nearest", Opcode::F64Nearest);
            }
            break;
          case 'b':
            switch (PeekChar(data, 0)) {
              case 'f': return LexKeyword(data, "f64.sub", Opcode::F64Sub);
              case 'i': return LexKeyword(data, "i64.sub", Opcode::I64Sub);
              default: break;
            }
            break;
          case 'd':
            switch (PeekChar(data, 5)) {
              case 'd':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.add", Opcode::F64Add);
                  case 'i': return LexKeyword(data, "i64.add", Opcode::I64Add);
                  default: break;
                }
                break;
              case 'n': return LexKeyword(data, "i64.and", Opcode::I64And);
              default: break;
            }
            break;
          case 'g': return LexKeyword(data, "f64.neg", Opcode::F64Neg);
          case 'i':
            switch (PeekChar(data, 8)) {
              default: return LexKeyword(data, "f64.ceil", Opcode::F64Ceil);
              case 't':
                switch (PeekChar(data, 16)) {
                  case 'f':
                    switch (PeekChar(data, 15)) {
                      case '/': return LexKeyword(data, "i64.reinterpret/f64", Opcode::I64ReinterpretF64);
                      case '_': return LexKeyword(data, "i64.reinterpret_f64", Opcode::I64ReinterpretF64);
                      default: break;
                    }
                    break;
                  case 'i':
                    switch (PeekChar(data, 15)) {
                      case '/': return LexKeyword(data, "f64.reinterpret/i64", Opcode::F64ReinterpretI64);
                      case '_': return LexKeyword(data, "f64.reinterpret_i64", Opcode::F64ReinterpretI64);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'l':
            switch (PeekChar(data, 5)) {
              case 'h': return LexKeyword(data, "i64.shl", Opcode::I64Shl);
              case 'u':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.mul", Opcode::F64Mul);
                  case 'i': return LexKeyword(data, "i64.mul", Opcode::I64Mul);
                  default: break;
                }
                break;
              default: break;
            }
            break;
          case 'm':
            switch (PeekChar(data, 8)) {
              case 's': return LexKeyword(data, "i64.rem_s", Opcode::I64RemS);
              case 'u': return LexKeyword(data, "i64.rem_u", Opcode::I64RemU);
              default: break;
            }
            break;
          case 'n':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "f64.min", Opcode::F64Min);
              case 's':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64.const", TokenType::F64ConstInstr, Opcode::F64Const);
                  case 'i': return LexKeyword(data, "i64.const", TokenType::I64ConstInstr, Opcode::I64Const);
                  default: break;
                }
                break;
              case 'v':
                switch (PeekChar(data, 16)) {
                  case '2':
                    switch (PeekChar(data, 12)) {
                      case 's': return LexKeyword(data, "f64.convert_s/i32", Opcode::F64ConvertI32S);
                      case 'u': return LexKeyword(data, "f64.convert_u/i32", Opcode::F64ConvertI32U);
                      default: break;
                    }
                    break;
                  case '4':
                    switch (PeekChar(data, 12)) {
                      case 's': return LexKeyword(data, "f64.convert_s/i64", Opcode::F64ConvertI64S);
                      case 'u': return LexKeyword(data, "f64.convert_u/i64", Opcode::F64ConvertI64U);
                      default: break;
                    }
                    break;
                  case 's':
                    switch (PeekChar(data, 14)) {
                      case '2': return LexKeyword(data, "f64.convert_i32_s", Opcode::F64ConvertI32S);
                      case '4': return LexKeyword(data, "f64.convert_i64_s", Opcode::F64ConvertI64S);
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 14)) {
                      case '2': return LexKeyword(data, "f64.convert_i32_u", Opcode::F64ConvertI32U);
                      case '4': return LexKeyword(data, "f64.convert_i64_u", Opcode::F64ConvertI64U);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'o':
            switch (PeekChar(data, 9)) {
              default:
                switch (PeekChar(data, 8)) {
                  case 'e':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f64.store", TokenType::MemoryInstr, Opcode::F64Store);
                      case 'i': return LexKeyword(data, "i64.store", TokenType::MemoryInstr, Opcode::I64Store);
                      default: break;
                    }
                    break;
                  case 'r': return LexKeyword(data, "f64.floor", Opcode::F64Floor);
                  default: break;
                }
                break;
              case '1': return LexKeyword(data, "i64.store16", TokenType::MemoryInstr, Opcode::I64Store16);
              case '3': return LexKeyword(data, "i64.store32", TokenType::MemoryInstr, Opcode::I64Store32);
              case '8': return LexKeyword(data, "i64.store8", TokenType::MemoryInstr, Opcode::I64Store8);
              case 'c':
                switch (PeekChar(data, 15)) {
                  default: return LexKeyword(data, "i64.atomic.load", TokenType::MemoryInstr, Opcode::I64AtomicLoad, Features::Threads);
                  case '.':
                    switch (PeekChar(data, 17)) {
                      case 'c': return LexKeyword(data, "i64.atomic.rmw8.xchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8XchgU, Features::Threads);
                      case 'd': return LexKeyword(data, "i64.atomic.rmw8.add_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8AddU, Features::Threads);
                      case 'm': return LexKeyword(data, "i64.atomic.rmw8.cmpxchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8CmpxchgU, Features::Threads);
                      case 'n': return LexKeyword(data, "i64.atomic.rmw8.and_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8AndU, Features::Threads);
                      case 'o': return LexKeyword(data, "i64.atomic.rmw8.xor_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8XorU, Features::Threads);
                      case 'r': return LexKeyword(data, "i64.atomic.rmw8.or_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8OrU, Features::Threads);
                      case 'u': return LexKeyword(data, "i64.atomic.rmw8.sub_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw8SubU, Features::Threads);
                      default: break;
                    }
                    break;
                  case '1': return LexKeyword(data, "i64.atomic.load16_u", TokenType::MemoryInstr, Opcode::I64AtomicLoad16U, Features::Threads);
                  case '2':
                    switch (PeekChar(data, 18)) {
                      case 'c': return LexKeyword(data, "i64.atomic.rmw32.xchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32XchgU, Features::Threads);
                      case 'd': return LexKeyword(data, "i64.atomic.rmw32.add_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32AddU, Features::Threads);
                      case 'm': return LexKeyword(data, "i64.atomic.rmw32.cmpxchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32CmpxchgU, Features::Threads);
                      case 'n': return LexKeyword(data, "i64.atomic.rmw32.and_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32AndU, Features::Threads);
                      case 'o': return LexKeyword(data, "i64.atomic.rmw32.xor_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32XorU, Features::Threads);
                      case 'r': return LexKeyword(data, "i64.atomic.rmw32.or_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32OrU, Features::Threads);
                      case 'u': return LexKeyword(data, "i64.atomic.rmw32.sub_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw32SubU, Features::Threads);
                      default: break;
                    }
                    break;
                  case '3': return LexKeyword(data, "i64.atomic.load32_u", TokenType::MemoryInstr, Opcode::I64AtomicLoad32U, Features::Threads);
                  case '6':
                    switch (PeekChar(data, 18)) {
                      case 'c': return LexKeyword(data, "i64.atomic.rmw16.xchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16XchgU, Features::Threads);
                      case 'd': return LexKeyword(data, "i64.atomic.rmw16.add_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16AddU, Features::Threads);
                      case 'm': return LexKeyword(data, "i64.atomic.rmw16.cmpxchg_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16CmpxchgU, Features::Threads);
                      case 'n': return LexKeyword(data, "i64.atomic.rmw16.and_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16AndU, Features::Threads);
                      case 'o': return LexKeyword(data, "i64.atomic.rmw16.xor_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16XorU, Features::Threads);
                      case 'r': return LexKeyword(data, "i64.atomic.rmw16.or_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16OrU, Features::Threads);
                      case 'u': return LexKeyword(data, "i64.atomic.rmw16.sub_u", TokenType::MemoryInstr, Opcode::I64AtomicRmw16SubU, Features::Threads);
                      default: break;
                    }
                    break;
                  case '8': return LexKeyword(data, "i64.atomic.load8_u", TokenType::MemoryInstr, Opcode::I64AtomicLoad8U, Features::Threads);
                  case 'a':
                    switch (PeekChar(data, 16)) {
                      case 'd': return LexKeyword(data, "i64.atomic.rmw.add", TokenType::MemoryInstr, Opcode::I64AtomicRmwAdd, Features::Threads);
                      case 'n': return LexKeyword(data, "i64.atomic.rmw.and", TokenType::MemoryInstr, Opcode::I64AtomicRmwAnd, Features::Threads);
                      default: break;
                    }
                    break;
                  case 'c': return LexKeyword(data, "i64.atomic.rmw.cmpxchg", TokenType::MemoryInstr, Opcode::I64AtomicRmwCmpxchg, Features::Threads);
                  case 'e':
                    switch (PeekChar(data, 16)) {
                      default: return LexKeyword(data, "i64.atomic.store", TokenType::MemoryInstr, Opcode::I64AtomicStore, Features::Threads);
                      case '1': return LexKeyword(data, "i64.atomic.store16", TokenType::MemoryInstr, Opcode::I64AtomicStore16, Features::Threads);
                      case '3': return LexKeyword(data, "i64.atomic.store32", TokenType::MemoryInstr, Opcode::I64AtomicStore32, Features::Threads);
                      case '8': return LexKeyword(data, "i64.atomic.store8", TokenType::MemoryInstr, Opcode::I64AtomicStore8, Features::Threads);
                    }
                    break;
                  case 'o': return LexKeyword(data, "i64.atomic.rmw.or", TokenType::MemoryInstr, Opcode::I64AtomicRmwOr, Features::Threads);
                  case 's': return LexKeyword(data, "i64.atomic.rmw.sub", TokenType::MemoryInstr, Opcode::I64AtomicRmwSub, Features::Threads);
                  case 'x':
                    switch (PeekChar(data, 18)) {
                      default: return LexKeyword(data, "i64.atomic.rmw.xor", TokenType::MemoryInstr, Opcode::I64AtomicRmwXor, Features::Threads);
                      case 'g': return LexKeyword(data, "i64.atomic.rmw.xchg", TokenType::MemoryInstr, Opcode::I64AtomicRmwXchg, Features::Threads);
                    }
                    break;
                }
                break;
              case 't':
                switch (PeekChar(data, 11)) {
                  case '/': return LexKeyword(data, "f64.promote/f32", Opcode::F64PromoteF32);
                  case '_': return LexKeyword(data, "f64.promote_f32", Opcode::F64PromoteF32);
                  default: break;
                }
                break;
            }
            break;
          case 'p':
            switch (PeekChar(data, 10)) {
              default: return LexKeyword(data, "i64.popcnt", Opcode::I64Popcnt);
              case 'g': return LexKeyword(data, "f64.copysign", Opcode::F64Copysign);
            }
            break;
          case 'r':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "i64.xor", Opcode::I64Xor);
              case '_':
                switch (PeekChar(data, 8)) {
                  case 's': return LexKeyword(data, "i64.shr_s", Opcode::I64ShrS);
                  case 'u': return LexKeyword(data, "i64.shr_u", Opcode::I64ShrU);
                  default: break;
                }
                break;
              case 't': return LexKeyword(data, "f64.sqrt", Opcode::F64Sqrt);
            }
            break;
          case 's': return LexKeyword(data, "f64.abs", Opcode::F64Abs);
          case 't':
            switch (PeekChar(data, 7)) {
              case 'e':
                switch (PeekChar(data, 11)) {
                  case '2': return LexKeyword(data, "i64.extend32_s", Opcode::I64Extend32S, Features::SignExtension);
                  case '6': return LexKeyword(data, "i64.extend16_s", Opcode::I64Extend16S, Features::SignExtension);
                  case '_': return LexKeyword(data, "i64.extend8_s", Opcode::I64Extend8S, Features::SignExtension);
                  case 'i':
                    switch (PeekChar(data, 15)) {
                      case 's': return LexKeyword(data, "i64.extend_i32_s", Opcode::I64ExtendI32S);
                      case 'u': return LexKeyword(data, "i64.extend_i32_u", Opcode::I64ExtendI32U);
                      default: break;
                    }
                    break;
                  case 's': return LexKeyword(data, "i64.extend_s/i32", Opcode::I64ExtendI32S);
                  case 'u': return LexKeyword(data, "i64.extend_u/i32", Opcode::I64ExtendI32U);
                  default: break;
                }
                break;
              case 'l': return LexKeyword(data, "i64.rotl", Opcode::I64Rotl);
              case 'r': return LexKeyword(data, "i64.rotr", Opcode::I64Rotr);
              default: break;
            }
            break;
          case 'u':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "f64.trunc", Opcode::F64Trunc);
              case '_':
                switch (PeekChar(data, 14)) {
                  case '2':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i64.trunc_s/f32", Opcode::I64TruncF32S);
                      case 'u': return LexKeyword(data, "i64.trunc_u/f32", Opcode::I64TruncF32U);
                      default: break;
                    }
                    break;
                  case '4':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i64.trunc_s/f64", Opcode::I64TruncF64S);
                      case 'u': return LexKeyword(data, "i64.trunc_u/f64", Opcode::I64TruncF64U);
                      default: break;
                    }
                    break;
                  case 'f':
                    switch (PeekChar(data, 18)) {
                      case 's':
                        switch (PeekChar(data, 16)) {
                          case '2': return LexKeyword(data, "i64.trunc_sat_f32_s", Opcode::I64TruncSatF32S, Features::SaturatingFloatToInt);
                          case '4': return LexKeyword(data, "i64.trunc_sat_f64_s", Opcode::I64TruncSatF64S, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      case 'u':
                        switch (PeekChar(data, 16)) {
                          case '2': return LexKeyword(data, "i64.trunc_sat_f32_u", Opcode::I64TruncSatF32U, Features::SaturatingFloatToInt);
                          case '4': return LexKeyword(data, "i64.trunc_sat_f64_u", Opcode::I64TruncSatF64U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 's':
                    switch (PeekChar(data, 12)) {
                      case '2': return LexKeyword(data, "i64.trunc_f32_s", Opcode::I64TruncF32S);
                      case '4': return LexKeyword(data, "i64.trunc_f64_s", Opcode::I64TruncF64S);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 18)) {
                      case '2':
                        switch (PeekChar(data, 10)) {
                          case 's': return LexKeyword(data, "i64.trunc_s:sat/f32", Opcode::I64TruncSatF32S, Features::SaturatingFloatToInt);
                          case 'u': return LexKeyword(data, "i64.trunc_u:sat/f32", Opcode::I64TruncSatF32U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      case '4':
                        switch (PeekChar(data, 10)) {
                          case 's': return LexKeyword(data, "i64.trunc_s:sat/f64", Opcode::I64TruncSatF64S, Features::SaturatingFloatToInt);
                          case 'u': return LexKeyword(data, "i64.trunc_u:sat/f64", Opcode::I64TruncSatF64U, Features::SaturatingFloatToInt);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 12)) {
                      case '2': return LexKeyword(data, "i64.trunc_f32_u", Opcode::I64TruncF32U);
                      case '4': return LexKeyword(data, "i64.trunc_f64_u", Opcode::I64TruncF64U);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
            }
            break;
          case 'v':
            switch (PeekChar(data, 7)) {
              default: return LexKeyword(data, "f64.div", Opcode::F64Div);
              case '_':
                switch (PeekChar(data, 8)) {
                  case 's': return LexKeyword(data, "i64.div_s", Opcode::I64DivS);
                  case 'u': return LexKeyword(data, "i64.div_u", Opcode::I64DivU);
                  default: break;
                }
                break;
            }
            break;
          case 'x': return LexKeyword(data, "f64.max", Opcode::F64Max);
          case 'z':
            switch (PeekChar(data, 5)) {
              case 'l': return LexKeyword(data, "i64.clz", Opcode::I64Clz);
              case 'q': return LexKeyword(data, "i64.eqz", Opcode::I64Eqz);
              case 't': return LexKeyword(data, "i64.ctz", Opcode::I64Ctz);
              default: break;
            }
            break;
        }
        break;
      case 'x':
        switch (PeekChar(data, 5)) {
          default:
            switch (PeekChar(data, 0)) {
              case 'f': return LexKeyword(data, "f64x2", TokenType::F64X2);
              case 'i': return LexKeyword(data, "i64x2", TokenType::I64X2);
              default: break;
            }
            break;
          case '.':
            switch (PeekChar(data, 8)) {
              default:
                switch (PeekChar(data, 6)) {
                  case 'e': return LexKeyword(data, "f64x2.eq", Opcode::F64X2Eq, Features::Simd);
                  case 'g':
                    switch (PeekChar(data, 7)) {
                      case 'e': return LexKeyword(data, "f64x2.ge", Opcode::F64X2Ge, Features::Simd);
                      case 't': return LexKeyword(data, "f64x2.gt", Opcode::F64X2Gt, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'l':
                    switch (PeekChar(data, 7)) {
                      case 'e': return LexKeyword(data, "f64x2.le", Opcode::F64X2Le, Features::Simd);
                      case 't': return LexKeyword(data, "f64x2.lt", Opcode::F64X2Lt, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'n': return LexKeyword(data, "f64x2.ne", Opcode::F64X2Ne, Features::Simd);
                  default: break;
                }
                break;
              case 'b':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64x2.sub", Opcode::F64X2Sub, Features::Simd);
                  case 'i': return LexKeyword(data, "i64x2.sub", Opcode::I64X2Sub, Features::Simd);
                  default: break;
                }
                break;
              case 'd':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64x2.add", Opcode::F64X2Add, Features::Simd);
                  case 'i': return LexKeyword(data, "i64x2.add", Opcode::I64X2Add, Features::Simd);
                  default: break;
                }
                break;
              case 'g':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64x2.neg", Opcode::F64X2Neg, Features::Simd);
                  case 'i': return LexKeyword(data, "i64x2.neg", Opcode::I64X2Neg, Features::Simd);
                  default: break;
                }
                break;
              case 'l':
                switch (PeekChar(data, 7)) {
                  case 'h': return LexKeyword(data, "i64x2.shl", Opcode::I64X2Shl, Features::Simd);
                  case 'p':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f64x2.splat", Opcode::F64X2Splat, Features::Simd);
                      case 'i': return LexKeyword(data, "i64x2.splat", Opcode::I64X2Splat, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 0)) {
                      case 'f': return LexKeyword(data, "f64x2.mul", Opcode::F64X2Mul, Features::Simd);
                      case 'i': return LexKeyword(data, "i64x2.mul", Opcode::I64X2Mul, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'n': return LexKeyword(data, "f64x2.min", Opcode::F64X2Min, Features::Simd);
              case 'p':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64x2.replace_lane", TokenType::SimdLaneInstr, Opcode::F64X2ReplaceLane, Features::Simd);
                  case 'i': return LexKeyword(data, "i64x2.replace_lane", TokenType::SimdLaneInstr, Opcode::I64X2ReplaceLane, Features::Simd);
                  default: break;
                }
                break;
              case 'r':
                switch (PeekChar(data, 10)) {
                  default: return LexKeyword(data, "f64x2.sqrt", Opcode::F64X2Sqrt, Features::Simd);
                  case 's': return LexKeyword(data, "i64x2.shr_s", Opcode::I64X2ShrS, Features::Simd);
                  case 'u': return LexKeyword(data, "i64x2.shr_u", Opcode::I64X2ShrU, Features::Simd);
                }
                break;
              case 's': return LexKeyword(data, "f64x2.abs", Opcode::F64X2Abs, Features::Simd);
              case 't':
                switch (PeekChar(data, 0)) {
                  case 'f': return LexKeyword(data, "f64x2.extract_lane", TokenType::SimdLaneInstr, Opcode::F64X2ExtractLane, Features::Simd);
                  case 'i': return LexKeyword(data, "i64x2.extract_lane", TokenType::SimdLaneInstr, Opcode::I64X2ExtractLane, Features::Simd);
                  default: break;
                }
                break;
              case 'v': return LexKeyword(data, "f64x2.div", Opcode::F64X2Div, Features::Simd);
              case 'x': return LexKeyword(data, "f64x2.max", Opcode::F64X2Max, Features::Simd);
            }
            break;
        }
        break;
    }
    break;
  case '6':
    switch (PeekChar(data, 3)) {
      default: return LexKeyword(data, "i16", PackedType::I16);
      case 'x':
        switch (PeekChar(data, 5)) {
          default: return LexKeyword(data, "i16x8", TokenType::I16X8);
          case '.':
            switch (PeekChar(data, 7)) {
              case 'a':
                switch (PeekChar(data, 10)) {
                  case 'o':
                    switch (PeekChar(data, 19)) {
                      case 's': return LexKeyword(data, "i16x8.narrow_i32x4_s", Opcode::I16X8NarrowI32X4S, Features::Simd);
                      case 'u': return LexKeyword(data, "i16x8.narrow_i32x4_u", Opcode::I16X8NarrowI32X4U, Features::Simd);
                      default: break;
                    }
                    break;
                  case 's': return LexKeyword(data, "i16x8.max_s", Opcode::I16X8MaxS, Features::Simd);
                  case 'u': return LexKeyword(data, "i16x8.max_u", Opcode::I16X8MaxU, Features::Simd);
                  default: break;
                }
                break;
              case 'b': return LexKeyword(data, "i16x8.abs", Opcode::I16X8Abs, Features::Simd);
              case 'd':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "i16x8.add", Opcode::I16X8Add, Features::Simd);
                  case '_':
                    switch (PeekChar(data, 14)) {
                      case 's': return LexKeyword(data, "i16x8.add_sat_s", Opcode::I16X8AddSatS, Features::Simd);
                      case 'u': return LexKeyword(data, "i16x8.add_sat_u", Opcode::I16X8AddSatU, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'e':
                switch (PeekChar(data, 8)) {
                  default: return LexKeyword(data, "i16x8.ne", Opcode::I16X8Ne, Features::Simd);
                  case '_':
                    switch (PeekChar(data, 9)) {
                      case 's':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i16x8.ge_s", Opcode::I16X8GeS, Features::Simd);
                          case 'l': return LexKeyword(data, "i16x8.le_s", Opcode::I16X8LeS, Features::Simd);
                          default: break;
                        }
                        break;
                      case 'u':
                        switch (PeekChar(data, 6)) {
                          case 'g': return LexKeyword(data, "i16x8.ge_u", Opcode::I16X8GeU, Features::Simd);
                          case 'l': return LexKeyword(data, "i16x8.le_u", Opcode::I16X8LeU, Features::Simd);
                          default: break;
                        }
                        break;
                      default: break;
                    }
                    break;
                  case 'g': return LexKeyword(data, "i16x8.neg", Opcode::I16X8Neg, Features::Simd);
                  case 'p': return LexKeyword(data, "i16x8.replace_lane", TokenType::SimdLaneInstr, Opcode::I16X8ReplaceLane, Features::Simd);
                }
                break;
              case 'h':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "i16x8.shl", Opcode::I16X8Shl, Features::Simd);
                  case '_':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "i16x8.shr_s", Opcode::I16X8ShrS, Features::Simd);
                      case 'u': return LexKeyword(data, "i16x8.shr_u", Opcode::I16X8ShrU, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'i':
                switch (PeekChar(data, 10)) {
                  case 'n':
                    switch (PeekChar(data, 23)) {
                      default:
                        switch (PeekChar(data, 22)) {
                          case 's': return LexKeyword(data, "i16x8.widen_low_i8x16_s", Opcode::I16X8WidenLowI8X16S, Features::Simd);
                          case 'u': return LexKeyword(data, "i16x8.widen_low_i8x16_u", Opcode::I16X8WidenLowI8X16U, Features::Simd);
                          default: break;
                        }
                        break;
                      case 's': return LexKeyword(data, "i16x8.widen_high_i8x16_s", Opcode::I16X8WidenHighI8X16S, Features::Simd);
                      case 'u': return LexKeyword(data, "i16x8.widen_high_i8x16_u", Opcode::I16X8WidenHighI8X16U, Features::Simd);
                    }
                    break;
                  case 's': return LexKeyword(data, "i16x8.min_s", Opcode::I16X8MinS, Features::Simd);
                  case 'u': return LexKeyword(data, "i16x8.min_u", Opcode::I16X8MinU, Features::Simd);
                  default: break;
                }
                break;
              case 'l': return LexKeyword(data, "i16x8.all_true", Opcode::I16X8AllTrue, Features::Simd);
              case 'n': return LexKeyword(data, "i16x8.any_true", Opcode::I16X8AnyTrue, Features::Simd);
              case 'p': return LexKeyword(data, "i16x8.splat", Opcode::I16X8Splat, Features::Simd);
              case 'q': return LexKeyword(data, "i16x8.eq", Opcode::I16X8Eq, Features::Simd);
              case 't':
                switch (PeekChar(data, 9)) {
                  case 's':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i16x8.gt_s", Opcode::I16X8GtS, Features::Simd);
                      case 'l': return LexKeyword(data, "i16x8.lt_s", Opcode::I16X8LtS, Features::Simd);
                      default: break;
                    }
                    break;
                  case 'u':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i16x8.gt_u", Opcode::I16X8GtU, Features::Simd);
                      case 'l': return LexKeyword(data, "i16x8.lt_u", Opcode::I16X8LtU, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'u':
                switch (PeekChar(data, 9)) {
                  default:
                    switch (PeekChar(data, 8)) {
                      case 'b': return LexKeyword(data, "i16x8.sub", Opcode::I16X8Sub, Features::Simd);
                      case 'l': return LexKeyword(data, "i16x8.mul", Opcode::I16X8Mul, Features::Simd);
                      default: break;
                    }
                    break;
                  case '_':
                    switch (PeekChar(data, 14)) {
                      case 's': return LexKeyword(data, "i16x8.sub_sat_s", Opcode::I16X8SubSatS, Features::Simd);
                      case 'u': return LexKeyword(data, "i16x8.sub_sat_u", Opcode::I16X8SubSatU, Features::Simd);
                      default: break;
                    }
                    break;
                }
                break;
              case 'v': return LexKeyword(data, "i16x8.avgr_u", Opcode::I16X8AvgrU, Features::Simd);
              case 'x':
                switch (PeekChar(data, 19)) {
                  case 's': return LexKeyword(data, "i16x8.extract_lane_s", TokenType::SimdLaneInstr, Opcode::I16X8ExtractLaneS, Features::Simd);
                  case 'u': return LexKeyword(data, "i16x8.extract_lane_u", TokenType::SimdLaneInstr, Opcode::I16X8ExtractLaneU, Features::Simd);
                  default: break;
                }
                break;
              default: break;
            }
            break;
        }
        break;
    }
    break;
  case '_':
    switch (PeekChar(data, 5)) {
      default: return LexKeyword(data, "br_if", TokenType::VarInstr, Opcode::BrIf);
      case '_':
        switch (PeekChar(data, 9)) {
          default: return LexKeyword(data, "br_on_exn", TokenType::BrOnExnInstr, Opcode::BrOnExn, Features::Exceptions);
          case 'l': return LexKeyword(data, "br_on_null", TokenType::VarInstr, Opcode::BrOnNull, Features::FunctionReferences);
          case 't': return LexKeyword(data, "br_on_cast", TokenType::BrOnCastInstr, Opcode::BrOnCast, Features::GC);
        }
        break;
      case 'b': return LexKeyword(data, "br_table", TokenType::BrTableInstr, Opcode::BrTable);
    }
    break;
  case 'a':
    switch (PeekChar(data, 5)) {
      default: return LexKeyword(data, "start", TokenType::Start);
      case 'd': return LexKeyword(data, "shared", TokenType::Shared);
    }
    break;
  case 'b':
    switch (PeekChar(data, 5)) {
      default: return LexKeyword(data, "table", TokenType::Table);
      case '.':
        switch (PeekChar(data, 9)) {
          default:
            switch (PeekChar(data, 6)) {
              case 'g': return LexKeyword(data, "table.get", TokenType::VarInstr, Opcode::TableGet, Features::ReferenceTypes);
              case 's': return LexKeyword(data, "table.set", TokenType::VarInstr, Opcode::TableSet, Features::ReferenceTypes);
              default: break;
            }
            break;
          case 'e': return LexKeyword(data, "table.size", TokenType::VarInstr, Opcode::TableSize, Features::ReferenceTypes);
          case 'l': return LexKeyword(data, "table.fill", TokenType::VarInstr, Opcode::TableFill, Features::ReferenceTypes);
          case 't': return LexKeyword(data, "table.init", TokenType::TableInitInstr, Opcode::TableInit, Features::BulkMemory);
          case 'w': return LexKeyword(data, "table.grow", TokenType::VarInstr, Opcode::TableGrow, Features::ReferenceTypes);
          case 'y': return LexKeyword(data, "table.copy", TokenType::TableCopyInstr, Opcode::TableCopy, Features::BulkMemory);
        }
        break;
    }
    break;
  case 'c':
    switch (PeekChar(data, 5)) {
      default: return LexKeyword(data, "local", TokenType::Local);
      case '.':
        switch (PeekChar(data, 6)) {
          case 'g': return LexKeyword(data, "local.get", TokenType::VarInstr, Opcode::LocalGet);
          case 's': return LexKeyword(data, "local.set", TokenType::VarInstr, Opcode::LocalSet);
          case 't': return LexKeyword(data, "local.tee", TokenType::VarInstr, Opcode::LocalTee);
          default: break;
        }
        break;
      case 'r': return LexKeyword(data, "declare", TokenType::Declare);
    }
    break;
  case 'd':
    switch (PeekChar(data, 3)) {
      default: return LexKeyword(data, "end", TokenType::End, Opcode::End);
      case 'u': return LexKeyword(data, "module", TokenType::Module);
    }
    break;
  case 'e':
    switch (PeekChar(data, 1)) {
      case 'e': return LexKeyword(data, "tee_local", TokenType::VarInstr, Opcode::LocalTee);
      case 'h': return LexKeyword(data, "then", TokenType::Then);
      case 'i': return LexKeyword(data, "field", TokenType::Field);
      case 'l':
        switch (PeekChar(data, 4)) {
          default: return LexKeyword(data, "elem", TokenType::Elem);
          case '.': return LexKeyword(data, "elem.drop", TokenType::VarInstr, Opcode::ElemDrop, Features::BulkMemory);
        }
        break;
      case 't': return LexKeyword(data, "item", TokenType::Item);
      case 'v': return LexKeyword(data, "event", TokenType::Event);
      default: break;
    }
    break;
  case 'f':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 1)) {
          case 'e': return LexKeyword(data, "ref", TokenType::Ref);
          case 'n': return LexKeyword(data, "inf", TokenType::Float, LiteralKind::Infinity);
          default: break;
        }
        break;
      case '.':
        switch (PeekChar(data, 4)) {
          case 'a': return LexKeyword(data, "ref.as_non_null", Opcode::RefAsNonNull, Features::FunctionReferences);
          case 'c': return LexKeyword(data, "ref.cast", TokenType::HeapType2Instr, Opcode::RefCast, Features::GC);
          case 'e':
            switch (PeekChar(data, 6)) {
              default: return LexKeyword(data, "ref.eq", Opcode::RefEq, Features::GC);
              case 't': return LexKeyword(data, "ref.extern", TokenType::RefExtern);
            }
            break;
          case 'f': return LexKeyword(data, "ref.func", TokenType::RefFuncInstr, Opcode::RefFunc, Features::ReferenceTypes);
          case 'i': return LexKeyword(data, "ref.is_null", Opcode::RefIsNull, Features::ReferenceTypes);
          case 'n': return LexKeyword(data, "ref.null", TokenType::RefNullInstr, Opcode::RefNull, Features::ReferenceTypes);
          case 't': return LexKeyword(data, "ref.test", TokenType::HeapType2Instr, Opcode::RefTest, Features::GC);
          default: break;
        }
        break;
      case 's':
        switch (PeekChar(data, 6)) {
          default: return LexKeyword(data, "offset", TokenType::Offset);
          case '=': return LexNameEqNum(data, "offset=", TokenType::OffsetEqNat);
        }
        break;
    }
    break;
  case 'g': return LexKeyword(data, "register", TokenType::Register);
  case 'i': return LexNameEqNum(data, "align=", TokenType::AlignEqNat);
  case 'l':
    switch (PeekChar(data, 4)) {
      default:
        switch (PeekChar(data, 1)) {
          case 'a': return LexKeyword(data, "call", TokenType::VarInstr, Opcode::Call);
          case 'u': return LexKeyword(data, "null", TokenType::Null);
          default: break;
        }
        break;
      case '_':
        switch (PeekChar(data, 8)) {
          default: return LexKeyword(data, "call_ref", Opcode::CallRef, Features::FunctionReferences);
          case 'i': return LexKeyword(data, "call_indirect", TokenType::CallIndirectInstr, Opcode::CallIndirect);
        }
        break;
      case 'c': return LexKeyword(data, "select", TokenType::SelectInstr, Opcode::Select);
    }
    break;
  case 'm':
    switch (PeekChar(data, 6)) {
      default: return LexKeyword(data, "memory", TokenType::Memory);
      case '.':
        switch (PeekChar(data, 10)) {
          case 'e': return LexKeyword(data, "memory.size", Opcode::MemorySize);
          case 'l': return LexKeyword(data, "memory.fill", Opcode::MemoryFill, Features::BulkMemory);
          case 'm':
            switch (PeekChar(data, 19)) {
              case '2': return LexKeyword(data, "memory.atomic.wait32", TokenType::MemoryInstr, Opcode::MemoryAtomicWait32, Features::Threads);
              case '4': return LexKeyword(data, "memory.atomic.wait64", TokenType::MemoryInstr, Opcode::MemoryAtomicWait64, Features::Threads);
              case 'y': return LexKeyword(data, "memory.atomic.notify", TokenType::MemoryInstr, Opcode::MemoryAtomicNotify, Features::Threads);
              default: break;
            }
            break;
          case 't': return LexKeyword(data, "memory.init", TokenType::MemoryInitInstr, Opcode::MemoryInit, Features::BulkMemory);
          case 'w': return LexKeyword(data, "memory.grow", Opcode::MemoryGrow);
          case 'y': return LexKeyword(data, "memory.copy", TokenType::MemoryCopyInstr, Opcode::MemoryCopy, Features::BulkMemory);
          default: break;
        }
        break;
    }
    break;
  case 'n':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 1)) {
          case 'a': return LexKeyword(data, "nan", TokenType::Float, LiteralKind::Nan);
          case 'x': return LexKeyword(data, "exn", TokenType::HeapKind, HeapKind::Exn);
          default: break;
        }
        break;
      case ':':
        switch (PeekChar(data, 6)) {
          default: return LexNan(data);
          case 'i': return LexKeyword(data, "nan:arithmetic", TokenType::NanArithmetic);
          case 'n': return LexKeyword(data, "nan:canonical", TokenType::NanCanonical);
        }
        break;
      case 'a': return LexKeyword(data, "binary", TokenType::Binary);
      case 'c':
        switch (PeekChar(data, 4)) {
          default: return LexKeyword(data, "func", TokenType::Func, HeapKind::Func);
          case '.': return LexKeyword(data, "func.bind", TokenType::FuncBindInstr, Opcode::FuncBind, Features::FunctionReferences);
          case 'r': return LexKeyword(data, "funcref", ReferenceKind::Funcref);
        }
        break;
      case 'r': return LexKeyword(data, "exnref", ReferenceKind::Exnref);
    }
    break;
  case 'o':
    switch (PeekChar(data, 4)) {
      default:
        switch (PeekChar(data, 1)) {
          case 'o': return LexKeyword(data, "loop", TokenType::BlockInstr, Opcode::Loop);
          case 'r': return LexKeyword(data, "drop", Opcode::Drop);
          default: break;
        }
        break;
      case '_': return LexKeyword(data, "grow_memory", Opcode::MemoryGrow);
      case 'a':
        switch (PeekChar(data, 6)) {
          default: return LexKeyword(data, "global", TokenType::Global);
          case '.':
            switch (PeekChar(data, 7)) {
              case 'g': return LexKeyword(data, "global.get", TokenType::VarInstr, Opcode::GlobalGet);
              case 's': return LexKeyword(data, "global.set", TokenType::VarInstr, Opcode::GlobalSet);
              default: break;
            }
            break;
        }
        break;
      case 'e': return LexKeyword(data, "quote", TokenType::Quote);
      case 'k': return LexKeyword(data, "block", TokenType::BlockInstr, Opcode::Block);
    }
    break;
  case 'p':
    switch (PeekChar(data, 1)) {
      case 'm': return LexKeyword(data, "import", TokenType::Import);
      case 'o': return LexKeyword(data, "nop", Opcode::Nop);
      case 'x': return LexKeyword(data, "export", TokenType::Export);
      case 'y': return LexKeyword(data, "type", TokenType::Type);
      default: break;
    }
    break;
  case 'r':
    switch (PeekChar(data, 4)) {
      case 'a': return LexKeyword(data, "unreachable", Opcode::Unreachable);
      case 'c':
        switch (PeekChar(data, 6)) {
          default: return LexKeyword(data, "struct", TokenType::Struct);
          case '.':
            switch (PeekChar(data, 7)) {
              case 'g':
                switch (PeekChar(data, 10)) {
                  default: return LexKeyword(data, "struct.get", TokenType::StructFieldInstr, Opcode::StructGet, Features::GC);
                  case '_':
                    switch (PeekChar(data, 11)) {
                      case 's': return LexKeyword(data, "struct.get_s", TokenType::StructFieldInstr, Opcode::StructGetS, Features::GC);
                      case 'u': return LexKeyword(data, "struct.get_u", TokenType::StructFieldInstr, Opcode::StructGetU, Features::GC);
                      default: break;
                    }
                    break;
                }
                break;
              case 'n':
                switch (PeekChar(data, 19)) {
                  default: return LexKeyword(data, "struct.new_with_rtt", TokenType::VarInstr, Opcode::StructNewWithRtt, Features::GC);
                  case 'w': return LexKeyword(data, "struct.new_default_with_rtt", TokenType::VarInstr, Opcode::StructNewDefaultWithRtt, Features::GC);
                }
                break;
              case 's': return LexKeyword(data, "struct.set", TokenType::StructFieldInstr, Opcode::StructSet, Features::GC);
              default: break;
            }
            break;
        }
        break;
      case 'e': return LexKeyword(data, "current_memory", Opcode::MemorySize);
      case 'f': return LexKeyword(data, "eqref", ReferenceKind::Eqref);
      case 'm': return LexKeyword(data, "param", TokenType::Param);
      case 'w': return LexKeyword(data, "throw", TokenType::VarInstr, Opcode::Throw, Features::Exceptions);
      case 'y':
        switch (PeekChar(data, 5)) {
          default: return LexKeyword(data, "array", TokenType::Array);
          case '.':
            switch (PeekChar(data, 6)) {
              case 'g':
                switch (PeekChar(data, 9)) {
                  default: return LexKeyword(data, "array.get", TokenType::VarInstr, Opcode::ArrayGet, Features::GC);
                  case '_':
                    switch (PeekChar(data, 10)) {
                      case 's': return LexKeyword(data, "array.get_s", TokenType::VarInstr, Opcode::ArrayGetS, Features::GC);
                      case 'u': return LexKeyword(data, "array.get_u", TokenType::VarInstr, Opcode::ArrayGetU, Features::GC);
                      default: break;
                    }
                    break;
                }
                break;
              case 'l': return LexKeyword(data, "array.len", TokenType::VarInstr, Opcode::ArrayLen, Features::GC);
              case 'n':
                switch (PeekChar(data, 18)) {
                  default: return LexKeyword(data, "array.new_with_rtt", TokenType::VarInstr, Opcode::ArrayNewWithRtt, Features::GC);
                  case 'w': return LexKeyword(data, "array.new_default_with_rtt", TokenType::VarInstr, Opcode::ArrayNewDefaultWithRtt, Features::GC);
                }
                break;
              case 's': return LexKeyword(data, "array.set", TokenType::VarInstr, Opcode::ArraySet, Features::GC);
              default: break;
            }
            break;
        }
        break;
      default: break;
    }
    break;
  case 's':
    switch (PeekChar(data, 4)) {
      default: return LexKeyword(data, "else", TokenType::Else, Opcode::Else);
      case 'l': return LexKeyword(data, "result", TokenType::Result);
      case 'r':
        switch (PeekChar(data, 11)) {
          default: return LexKeyword(data, "assert_trap", TokenType::AssertTrap);
          case 'l': return LexKeyword(data, "assert_invalid", TokenType::AssertInvalid);
          case 'n': return LexKeyword(data, "assert_unlinkable", TokenType::AssertUnlinkable);
          case 'o': return LexKeyword(data, "assert_malformed", TokenType::AssertMalformed);
          case 'r': return LexKeyword(data, "assert_return", TokenType::AssertReturn);
          case 'u': return LexKeyword(data, "assert_exhaustion", TokenType::AssertExhaustion);
        }
        break;
    }
    break;
  case 't':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 0)) {
          case 'g': return LexKeyword(data, "get", TokenType::Get);
          case 'l': return LexKeyword(data, "let", TokenType::LetInstr, Opcode::Let, Features::FunctionReferences);
          case 'm': return LexKeyword(data, "mut", TokenType::Mut);
          case 'r': return LexKeyword(data, "rtt", TokenType::Rtt);
          default: break;
        }
        break;
      case '.':
        switch (PeekChar(data, 7)) {
          default: return LexKeyword(data, "rtt.sub", TokenType::RttSubInstr, Opcode::RttSub, Features::GC);
          case 'o': return LexKeyword(data, "rtt.canon", TokenType::HeapTypeInstr, Opcode::RttCanon, Features::GC);
        }
        break;
      case '_':
        switch (PeekChar(data, 9)) {
          default:
            switch (PeekChar(data, 0)) {
              case 'g': return LexKeyword(data, "get_local", TokenType::VarInstr, Opcode::LocalGet);
              case 's': return LexKeyword(data, "set_local", TokenType::VarInstr, Opcode::LocalSet);
              default: break;
            }
            break;
          case 'l':
            switch (PeekChar(data, 0)) {
              case 'g': return LexKeyword(data, "get_global", TokenType::VarInstr, Opcode::GlobalGet);
              case 's': return LexKeyword(data, "set_global", TokenType::VarInstr, Opcode::GlobalSet);
              default: break;
            }
            break;
        }
        break;
      case 'a':
        switch (PeekChar(data, 4)) {
          default: return LexKeyword(data, "data", TokenType::Data);
          case '.': return LexKeyword(data, "data.drop", TokenType::VarInstr, Opcode::DataDrop, Features::BulkMemory);
        }
        break;
      case 'c': return LexKeyword(data, "catch", TokenType::Catch, Opcode::Catch);
      case 'e':
        switch (PeekChar(data, 6)) {
          default: return LexKeyword(data, "extern", TokenType::HeapKind, HeapKind::Extern);
          case 'r': return LexKeyword(data, "externref", ReferenceKind::Externref);
        }
        break;
      case 'h': return LexKeyword(data, "rethrow", Opcode::Rethrow, Features::Exceptions);
      case 'u':
        switch (PeekChar(data, 6)) {
          default: return LexKeyword(data, "return", Opcode::Return);
          case '_':
            switch (PeekChar(data, 11)) {
              default: return LexKeyword(data, "return_call", TokenType::VarInstr, Opcode::ReturnCall, Features::TailCall);
              case '_':
                switch (PeekChar(data, 15)) {
                  default: return LexKeyword(data, "return_call_ref", Opcode::ReturnCallRef, Features::FunctionReferences);
                  case 'i': return LexKeyword(data, "return_call_indirect", TokenType::CallIndirectInstr, Opcode::ReturnCallIndirect, Features::TailCall);
                }
                break;
            }
            break;
        }
        break;
    }
    break;
  case 'v': return LexKeyword(data, "invoke", TokenType::Invoke);
  case 'x':
    switch (PeekChar(data, 5)) {
      default: return LexKeyword(data, "i8x16", TokenType::I8X16);
      case '.':
        switch (PeekChar(data, 8)) {
          default:
            switch (PeekChar(data, 7)) {
              case 'e': return LexKeyword(data, "i8x16.ne", Opcode::I8X16Ne, Features::Simd);
              case 'q': return LexKeyword(data, "i8x16.eq", Opcode::I8X16Eq, Features::Simd);
              default: break;
            }
            break;
          case '_':
            switch (PeekChar(data, 9)) {
              case 's':
                switch (PeekChar(data, 7)) {
                  case 'e':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i8x16.ge_s", Opcode::I8X16GeS, Features::Simd);
                      case 'l': return LexKeyword(data, "i8x16.le_s", Opcode::I8X16LeS, Features::Simd);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i8x16.gt_s", Opcode::I8X16GtS, Features::Simd);
                      case 'l': return LexKeyword(data, "i8x16.lt_s", Opcode::I8X16LtS, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              case 'u':
                switch (PeekChar(data, 7)) {
                  case 'e':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i8x16.ge_u", Opcode::I8X16GeU, Features::Simd);
                      case 'l': return LexKeyword(data, "i8x16.le_u", Opcode::I8X16LeU, Features::Simd);
                      default: break;
                    }
                    break;
                  case 't':
                    switch (PeekChar(data, 6)) {
                      case 'g': return LexKeyword(data, "i8x16.gt_u", Opcode::I8X16GtU, Features::Simd);
                      case 'l': return LexKeyword(data, "i8x16.lt_u", Opcode::I8X16LtU, Features::Simd);
                      default: break;
                    }
                    break;
                  default: break;
                }
                break;
              default: break;
            }
            break;
          case 'b':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "i8x16.sub", Opcode::I8X16Sub, Features::Simd);
              case '_':
                switch (PeekChar(data, 14)) {
                  case 's': return LexKeyword(data, "i8x16.sub_sat_s", Opcode::I8X16SubSatS, Features::Simd);
                  case 'u': return LexKeyword(data, "i8x16.sub_sat_u", Opcode::I8X16SubSatU, Features::Simd);
                  default: break;
                }
                break;
            }
            break;
          case 'd':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "i8x16.add", Opcode::I8X16Add, Features::Simd);
              case '_':
                switch (PeekChar(data, 14)) {
                  case 's': return LexKeyword(data, "i8x16.add_sat_s", Opcode::I8X16AddSatS, Features::Simd);
                  case 'u': return LexKeyword(data, "i8x16.add_sat_u", Opcode::I8X16AddSatU, Features::Simd);
                  default: break;
                }
                break;
            }
            break;
          case 'g':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "i8x16.neg", Opcode::I8X16Neg, Features::Simd);
              case 'r': return LexKeyword(data, "i8x16.avgr_u", Opcode::I8X16AvgrU, Features::Simd);
            }
            break;
          case 'i': return LexKeyword(data, "i8x16.swizzle", Opcode::I8X16Swizzle, Features::Simd);
          case 'l':
            switch (PeekChar(data, 9)) {
              default: return LexKeyword(data, "i8x16.shl", Opcode::I8X16Shl, Features::Simd);
              case '_': return LexKeyword(data, "i8x16.all_true", Opcode::I8X16AllTrue, Features::Simd);
              case 'a': return LexKeyword(data, "i8x16.splat", Opcode::I8X16Splat, Features::Simd);
            }
            break;
          case 'n':
            switch (PeekChar(data, 10)) {
              case 's': return LexKeyword(data, "i8x16.min_s", Opcode::I8X16MinS, Features::Simd);
              case 'u': return LexKeyword(data, "i8x16.min_u", Opcode::I8X16MinU, Features::Simd);
              default: break;
            }
            break;
          case 'p': return LexKeyword(data, "i8x16.replace_lane", TokenType::SimdLaneInstr, Opcode::I8X16ReplaceLane, Features::Simd);
          case 'r':
            switch (PeekChar(data, 10)) {
              case 'o':
                switch (PeekChar(data, 19)) {
                  case 's': return LexKeyword(data, "i8x16.narrow_i16x8_s", Opcode::I8X16NarrowI16X8S, Features::Simd);
                  case 'u': return LexKeyword(data, "i8x16.narrow_i16x8_u", Opcode::I8X16NarrowI16X8U, Features::Simd);
                  default: break;
                }
                break;
              case 's': return LexKeyword(data, "i8x16.shr_s", Opcode::I8X16ShrS, Features::Simd);
              case 'u': return LexKeyword(data, "i8x16.shr_u", Opcode::I8X16ShrU, Features::Simd);
              default: break;
            }
            break;
          case 's': return LexKeyword(data, "i8x16.abs", Opcode::I8X16Abs, Features::Simd);
          case 't':
            switch (PeekChar(data, 19)) {
              case 's': return LexKeyword(data, "i8x16.extract_lane_s", TokenType::SimdLaneInstr, Opcode::I8X16ExtractLaneS, Features::Simd);
              case 'u': return LexKeyword(data, "i8x16.extract_lane_u", TokenType::SimdLaneInstr, Opcode::I8X16ExtractLaneU, Features::Simd);
              default: break;
            }
            break;
          case 'u': return LexKeyword(data, "i8x16.shuffle", TokenType::SimdShuffleInstr, Opcode::I8X16Shuffle, Features::Simd);
          case 'x':
            switch (PeekChar(data, 10)) {
              case 's': return LexKeyword(data, "i8x16.max_s", Opcode::I8X16MaxS, Features::Simd);
              case 'u': return LexKeyword(data, "i8x16.max_u", Opcode::I8X16MaxU, Features::Simd);
              default: break;
            }
            break;
          case 'y': return LexKeyword(data, "i8x16.any_true", Opcode::I8X16AnyTrue, Features::Simd);
        }
        break;
    }
    break;
  case 'y':
    switch (PeekChar(data, 3)) {
      default:
        switch (PeekChar(data, 1)) {
          case 'n': return LexKeyword(data, "any", TokenType::HeapKind, HeapKind::Any);
          case 'r': return LexKeyword(data, "try", TokenType::BlockInstr, Opcode::Try, Features::Exceptions);
          default: break;
        }
        break;
      case 'f': return LexKeyword(data, "anyfunc", ReferenceKind::Funcref);
      case 'r': return LexKeyword(data, "anyref", ReferenceKind::Anyref);
    }
    break;
}
break;

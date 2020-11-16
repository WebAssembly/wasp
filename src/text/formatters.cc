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

#include "wasp/text/formatters.h"

#include <cassert>

#include "wasp/base/formatter_macros.h"
#include "wasp/base/formatters.h"
#include "wasp/base/macros.h"

namespace wasp::text {

WASP_TEXT_STRUCTS(WASP_FORMATTER_VARGS)

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::TokenType& self) {
  string_view result;
  switch (self) {
#define WASP_V(Name)                  \
  case ::wasp::text::TokenType::Name: \
    result = #Name;                   \
    break;
#include "wasp/text/token_type.inc"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Sign& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::Sign::None:
      result = "None";
      break;
    case ::wasp::text::Sign::Plus:
      result = "Plus";
      break;
    case ::wasp::text::Sign::Minus:
      result = "Minus";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::LiteralKind& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::LiteralKind::Normal:
      result = "Normal";
      break;
    case ::wasp::text::LiteralKind::Nan:
      result = "Nan";
      break;
    case ::wasp::text::LiteralKind::NanPayload:
      result = "NanPayload";
      break;
    case ::wasp::text::LiteralKind::Infinity:
      result = "Infinity";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Base& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::Base::Decimal:
      result = "Decimal";
      break;
    case ::wasp::text::Base::Hex:
      result = "Hex";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::HasUnderscores& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::HasUnderscores::No:
      result = "No";
      break;
    case ::wasp::text::HasUnderscores::Yes:
      result = "Yes";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::SimdShape& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::SimdShape::I8X16:
      result = "i8x16";
      break;
    case ::wasp::text::SimdShape::I16X8:
      result = "i16x8";
      break;
    case ::wasp::text::SimdShape::I32X4:
      result = "i32x4";
      break;
    case ::wasp::text::SimdShape::I64X2:
      result = "i64x2";
      break;
    case ::wasp::text::SimdShape::F32X4:
      result = "f32x4";
      break;
    case ::wasp::text::SimdShape::F64X2:
      result = "f64x2";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Var& self) {
  if (self.is_index()) {
    os << self.index();
  } else {
    os << self.name();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::ModuleItem& self) {
  return os << self.desc;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Const& self) {
  return os << self.value;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::ScriptModuleKind& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::ScriptModuleKind::Binary:
      result = "binary";
      break;
    case ::wasp::text::ScriptModuleKind::Text:
      result = "text";
      break;
    case ::wasp::text::ScriptModuleKind::Quote:
      result = "quote";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::AssertionKind& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::AssertionKind::Malformed:
      result = "malformed";
      break;
    case ::wasp::text::AssertionKind::Invalid:
      result = "invalid";
      break;
    case ::wasp::text::AssertionKind::Unlinkable:
      result = "unlinkable";
      break;
    case ::wasp::text::AssertionKind::ActionTrap:
      result = "action_trap";
      break;
    case ::wasp::text::AssertionKind::Return:
      result = "return";
      break;
    case ::wasp::text::AssertionKind::ModuleTrap:
      result = "module_trap";
      break;
    case ::wasp::text::AssertionKind::Exhaustion:
      result = "exhaustion";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::NanKind& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::NanKind::Canonical:
      result = "canonical";
      break;
    case ::wasp::text::NanKind::Arithmetic:
      result = "arithmetic";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::NumericDataType& self) {
  string_view result;
  switch (self) {
    case ::wasp::text::NumericDataType::I8:
      result = "i8";
      break;
    case ::wasp::text::NumericDataType::I16:
      result = "i16";
      break;
    case ::wasp::text::NumericDataType::I32:
      result = "i32";
      break;
    case ::wasp::text::NumericDataType::I64:
      result = "i64";
      break;
    case ::wasp::text::NumericDataType::F32:
      result = "f32";
      break;
    case ::wasp::text::NumericDataType::F64:
      result = "f64";
      break;
    case ::wasp::text::NumericDataType::V128:
      result = "v128";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return os << result;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Command& self) {
  return os << self.contents;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::HeapType& self) {
  if (self.is_heap_kind()) {
    os << self.heap_kind();
  } else {
    assert(self.is_var());
    os << self.var();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::RefType& self) {
  os << "(ref ";
  if (self.null == ::wasp::Null::Yes) {
    os << "null ";
  }
  return os << self.heap_type << ")";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::ReferenceType& self) {
  if (self.is_reference_kind()) {
    os << self.reference_kind();
  } else {
    assert(self.is_ref());
    os << self.ref();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const ::wasp::text::Rtt& self) {
  return os << "(rtt " << self.depth << " " << self.type << ")";
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::ValueType& self) {
  if (self.is_numeric_type()) {
    os << self.numeric_type();
  } else if (self.is_reference_type()) {
    os << self.reference_type();
  } else {
    assert(self.is_rtt());
    os << self.rtt();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::StorageType& self) {
  if (self.is_value_type()) {
    os << self.value_type();
  } else {
    assert(self.is_packed_type());
    os << self.packed_type();
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::TableType& self) {
  os << self.limits << " " << self.elemtype;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ::wasp::text::GlobalType& self) {
  os << self.mut << " " << self.valtype;
  return os;
}

}  // namespace wasp::text

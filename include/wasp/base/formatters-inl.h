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

#include "wasp/base/macros.h"

#include <type_traits>

namespace fmt {

template <typename T, std::size_t SIZE, typename Allocator>
string_view to_string_view(const basic_memory_buffer<T, SIZE, Allocator>& buf) {
  return string_view{buf.data(), buf.size()};
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::At<T>>::format(
    const ::wasp::At<T>& self,
    Ctx& ctx) {
  return formatter<T>::format(*self, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::SpanU8>::format(
    const ::wasp::SpanU8& self,
    Ctx& ctx) {
  memory_buffer buf;
  format_to(buf, "\"");
  for (auto x : self) {
    format_to(buf, "\\{:02x}", x);
  }
  format_to(buf, "\"");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::span<const T>>::format(
    ::wasp::span<const T> self,
    Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  format_to(buf, "[");
  for (const auto& x : self) {
    format_to(buf, "{}{}", space, x);
    space = " ";
  }
  format_to(buf, "]");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T, size_t N>
template <typename Ctx>
typename Ctx::iterator formatter<std::array<T, N>>::format(
    const std::array<T, N>& self,
    Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  format_to(buf, "[");
  for (const auto& x : self) {
    format_to(buf, "{}{}", space, x);
    space = " ";
  }
  format_to(buf, "]");
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<std::vector<T>>::format(
    const std::vector<T>& self,
    Ctx& ctx) {
  return formatter<::wasp::span<const T>>::format(::wasp::span<const T>{self},
                                                  ctx);
}

template <typename... Ts>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::variant<Ts...>>::format(
    const ::wasp::variant<Ts...>& self,
    Ctx& ctx) {
  memory_buffer buf;
  std::visit(
      [&](auto&& arg) {
        using Type = std::remove_cv_t<std::remove_reference_t<decltype(arg)>>;
        if constexpr (std::is_same_v<Type, ::wasp::monostate>) {
          format_to(buf, "empty");
        } else {
          format_to(buf, "{} {}", ::wasp::VariantName<Type>().GetName(), arg);
        }
      },
      self);
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<::wasp::optional<T>>::format(
    const ::wasp::optional<T>& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.has_value()) {
    memory_buffer buf;
    format_to(buf, "{}", *self);
    return formatter<string_view>::format(to_string_view(buf), ctx);
  } else {
    return formatter<string_view>::format("none", ctx);
  }
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::v128>::format(const ::wasp::v128& self,
                                                       Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  for (const auto& x : self.as<::wasp::u32x4>()) {
    format_to(buf, "{}{:#x}", space, x);
    space = " ";
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Opcode>::format(
    const ::wasp::Opcode& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(prefix, val, Name, str, ...) \
  case ::wasp::Opcode::Name:                \
    result = str;                           \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#define WASP_PREFIX_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/opcode.def"
#undef WASP_V
#undef WASP_FEATURE_V
#undef WASP_PREFIX_V
    default: {
      // Special case for opcodes with unknown ids.
      memory_buffer buf;
      format_to(buf, "<unknown:{}>", static_cast<::wasp::u32>(self));
      return formatter<string_view>::format(to_string_view(buf), ctx);
    }
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::ValueType>::format(
    const ::wasp::ValueType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ValueType::Name:     \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/value_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::ReferenceType>::format(
    const ::wasp::ReferenceType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ReferenceType::Name: \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/reference_type.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::ExternalKind>::format(
    const ::wasp::ExternalKind& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str, ...) \
  case ::wasp::ExternalKind::Name:  \
    result = str;                   \
    break;
#define WASP_FEATURE_V(...) WASP_V(__VA_ARGS__)
#include "wasp/base/def/external_kind.def"
#undef WASP_V
#undef WASP_FEATURE_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::EventAttribute>::format(
    const ::wasp::EventAttribute& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)       \
  case ::wasp::EventAttribute::Name: \
    result = str;                    \
    break;
#include "wasp/base/def/event_attribute.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Mutability>::format(
    const ::wasp::Mutability& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
#define WASP_V(val, Name, str)   \
  case ::wasp::Mutability::Name: \
    result = str;                \
    break;
#include "wasp/base/def/mutability.def"
#undef WASP_V
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::SegmentType>::format(
    const ::wasp::SegmentType& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::SegmentType::Active:
      result = "active";
      break;
    case ::wasp::SegmentType::Passive:
      result = "passive";
      break;
    case ::wasp::SegmentType::Declared:
      result = "declared";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Features>::format(
    const ::wasp::Features& self,
    Ctx& ctx) {
  memory_buffer buf;
  string_view separator;
#define WASP_V(enum_, variable, flag, default_) \
  if (self.variable##_enabled()) {              \
    format_to(buf, "{}" #variable, separator);  \
    separator = "|";                            \
  }
#include "wasp/base/features.def"
#undef WASP_V

  if (separator.size() == 0) {
    return formatter<string_view>::format("none", ctx);
  } else {
    return formatter<string_view>::format(to_string_view(buf), ctx);
  }
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Shared>::format(
    const ::wasp::Shared& self,
    Ctx& ctx) {
  string_view result;
  switch (self) {
    case ::wasp::Shared::No:
      result = "unshared";
      break;
    case ::wasp::Shared::Yes:
      result = "shared";
      break;
    default:
      WASP_UNREACHABLE();
  }
  return formatter<string_view>::format(result, ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::Limits>::format(
    const ::wasp::Limits& self,
    Ctx& ctx) {
  memory_buffer buf;
  if (self.max) {
    if (self.shared == ::wasp::Shared::Yes) {
      format_to(buf, "{{min {}, max {}, {}}}", self.min, *self.max,
                self.shared);
    } else {
      format_to(buf, "{{min {}, max {}}}", self.min, *self.max);
    }
  } else {
    format_to(buf, "{{min {}}}", self.min);
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::monostate>::format(
    const ::wasp::monostate& self,
    Ctx& ctx) {
  return formatter<string_view>::format("", ctx);
}

}  // namespace fmt

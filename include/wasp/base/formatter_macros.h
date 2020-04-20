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

#ifndef WASP_BASE_FORMATTER_MACROS_H_
#define WASP_BASE_FORMATTER_MACROS_H_

#define WASP_DECLARE_FORMATTER(Name, ...)                     \
  template <>                                                 \
  struct formatter<::wasp::Name> : formatter<string_view> {   \
    template <typename Ctx>                                   \
    typename Ctx::iterator format(const ::wasp::Name&, Ctx&); \
  };

#define WASP_FORMATTER_VARGS(Name, Count, ...) \
  WASP_FORMATTER_##Count(Name, __VA_ARGS__)

// Use ... here so WASP_FORMATTER_VARGS works.
#define WASP_FORMATTER_0(Name, ...)                                  \
  template <typename Ctx>                                            \
  typename Ctx::iterator formatter<::wasp::Name>::format(            \
      const ::wasp::Name& self, Ctx& ctx) {                          \
    memory_buffer buf;                                               \
    format_to(buf, "{{}}");                                          \
    return formatter<string_view>::format(to_string_view(buf), ctx); \
  }

#define WASP_FORMATTER_1(Name, f1)                                   \
  template <typename Ctx>                                            \
  typename Ctx::iterator formatter<::wasp::Name>::format(            \
      const ::wasp::Name& self, Ctx& ctx) {                          \
    memory_buffer buf;                                               \
    format_to(buf, "{{" #f1 " {}}}", self.f1);                       \
    return formatter<string_view>::format(to_string_view(buf), ctx); \
  }

#define WASP_FORMATTER_2(Name, f1, f2)                               \
  template <typename Ctx>                                            \
  typename Ctx::iterator formatter<::wasp::Name>::format(            \
      const ::wasp::Name& self, Ctx& ctx) {                          \
    memory_buffer buf;                                               \
    format_to(buf, "{{" #f1 " {}, " #f2 " {}}}", self.f1, self.f2);  \
    return formatter<string_view>::format(to_string_view(buf), ctx); \
  }

#define WASP_FORMATTER_3(Name, f1, f2, f3)                                     \
  template <typename Ctx>                                                      \
  typename Ctx::iterator formatter<::wasp::Name>::format(                      \
      const ::wasp::Name& self, Ctx& ctx) {                                    \
    memory_buffer buf;                                                         \
    format_to(buf, "{{" #f1 " {}, " #f2 " {}, " #f3 " {}}}", self.f1, self.f2, \
              self.f3);                                                        \
    return formatter<string_view>::format(to_string_view(buf), ctx);           \
  }

#define WASP_FORMATTER_4(Name, f1, f2, f3, f4)                           \
  template <typename Ctx>                                                \
  typename Ctx::iterator formatter<::wasp::Name>::format(                \
      const ::wasp::Name& self, Ctx& ctx) {                              \
    memory_buffer buf;                                                   \
    format_to(buf, "{{" #f1 " {}, " #f2 " {}, " #f3 " {}, " #f4 " {}}}", \
              self.f1, self.f2, self.f3, self.f4);                       \
    return formatter<string_view>::format(to_string_view(buf), ctx);     \
  }

#define WASP_FORMATTER_5(Name, f1, f2, f3, f4, f5)                             \
  template <typename Ctx>                                                      \
  typename Ctx::iterator formatter<::wasp::Name>::format(                      \
      const ::wasp::Name& self, Ctx& ctx) {                                    \
    memory_buffer buf;                                                         \
    format_to(                                                                 \
        buf, "{{" #f1 " {}, " #f2 " {}, " #f3 " {}, " #f4 " {}, " #f5 " {}}}", \
        self.f1, self.f2, self.f3, self.f4, self.f5);                          \
    return formatter<string_view>::format(to_string_view(buf), ctx);           \
  }

#endif // WASP_BASE_FORMATTER_MACROS_H_

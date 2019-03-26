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

namespace fmt {

template <typename T, std::size_t SIZE, typename Allocator>
string_view to_string_view(const basic_memory_buffer<T, SIZE, Allocator>& buf) {
  return string_view{buf.data(), buf.size()};
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

template <typename T>
template <typename Ctx>
typename Ctx::iterator formatter<std::vector<T>>::format(
    const std::vector<T>& self,
    Ctx& ctx) {
  return formatter<::wasp::span<const T>>::format(::wasp::span<const T>{self},
                                                  ctx);
}

template <typename Ctx>
typename Ctx::iterator formatter<::wasp::v128>::format(::wasp::v128 self,
                                                       Ctx& ctx) {
  memory_buffer buf;
  string_view space = "";
  for (const auto& x : self.as<::wasp::u32x4>()) {
    format_to(buf, "{}{:#x}", space, x);
    space = " ";
  }
  return formatter<string_view>::format(to_string_view(buf), ctx);
}

}  // namespace fmt

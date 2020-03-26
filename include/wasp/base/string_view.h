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

#ifndef WASP_BASE_STRING_VIEW_H_
#define WASP_BASE_STRING_VIEW_H_

#include <string_view>

namespace wasp {

using std::string_view;

using std::string_view;
using std::wstring_view;
using std::u16string_view;
using std::u32string_view;
using std::basic_string_view;

using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator<=;
using std::operator>;
using std::operator>=;

using std::operator<<;

inline string_view operator ""_sv(const char* str, size_t N) {
  return string_view{str, N};
}

// Implement c++20 string_view::starts_with, but as a free function.
template <typename CharT, class Traits = std::char_traits<CharT>>
inline bool starts_with(basic_string_view<CharT, Traits> sv,
                        basic_string_view<CharT, Traits> v) {
  return sv.size() >= v.size() && sv.compare(0, v.size(), v) == 0;
}

template <typename CharT, class Traits = std::char_traits<CharT>>
inline bool starts_with(basic_string_view<CharT, Traits> sv, const char* s) {
  return starts_with(sv, basic_string_view<CharT, Traits>(s));
}

}  // namespace wasp

#endif  // WASP_BASE_STRING_VIEW_H_

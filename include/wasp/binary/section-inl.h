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

inline bool Section::is_known() const {
  return contents.index() == 0;
}

inline bool Section::is_custom() const {
  return contents.index() == 1;
}

inline KnownSection& Section::known() {
  return get<KnownSection>(contents);
}

inline const KnownSection& Section::known() const {
  return get<KnownSection>(contents);
}

inline CustomSection& Section::custom() {
  return get<CustomSection>(contents);
}

inline const CustomSection& Section::custom() const {
  return get<CustomSection>(contents);
}

inline SpanU8 Section::data() const {
  if (is_known()) {
    return known().data;
  } else {
    return custom().data;
  }
}

}  // namespace binary
}  // namespace wasp

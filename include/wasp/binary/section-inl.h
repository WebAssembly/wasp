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

inline At<KnownSection>& Section::known() {
  return get<At<KnownSection>>(contents);
}

inline const At<KnownSection>& Section::known() const {
  return get<At<KnownSection>>(contents);
}

inline At<CustomSection>& Section::custom() {
  return get<At<CustomSection>>(contents);
}

inline const At<CustomSection>& Section::custom() const {
  return get<At<CustomSection>>(contents);
}

inline At<SectionId> Section::id() const {
  return is_known() ? known()->id : MakeAt(SectionId::Custom);
}

inline SpanU8 Section::data() const {
  return is_known() ? known()->data : custom()->data;
}

}  // namespace binary
}  // namespace wasp

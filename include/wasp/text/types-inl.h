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

namespace wasp::text {

template <typename T>
auto NumericData::value(Index index) const -> T {
  assert(index < count());
  T result;
  size_t size = data_type_size();
  // TODO: Handle big endian.
  memcpy(&result, data.data() + index * size, size);
  return result;
}

}  // namespace wasp::text

//
// Copyright 2019 WebAssembly Community Group participants
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

inline void Errors::PushContext(Location loc, string_view desc) {
  HandlePushContext(loc, desc);
}

inline void Errors::PopContext() {
  HandlePopContext();
}

inline void Errors::OnError(Location loc, string_view message) {
  HandleOnError(loc, message);
}

}  // namespace wasp

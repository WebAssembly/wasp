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

#ifndef WASP_TEXT_NUMERIC_H_
#define WASP_TEXT_NUMERIC_H_

#include "wasp/base/at.h"
#include "wasp/text/types.h"

namespace wasp {
namespace text {

template <typename T>
auto StrToNat(LiteralInfo, SpanU8) -> OptAt<T>;

template <typename T>
auto StrToInt(LiteralInfo, SpanU8) -> OptAt<T>;

template <typename T>
auto StrToFloat(LiteralInfo, SpanU8) -> OptAt<T>;

}  // namespace text
}  // namespace wasp

#include "wasp/text/numeric-inl.h"

#endif  // WASP_TEXT_NUMERIC_H_

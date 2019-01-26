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

#ifndef WASP_BINARY_LAZY_MODULE_UTILS_H
#define WASP_BINARY_LAZY_MODULE_UTILS_H

#include <utility>

#include "wasp/base/features.h"
#include "wasp/binary/external_kind.h"
#include "wasp/binary/lazy_module.h"

namespace wasp {
namespace binary {

using IndexNamePair = std::pair<Index, string_view>;

template <typename Errors, typename F>
void ForEachFunctionName(LazyModule<Errors>&, F&&, const Features&, Errors&);

template <typename Errors, typename Iterator>
Iterator CopyFunctionNames(LazyModule<Errors>&,
                           Iterator out,
                           const Features&,
                           Errors&);

template <typename Errors>
Index GetImportCount(LazyModule<Errors>&,
                     ExternalKind,
                     const Features&,
                     Errors&);

}  // namespace binary
}  // namespace wasp

#include "wasp/binary/lazy_module_utils-inl.h"

#endif  // WASP_BINARY_LAZY_MODULE_UTILS_H

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

// The section ids are ordered by their expected order in the binary format.
// So, for example, the DataCount section must come after the Element section
// but before the Code section, if it exists.

WASP_V(0, Custom, "custom")
WASP_V(1, Type, "type")
WASP_V(2, Import, "import")
WASP_V(3, Function, "function")
WASP_V(4, Table, "table")
WASP_V(5, Memory, "memory")
WASP_FEATURE_V(13, Tag, "tag", exceptions)
WASP_V(6, Global, "global")
WASP_V(7, Export, "export")
WASP_V(8, Start, "start")
WASP_V(9, Element, "element")
WASP_FEATURE_V(12, DataCount, "datacount", bulk_memory)
WASP_V(10, Code, "code")
WASP_V(11, Data, "data")

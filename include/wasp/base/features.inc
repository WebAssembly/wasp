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

//     enum                  variable                 flag                       default

WASP_V(BulkMemory,           bulk_memory,             "bulk-memory",             false)
WASP_V(Exceptions,           exceptions,              "exceptions",              false)
WASP_V(FunctionReferences,   function_references,     "function-references",     false)
WASP_V(GC,                   gc,                      "gc",                      false)
WASP_V(Memory64,             memory64,                "memory64",                false)
WASP_V(MultiValue,           multi_value,             "multi-value",             false)
WASP_V(MutableGlobals,       mutable_globals,         "mutable-globals",         true )
WASP_V(NumericValues,        numeric_values,          "numeric-values",          false)
WASP_V(ReferenceTypes,       reference_types,         "reference-types",         false)
WASP_V(SaturatingFloatToInt, saturating_float_to_int, "saturating-float-to-int", false)
WASP_V(SignExtension,        sign_extension,          "sign-extension",          false)
WASP_V(Simd,                 simd,                    "simd",                    true)
WASP_V(TailCall,             tail_call,               "tail-call",               false)
WASP_V(Threads,              threads,                 "threads",                 false)

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

#ifndef WASP_BINARY_READER_H_
#define WASP_BINARY_READER_H_

#include <iterator>
#include <vector>

#include "src/base/types.h"
#include "src/binary/types.h"

namespace wasp {
namespace binary {

/// ---
struct Error {
  explicit Error(SpanU8 pos, std::string&& message)
      : pos{pos}, message{std::move(message)} {}

  SpanU8 pos;
  std::string message;
};

/// ---
class ErrorsNop {
 public:
  void PushContext(SpanU8 pos, string_view desc) {}
  void PopContext() {}
  void OnError(SpanU8 pos, string_view message) {}
};

/// ---
class ErrorsVector {
 public:
  void PushContext(SpanU8 pos, string_view desc);
  void PopContext();
  void OnError(SpanU8 pos, string_view message);
};

/// ---
template <typename Errors>
class ErrorsContextGuard {
 public:
  explicit ErrorsContextGuard(Errors& errors, SpanU8 pos, string_view desc)
      : errors_{errors} {
    errors.PushContext(pos, desc);
  }
  ~ErrorsContextGuard() { PopContext(); }

  void PopContext() {
    if (!popped_context_) {
      errors_.PopContext();
      popped_context_ = true;
    }
  }

 private:
  Errors& errors_;
  bool popped_context_ = false;
};

template <typename Sequence>
class LazySequenceIterator;

/// ---
template <typename T, typename Errors>
class LazySequence {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = LazySequenceIterator<LazySequence>;
  using const_iterator = LazySequenceIterator<LazySequence>;

  explicit LazySequence(SpanU8 data, Errors& errors)
      : data_{data}, errors_{errors} {}

  iterator begin() { return iterator{this, data_}; }
  iterator end() { return iterator{this, SpanU8{}}; }
  const_iterator begin() const { return const_iterator{this, data_}; }
  const_iterator end() const { return const_iterator{this, SpanU8{}}; }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

 private:
  template <typename Sequence>
  friend class LazySequenceIterator;

  SpanU8 data_;
  Errors& errors_;
};

/// ---
template <typename Sequence>
class LazySequenceIterator {
 public:
  using difference_type = typename Sequence::difference_type;
  using value_type = typename Sequence::value_type;
  using pointer = typename Sequence::const_pointer;
  using reference = typename Sequence::const_reference;
  using iterator_category = std::forward_iterator_tag;

  explicit LazySequenceIterator(Sequence* seq, SpanU8 data);

  SpanU8 data() const { return data_; }

  reference operator*() const { return *value_; }
  pointer operator->() const { return &*value_; }

  LazySequenceIterator& operator++();
  LazySequenceIterator operator++(int);

  friend bool operator==(const LazySequenceIterator& lhs,
                         const LazySequenceIterator& rhs) {
    return lhs.data_.begin() == rhs.data_.begin();
  }

  friend bool operator!=(const LazySequenceIterator& lhs,
                         const LazySequenceIterator& rhs) {
    return !(lhs == rhs);
  }

 protected:
  bool empty() const { return data_.empty(); }
  void clear() { data_ = {}; }

  Sequence* sequence_;
  SpanU8 data_;
  optional<value_type> value_;
};

/// ---
template <typename Errors>
class LazyModule {
 public:
  explicit LazyModule(SpanU8, Errors&);

  optional<SpanU8> magic;
  optional<SpanU8> version;
  LazySequence<Section<>, Errors> sections;
};

/// ---
template <typename T, typename Errors>
class LazySection {
 public:
  explicit LazySection(SpanU8, Errors&);
  explicit LazySection(KnownSection<>, Errors&);

  optional<Index> count;
  LazySequence<T, Errors> sequence;
};

/// ---
template <typename Errors>
using LazyTypeSection = LazySection<FunctionType, Errors>;

/// ---
template <typename Errors>
using LazyImportSection = LazySection<Import<>, Errors>;

/// ---
template <typename Errors>
using LazyFunctionSection = LazySection<Function, Errors>;

/// ---
template <typename Errors>
using LazyTableSection = LazySection<Table, Errors>;

/// ---
template <typename Errors>
using LazyMemorySection = LazySection<Memory, Errors>;

/// ---
template <typename Errors>
using LazyGlobalSection = LazySection<Global<>, Errors>;

/// ---
template <typename Errors>
using LazyExportSection = LazySection<Export<>, Errors>;

/// ---
template <typename Errors>
using LazyElementSection = LazySection<ElementSegment<>, Errors>;

/// ---
template <typename Errors>
using LazyCodeSection = LazySection<Code<>, Errors>;

/// ---
template <typename Errors>
using LazyDataSection = LazySection<DataSegment<>, Errors>;

/// ---
template <typename Errors>
struct StartSection {
  explicit StartSection(SpanU8, Errors&);
  explicit StartSection(KnownSection<>, Errors&);

  optional<Start> start();

 private:
  Errors& errors_;
  optional<Start> start_;
};

/// ---
template <typename Errors>
using LazyInstructions = LazySequence<Instruction, Errors>;
////////////////////////////////////////////////////////////////////////////////

template <typename Errors>
LazyModule<Errors> ReadModule(SpanU8 data, Errors& errors) {
  return LazyModule<Errors>{data, errors};
}

template <typename Data, typename Errors>
LazyTypeSection<Errors> ReadTypeSection(Data&& data, Errors& errors) {
  return LazyTypeSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyImportSection<Errors> ReadImportSection(Data&& data, Errors& errors) {
  return LazyImportSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyFunctionSection<Errors> ReadFunctionSection(Data&& data, Errors& errors) {
  return LazyFunctionSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyTableSection<Errors> ReadTableSection(Data&& data, Errors& errors) {
  return LazyTableSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyMemorySection<Errors> ReadMemorySection(Data&& data, Errors& errors) {
  return LazyMemorySection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyGlobalSection<Errors> ReadGlobalSection(Data&& data, Errors& errors) {
  return LazyGlobalSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyExportSection<Errors> ReadExportSection(Data&& data, Errors& errors) {
  return LazyExportSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyElementSection<Errors> ReadElementSection(Data&& data, Errors& errors) {
  return LazyElementSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
StartSection<Errors> ReadStartSection(Data&& data, Errors& errors) {
  return StartSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyCodeSection<Errors> ReadCodeSection(Data&& data, Errors& errors) {
  return LazyCodeSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Data, typename Errors>
LazyDataSection<Errors> ReadDataSection(Data&& data, Errors& errors) {
  return LazyDataSection<Errors>{std::forward<Data>(data), errors};
}

template <typename Errors>
LazyInstructions<Errors> ReadExpr(SpanU8 data, Errors& errors) {
  return LazyInstructions<Errors>{data, errors};
}

}  // namespace binary
}  // namespace wasp

#include "src/binary/reader-inl.h"

#endif  // WASP_BINARY_READER_H_

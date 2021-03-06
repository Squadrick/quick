// Copyright: 2019 Mohit Saini
// Author: Mohit Saini (mohitsaini1196@gmail.com)

#ifndef QUICK_DEBUG_STREAM_HPP_
#define QUICK_DEBUG_STREAM_HPP_

#include <iostream>  // NOLINT
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <string>

#include <quick/type_traits.hpp>

// ToDos()
//  1. Implement DebugStream for std::tuple


namespace quick {

// Not Thread Safe
class DebugStream: public std::ostringstream {
 private:
  using string = std::string;
  using ostringstream = std::ostringstream;

 public:
  DebugStream() {}
  DebugStream(const DebugStream&) = default;
  template<typename T>
  DebugStream(const T& input) {
    (*this) << input;
  }
  template<typename T>
  DebugStream& Consume(const T& input) {
    (*this) << input;
    return *this;
  }
  using BaseStream = std::ostringstream;
  bool is_inline = false;
  uint8_t indentation_space = 2;
  uint32_t depth = 0;

  inline void TabSpace() {
    std::operator<<(static_cast<BaseStream&>(*this),
                    string(depth*indentation_space, ' '));
  }

  inline DebugStream& operator<<(char c) {
    std::operator<<((static_cast<BaseStream&>(*this)), c);
    if (c == '\n') {
      TabSpace();
    }
    return *this;
  }

  template<typename T>
  std::enable_if_t<(std::is_fundamental<T>::value), DebugStream>&
  operator<<(const T& input) {
    BaseStream::operator<<(input);
    return *this;
  }

  DebugStream& operator<<(const char* input) {
    for (auto i = input; *i ; i++) {
      (*this) << (*i);
    }
    return *this;
  }

  DebugStream& operator<<(const std::string& input) {
    for (auto c : input) {
      (*this) << c;
    }
    return *this;
  }

  inline void BranchStart(char c) {
    (*this) << c;
    if (not is_inline) {
      std::operator<<(static_cast<BaseStream&>(*this), "\n");
      depth++;
      TabSpace();
    }
  }

  inline void BranchEnd(char c) {
    if (not is_inline) {
      std::operator<<(static_cast<BaseStream&>(*this), "\n");
      if (depth == 0) {
        throw std::runtime_error("[quick::DebugStream]: Invalid BranchEnd");
      }
      depth--;
      TabSpace();
    }
    (*this) << c;
  }

  template<typename ValueType>
  struct ScopedControlsStruct {
    static_assert(std::is_trivially_destructible<ValueType>::value, "");
    ValueType new_value, original_value;
    ValueType* restore_pointer = nullptr;
    ScopedControlsStruct(ValueType new_value, ValueType* restore_pointer)
          : new_value(new_value),
            original_value(*restore_pointer),
            restore_pointer(restore_pointer) {
      *restore_pointer = new_value;
    }
    ~ScopedControlsStruct() {
      if (restore_pointer != nullptr) {
        *restore_pointer = original_value;
      }
    }
  };

  using ScopedInlineStruct = ScopedControlsStruct<bool>;
  using ScopedIndentationStruct = ScopedControlsStruct<uint8_t>;

  inline ScopedInlineStruct SetInlineForThisScope(bool value) {
    return ScopedInlineStruct(value, &this->is_inline);
  }

  inline ScopedIndentationStruct SetIndentationForThisScope(uint8_t value) {
    return ScopedIndentationStruct(value, &this->indentation_space);
  }

  DebugStream& SetInline(bool value) {
    this->is_inline = value;
    return *this;
  }

  DebugStream& SetIndentation(uint8_t value) {
    this->indentation_space = value;
    return *this;
  }
};

namespace detail {
}


template<typename T>
std::enable_if_t<std::is_enum<T>::value, DebugStream>&
operator<<(DebugStream& ds, const T& input) {
  ds << "ENUM-" << static_cast<int32_t>(input);
  return ds;
}


template<typename T>
std::enable_if_t<(quick::is_specialization<T, std::vector>::value ||
                  quick::is_specialization<T, std::list>::value ||
                  quick::is_specialization<T, std::unordered_set>::value ||
                  quick::is_specialization<T, std::set>::value), DebugStream>&
operator<<(DebugStream& ds, const T& input) {
  if (input.size() == 0) {
    ds << "[]";
  } else {
    ds.BranchStart('[');
    bool is_first_item = true;
    for (const auto& item : input) {
      ds << (is_first_item ? "" : ", ") << item;
      is_first_item = false;
    }
    ds.BranchEnd(']');
  }
  return ds;
}


template<typename T>
std::enable_if_t<(quick::is_specialization<T, std::map>::value ||
                  quick::is_specialization<T, std::unordered_map>::value),
                 DebugStream>&
operator<<(DebugStream& ds, const T& input) {
  if (input.size() == 0) {
    ds << "{}";
  } else {
    ds.BranchStart('{');
    bool is_first_item = true;
    for (const auto& item : input) {
      if (not is_first_item) {
        ds << ",";
        if (not ds.is_inline) {
          ds << "\n";
        }
      }
      {
        // ds.SetInlineForThisScope(true);
        ds.is_inline = true;
        ds << item.first;
        ds.is_inline = false;
      }
      ds << ": ";
      ds << item.second;
      is_first_item = false;
    }
    ds.BranchEnd('}');
  }
  return ds;
}

template<typename T1, typename T2>
DebugStream& operator<<(DebugStream& ds, const std::pair<T1, T2>& input) {
  ds.BranchStart('(');
  ds << input.first << ", " << input.second;
  ds.BranchEnd(')');
  return ds;
}

template<typename T>
std::enable_if_t<
  std::is_same<void,
               decltype(
                 std::declval<const T&>().DebugStream(
                   std::declval<quick::DebugStream&>()))>::value,
  DebugStream>&
operator<<(DebugStream& ds, const T& input) {
  ds.BranchStart('{');
  input.DebugStream(ds);
  ds.BranchEnd('}');
  return ds;
}

}  // namespace quick

namespace qk = quick;

#endif  // QUICK_DEBUG_STREAM_HPP_

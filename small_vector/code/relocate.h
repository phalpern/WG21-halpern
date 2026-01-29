/* relocate.h                                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_RELOCATE
#define INCLUDED_RELOCATE

#include <memory>
#include <cstring>

namespace xstd
{

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// `is_trivially_relocatable, `is_replaceable`, `is_nothrow_relocatable`,
// `trivially_relocate()`, and `relocate()`.
///////////////////////////////////////////////////////////////////////////////

// Rough default implementations of `is_trivially_relocatable` and
// `is_replaceable`.  Since we don't have keyword support yet, these traits
// would need to be specialized for types that aren't trivially copy
// constructible or trivially assignable.
template <class T>
struct is_trivially_relocatable :
    conjunction<is_trivially_copy_constructible<T>,
                is_trivially_destructible<T>> { };

template <class T>
struct is_replaceable : is_trivially_move_assignable<T> { };

template <class T>
constexpr inline bool is_trivially_relocatable_v =
  is_trivially_relocatable<T>::value;

template <class T>
constexpr inline bool is_replaceable_v = is_replaceable<T>::value;

template <class T>
struct is_nothrow_relocatable :
    disjunction<is_trivially_relocatable<T>,
                is_nothrow_move_constructible<T>> { };

template <class T>
constexpr inline bool is_nothrow_relocatable_v =
  is_nothrow_relocatable<T>::value;

// Almost-correct implementation of magic `trivially_relocate` function, but
// relies on UB.
template <class T>
T* trivially_relocate(T* begin, T* end, T* new_location)
{
  static_assert(is_trivially_relocatable_v<T> && ! is_const_v<T>);

  std::memmove((void *) new_location,
               (void*) begin, (end - begin) * sizeof(T));
  return new_location + (end - begin);
}

template <class T>
constexpr void __non_trivially_relocate(T* begin, T* end, T* new_location)
{
  if (begin < new_location && new_location < end) {
    new_location += (end - begin);
    while (begin != end) {
      std::construct_at(--new_location, std::move(*--end));
      end->~T();
    }
  }
  else {
    for ( ; begin != end; ++begin, ++new_location) {
      std::construct_at(new_location, std::move(*begin));
      begin->~T();
    }
  }
}

template <class T>
constexpr T* relocate(T* begin, T* end, T* new_location)
{
  static_assert(is_nothrow_relocatable_v<T> && ! is_const_v<T>);

  if consteval {
    __non_trivially_relocate(begin, end, new_location);
  }
  else {
    if constexpr (is_trivially_relocatable_v<T>) {
      trivially_relocate(begin, end, new_location);
    }
    else {
      __non_trivially_relocate(begin, end, new_location);
    }
  }

  return new_location + (end - begin);
}

}  // close namespace xstd

#endif // ! defined(INCLUDED_RELOCATE)

// Local Variables:
// c-basic-offset: 2
// End:

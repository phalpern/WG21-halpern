/* type_list.h                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_TYPE_LIST_H
#define INCLUDED_TYPE_LIST_H

#include <utility>
#include <cstdlib>

// General-purpose typelist
template <typename... T>
struct type_list
{
  static constexpr std::size_t size() { return 0; }
};

template <typename T0, typename... T>
struct type_list<T0, T...>
{
  static constexpr std::size_t size() { return 1 + sizeof...(T); }
  using head = T0;
  using tail = type_list<T...>;
};

/// Metafunction to get size of a type list
template <class TL>
constexpr inline std::size_t type_list_size_v = TL::size();

/// Metafunction to get Nth element of a type list
template <std::size_t N, class TL> struct type_list_element;

template <std::size_t N, class... Types>
struct type_list_element<N, type_list<Types...>>
{
#ifdef __clang__
  using type = __type_pack_element<N, Types...>;
#else
  using type = typename std::_Nth_type<N, Types...>::type;
#endif
};

template <std::size_t N, class TL>
using type_list_element_t = typename type_list_element<N, TL>::type;

// Metafunction for contcatonating two typelists
template <typename TL1, typename TL2> struct type_list_concat;

template <typename... T1, typename... T2>
struct type_list_concat<type_list<T1...>, type_list<T2...>>
{
  using type = type_list<T1..., T2...>;
};

template <typename TL1, typename TL2>
using type_list_concat_t = typename type_list_concat<TL1, TL2>::type;

/// Metafunction to count the number of elements, E, in `TL` for which
/// `Pred<E>::value` is true.
template <typename TL, template <class T> class Pred>
struct type_list_count
{
};

template <typename... T , template <class U> class Pred>
struct type_list_count<type_list<T...>, Pred>
{
  static constexpr inline std::size_t value = (Pred<T>::value + ... + 0);
};

template <typename TL, template <class T> class Pred>
constexpr inline std::size_t type_list_count_v =
  type_list_count<TL, Pred>::value;

/// Metafunction to return the index of the first element, E, in `TL` for which
/// `Pred<E>::value` is true.  Returns `TL::size() + 1` if E is not found.
class __find_cell
{
  std::size_t m_idx   = 0;
  bool        m_match = false;

public:
  constexpr std::size_t idx() { return m_idx; }

  // Given a chain (fold) of `__find_cell || bool || bool || ...`, the final
  // value of `idx()` will be the index of the first `true` value or, if there
  // are none, the index of one-past-the-end (i.e., the length of the chain).
  constexpr __find_cell&& operator||(bool rhs) &&
  {
    m_match = (m_match || rhs);
    if (! m_match)
      ++m_idx;
    return std::move(*this);
  }
};

template <typename TL, template <class U> class Pred>
struct type_list_find;

template <typename... T, template <class U> class Pred>
struct type_list_find<type_list<T...>, Pred>
{
  // Fold expression returns `__find_cell` having a true `m_match`.
  static constexpr inline std::size_t value =
    (__find_cell{} || ... || Pred<T>::value).idx();
};

template <typename TL, template <class U> class Pred>
constexpr inline std::size_t type_list_find_v =
  type_list_find<TL, Pred>::value;

#endif  // INCLUDED_TYPE_LIST_H

// Local Variables:
// c-basic-offset: 2
// End:

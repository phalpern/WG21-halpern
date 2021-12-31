/* aligned_type.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* This component defines `std::raw_aligned_storage`, `aligned_storage_for`,
 * and `std::aligned_type` as described in P1083 (see http://wg21.link/P1083)
 */

#ifndef INCLUDED_ALIGNED_TYPE_DOT_H
#define INCLUDED_ALIGNED_TYPE_DOT_H

#include <xstd.h>
#include <cstddef>
#include <type_traits>

BEGIN_NAMESPACE_XSTD

constexpr size_t max_align_v = alignof(max_align_t);

template <size_t Align, size_t Sz = Align>
struct raw_aligned_storage
{
  static_assert(0 == (Align & (Align - 1)), "Align must be a power of 2");

  static constexpr size_t alignment = Align;
  static constexpr size_t size      = (Sz + Align - 1) & ~(Align - 1);

  using type = raw_aligned_storage;

  constexpr       void* data()       noexcept { return buffer; }
  constexpr const void* data() const noexcept { return buffer; }

  alignas(alignment) byte buffer[size];
};

// Optional:
template <typename T>
struct aligned_storage_for : raw_aligned_storage<alignof(T), sizeof(T)>
{
  constexpr T& object() noexcept { return *static_cast<T *>(this->data()); }
  constexpr const T& object() const noexcept
    { return *static_cast<const T*>(this->data()); }
};

template <size_t Align, typename... Tp> struct aligned_type_imp;

template <size_t A, typename T0, typename... Tp>
struct aligned_type_imp<A, T0, Tp...>
{
  // Having both branches of the `conditional` be types that have their own
  // `type` member prevents compiler recursion on the unused branch.
  using type = typename conditional_t<sizeof(T0) == A && alignof(T0) == A,
                                      type_identity<T0>,
                                      aligned_type_imp<A, Tp...>>::type;
};

template <size_t Align>
struct aligned_type_imp<Align>
{
  using type = raw_aligned_storage<Align>;
};

// Compute the first of a list of types to match the specified `Align`.
template <size_t Align>
using aligned_type = typename aligned_type_imp<Align,
                                               unsigned char,
                                               unsigned short,
                                               unsigned int,
                                               unsigned long,
                                               unsigned long long,
                                               float,
                                               double,
                                               long double,
                                               void*,
                                               void (*)(),
                                               bool true_type::*,
                                               bool (true_type::*)(),
                                               max_align_t
                                           >::type;

END_NAMESPACE_XSTD

#endif // INCLUDED_ALIGNED_TYPE_DOT_H

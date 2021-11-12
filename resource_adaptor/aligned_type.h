/* aligned_type.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* This component defines `std::aligned_object_storage` `std::aligned_type` as
 * described in P1083 (see http://wg21.link/P1083)
 */

#ifndef INCLUDED_ALIGNED_TYPE_DOT_H
#define INCLUDED_ALIGNED_TYPE_DOT_H

#include <xstd.h>
#include <cstddef>
#include <type_traits>

BEGIN_NAMESPACE_XSTD

constexpr size_t max_align_v = alignof(max_align_t);

constexpr size_t natural_alignment(size_t sz)
{
    const size_t x = sz | alignof(max_align_t);
    return ((x ^ (x - 1)) + 1) >> 1;
}

template <size_t N,
          size_t Align = natural_alignment(N)>
struct aligned_object_storage
{
    static constexpr size_t alignment = Align;
    static constexpr size_t size      = ((N + Align - 1) / Align) * Align;

    using type = aligned_object_storage;

    constexpr       unsigned char* storage()       { return _filler; }
    constexpr const unsigned char* storage() const { return _filler; }

    alignas(alignment) unsigned char _filler[size];  // Private name
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
    using type = aligned_object_storage<Align, Align>;
};

// Compute the first of a list of types to match the specified `Align`.
template <size_t Align>
using aligned_type = typename aligned_type_imp<Align,
    max_align_t,
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
    bool (true_type::*)()
    >::type;

END_NAMESPACE_XSTD

#endif // INCLUDED_ALIGNED_TYPE_DOT_H

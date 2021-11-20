/* resource_adaptor.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* This component defines `std::pmr::resource_adaptor` as described in P1083
 * (see http://wg21.link/P1083)
 */

#ifndef INCLUDED_RESOURCE_ADAPTOR_DOT_H
#define INCLUDED_RESOURCE_ADAPTOR_DOT_H

#include <xstd.h>
#include <memory>
#include <cstddef> // max_align_t
#include <cstdlib>
#include <cassert>
#include <utility>
#include <memory_resource>
#include <aligned_type.h>

BEGIN_NAMESPACE_XPMR

// Adaptor to make a polymorphic allocator resource type from an STL allocator
// type.
template <typename Allocator, size_t MaxAlignment>
class resource_adaptor_imp : public memory_resource
{
    static_assert(0 == (MaxAlignment & (MaxAlignment - 1)),
                  "MaxAlignment must be a power of 2");

    Allocator m_alloc;

  public:
    typedef Allocator allocator_type;

    static constexpr size_t max_alignment = MaxAlignment;

    resource_adaptor_imp() = default;

    resource_adaptor_imp(const resource_adaptor_imp&) noexcept = default;

    template <class Allocator2>
    resource_adaptor_imp(Allocator2&& a2, typename
                         enable_if<is_convertible<Allocator2, Allocator>::value,
                                   int>::type = 0) noexcept;

    allocator_type get_allocator() const noexcept { return m_alloc; }

  private:
    template <size_t Align, typename F>
    void linear_search_alignments(size_t alignment, F&& f);

    void *do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void *p, size_t bytes, size_t alignment) override;
    bool do_is_equal(const memory_resource& other) const noexcept override;
};

// This alias ensures that 'resource_adaptor<SomeAlloc<T>>' and
// 'resource_adaptor<SomeAlloc<U>>' are always the same type, whether or not
// 'T' and 'U' are the same type.
template <class Allocator, size_t MaxAlignment = alignof(max_align_t)>
using resource_adaptor = resource_adaptor_imp<
    typename std::allocator_traits<Allocator>::template rebind_alloc<std::byte>,
    MaxAlignment>;

namespace _details {

// Return the log base 2 of `n`, rounded down. Precondition: `n > 0`
constexpr int integralLog2(size_t n)
{
    int result = 0;
    while (n != 1) {
        n >>= 1;
        ++result;
    }
    return result;
}

} // close namespace `_details`

END_NAMESPACE_XPMR

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

template <class Allocator, size_t MaxAlignment>
template <class Allocator2>
inline
XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::resource_adaptor_imp(
    Allocator2&& a2, typename
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type) noexcept
    : m_alloc(forward<Allocator2>(a2))
{
}

// Perform a binary search for `alignment` among supported alignments.
template <class Allocator, size_t MaxAlignment>
template <size_t Align, typename F>
inline void XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
linear_search_alignments(size_t alignment, F&& f)
{
    if constexpr (Align > MaxAlignment) {
        if (alignment > MaxAlignment)
            throw bad_alloc{};
    }
    else if (alignment == Align) {
        using chunk_alloc = typename allocator_traits<Allocator>::
            template rebind_alloc<aligned_type<Align>>;
        chunk_alloc rebound_alloc(m_alloc);
        f(rebound_alloc, alignment);
    }
    else
        linear_search_alignments<Align * 2>(alignment, f);
}

template <class Allocator, size_t MaxAlignment>
void* XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
do_allocate(size_t bytes, size_t alignment)
{
    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > MaxAlignment)
            alignment = MaxAlignment;
    }

    size_t chunks = (bytes + alignment - 1) / alignment;

    void* ret = nullptr;
    linear_search_alignments<1>(
        alignment,
        [chunks,&ret](auto alloc, size_t alignment) {
            ret = allocator_traits<decltype(alloc)>::allocate(alloc, chunks);
        });

    return ret;
}

template <class Allocator, size_t MaxAlignment>
void XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
do_deallocate(void *p, size_t  bytes, size_t  alignment)
{
    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > MaxAlignment)
            alignment = MaxAlignment;
    }

    // Assert that `alignment` is a power of 2
    assert(0 == (alignment & (alignment - 1)));

    size_t chunks = (bytes + alignment - 1) / alignment;

    linear_search_alignments<1>(
        alignment,
        [p,chunks](auto alloc, size_t alignment) {
            using chunk_alloc_traits = allocator_traits<decltype(alloc)>;
            auto p2 = static_cast<typename chunk_alloc_traits::pointer>(p);
            chunk_alloc_traits::deallocate(alloc, p2, chunks);
        });
}

template <class Allocator, size_t MaxAlignment>
bool XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
do_is_equal(const memory_resource& other) const noexcept
{
    const resource_adaptor_imp *other_p =
        dynamic_cast<const resource_adaptor_imp*>(&other);

    if (other_p)
        return this->m_alloc == other_p->m_alloc;
    else
        return false;
}

#endif // ! defined(INCLUDED_RESOURCE_ADAPTOR_DOT_H)

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
#include <utility>
#include <cassert>
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

  public:
    typedef Allocator allocator_type;

    static constexpr size_t max_alignment = MaxAlignment;

    resource_adaptor_imp() = default;

    resource_adaptor_imp(const resource_adaptor_imp&) noexcept = default;

    template <class... Args>
    requires (std::is_constructible_v<Allocator, Args...>)
        explicit resource_adaptor_imp(Args&&... args);

    allocator_type get_allocator() const noexcept { return m_alloc; }

  private:
    // Compute the log2(n), rounded down, for n <= MaxAlignment.  Uses at most
    // log2(log2(MaxAlignment)+1) right-shift operations (though each
    // right-shift might be multiple bits) and the same number of conditinal
    // branches plus one.
    constexpr size_t integralLog2(size_t n)
    {
#       define LOG2_CHECK(e)                                            \
            if constexpr ((1ULL << (e)) <= MaxAlignment)                \
                if ((1ULL << (e)) <= n)

        size_t result = 0;
        LOG2_CHECK(32) { result += 32; n >>= 32; }
        LOG2_CHECK(16) { result += 16; n >>= 16; }
        LOG2_CHECK( 8) { result +=  8; n >>=  8; }
        LOG2_CHECK( 4) { result +=  4; n >>=  4; }
        LOG2_CHECK( 2) { result +=  2; n >>=  2; }
        LOG2_CHECK( 1) { result +=  1;           }

#       undef LOG2_CHECK

        return result;
    }

    Allocator m_alloc;

    template <size_t Align>
    void *aligned_allocate(size_t chunks);

    template <size_t Align>
    void aligned_deallocate(void *p, size_t chunks);

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

END_NAMESPACE_XPMR

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

template <class Allocator, size_t MaxAlignment>
template <class... Args>
    requires (std::is_constructible_v<Allocator, Args...>)
XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::resource_adaptor_imp(Args&&... args)
    : m_alloc(std::forward<Args>(args)...)
{
}

template <class Allocator, size_t MaxAlignment>
template <size_t Align>
void* XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
aligned_allocate(size_t chunks)
{
    using chunk_alloc_t = allocator_traits<Allocator>::
        template rebind_alloc<aligned_type<Align>>;

    chunk_alloc_t chunk_alloc(m_alloc);
    return allocator_traits<chunk_alloc_t>::allocate(chunk_alloc, chunks);
}

template <class Allocator, size_t MaxAlignment>
template <size_t Align>
void XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
aligned_deallocate(void *p, size_t chunks)
{
    using chunk_alloc_t = allocator_traits<Allocator>::
        template rebind_alloc<aligned_type<Align>>;

    chunk_alloc_t chunk_alloc(m_alloc);
    auto chunk_p = static_cast<aligned_type<Align> *>(p);
    allocator_traits<chunk_alloc_t>::deallocate(chunk_alloc, chunk_p, chunks);
}

template <class Allocator, size_t MaxAlignment>
void *XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
do_allocate(size_t bytes, size_t alignment)
{
    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > MaxAlignment)
            alignment = MaxAlignment;
    }
    else if (0 != (alignment & (alignment - 1)))
        // Assert that `alignment` is a power of 2
        assert(0 == (alignment & (alignment - 1)));

    size_t chunks = (bytes + alignment - 1) / alignment;

#define ALLOC_CASE(n) \
    case (n): if constexpr ((1ULL << (n)) <= MaxAlignment)      \
        return aligned_allocate<(1ULL << (n))>(chunks)

    switch (integralLog2(alignment)) {
        ALLOC_CASE(0);
        ALLOC_CASE(1);
        ALLOC_CASE(2);
        ALLOC_CASE(3);
        ALLOC_CASE(4);
        ALLOC_CASE(5);
        ALLOC_CASE(6);
        ALLOC_CASE(7);
        ALLOC_CASE(8);
        ALLOC_CASE(9);
        ALLOC_CASE(10);
        ALLOC_CASE(11);
        ALLOC_CASE(12);
        ALLOC_CASE(13);
        ALLOC_CASE(14);
        ALLOC_CASE(15);
        ALLOC_CASE(16);
        ALLOC_CASE(17);
        ALLOC_CASE(18);
        ALLOC_CASE(19);
        ALLOC_CASE(20);
        ALLOC_CASE(21);
        ALLOC_CASE(22);
        ALLOC_CASE(23);
        ALLOC_CASE(24);
        ALLOC_CASE(25);
        ALLOC_CASE(26);
        ALLOC_CASE(27);
        ALLOC_CASE(28);
        ALLOC_CASE(29);
        ALLOC_CASE(30);
        ALLOC_CASE(31);
        ALLOC_CASE(32);
        ALLOC_CASE(33);
        ALLOC_CASE(34);
        ALLOC_CASE(35);
        ALLOC_CASE(36);
        ALLOC_CASE(37);
        ALLOC_CASE(38);
        ALLOC_CASE(39);
        ALLOC_CASE(40);
        ALLOC_CASE(41);
        ALLOC_CASE(42);
        ALLOC_CASE(43);
        ALLOC_CASE(44);
        ALLOC_CASE(45);
        ALLOC_CASE(46);
        ALLOC_CASE(47);
        ALLOC_CASE(48);
        ALLOC_CASE(49);
        ALLOC_CASE(50);
        ALLOC_CASE(51);
        ALLOC_CASE(52);
        ALLOC_CASE(53);
        ALLOC_CASE(54);
        ALLOC_CASE(55);
        ALLOC_CASE(56);
        ALLOC_CASE(57);
        ALLOC_CASE(58);
        ALLOC_CASE(59);
        ALLOC_CASE(60);
        ALLOC_CASE(61);
        ALLOC_CASE(62);
        ALLOC_CASE(63);
        default:
            throw bad_alloc{};
    } // end switch

#undef ALLOC_CASE

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
    else
        // Assert that `alignment` is a power of 2
        assert(0 == (alignment & (alignment - 1)));

    size_t chunks = (bytes + alignment - 1) / alignment;

#define DEALLOC_CASE(n) \
    case (n): if constexpr ((1ULL << (n)) <= MaxAlignment)      \
        return aligned_deallocate<(1ULL << (n))>(p, chunks)

    switch (integralLog2(alignment)) {
        DEALLOC_CASE(0);
        DEALLOC_CASE(1);
        DEALLOC_CASE(2);
        DEALLOC_CASE(3);
        DEALLOC_CASE(4);
        DEALLOC_CASE(5);
        DEALLOC_CASE(6);
        DEALLOC_CASE(7);
        DEALLOC_CASE(8);
        DEALLOC_CASE(9);
        DEALLOC_CASE(10);
        DEALLOC_CASE(11);
        DEALLOC_CASE(12);
        DEALLOC_CASE(13);
        DEALLOC_CASE(14);
        DEALLOC_CASE(15);
        DEALLOC_CASE(16);
        DEALLOC_CASE(17);
        DEALLOC_CASE(18);
        DEALLOC_CASE(19);
        DEALLOC_CASE(20);
        DEALLOC_CASE(21);
        DEALLOC_CASE(22);
        DEALLOC_CASE(23);
        DEALLOC_CASE(24);
        DEALLOC_CASE(25);
        DEALLOC_CASE(26);
        DEALLOC_CASE(27);
        DEALLOC_CASE(28);
        DEALLOC_CASE(29);
        DEALLOC_CASE(30);
        DEALLOC_CASE(31);
        DEALLOC_CASE(32);
        DEALLOC_CASE(33);
        DEALLOC_CASE(34);
        DEALLOC_CASE(35);
        DEALLOC_CASE(36);
        DEALLOC_CASE(37);
        DEALLOC_CASE(38);
        DEALLOC_CASE(39);
        DEALLOC_CASE(40);
        DEALLOC_CASE(41);
        DEALLOC_CASE(42);
        DEALLOC_CASE(43);
        DEALLOC_CASE(44);
        DEALLOC_CASE(45);
        DEALLOC_CASE(46);
        DEALLOC_CASE(47);
        DEALLOC_CASE(48);
        DEALLOC_CASE(49);
        DEALLOC_CASE(50);
        DEALLOC_CASE(51);
        DEALLOC_CASE(52);
        DEALLOC_CASE(53);
        DEALLOC_CASE(54);
        DEALLOC_CASE(55);
        DEALLOC_CASE(56);
        DEALLOC_CASE(57);
        DEALLOC_CASE(58);
        DEALLOC_CASE(59);
        DEALLOC_CASE(60);
        DEALLOC_CASE(61);
        DEALLOC_CASE(62);
        DEALLOC_CASE(63);
        default:
            assert(0 && "Alignment should be < MaxAlignment");
    } // end switch

#undef DEALLOC_CASE
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

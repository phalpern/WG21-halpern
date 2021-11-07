/* polymorphic_allocator.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H
#define INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H

#include <xstd.h>
#include <memory>
#include <cstddef> // max_align_t
#include <cstdlib>
#include <utility>
#include <experimental/memory_resource>

namespace std::pmr { using namespace std::experimental::pmr; }

#if __cplusplus < 201703L
BEGIN_NAMESPACE_XSTD
enum byte : unsigned char { };
END_NAMESPACE_XSTD
#endif

BEGIN_NAMESPACE_XPMR

// Adaptor to make a polymorphic allocator resource type from an STL allocator
// type.
template <typename Allocator, size_t MaxAlignment>
class resource_adaptor_imp : public memory_resource
{
    static_assert(0 == (MaxAlignment & (MaxAlignment - 1)),
                  "MaxAlignment must be a power of 2");

    Allocator m_alloc;

    template <size_t Align>
    void *aligned_allocate(size_t bytes);

    template <size_t Align>
    void aligned_deallocate(void *p, size_t bytes);

  public:
    typedef Allocator allocator_type;

    static constexpr size_t max_alignment = MaxAlignment;

    resource_adaptor_imp() = default;

    resource_adaptor_imp(const resource_adaptor_imp&) = default;

    template <class Allocator2>
    resource_adaptor_imp(Allocator2&& a2, typename
                         enable_if<is_convertible<Allocator2, Allocator>::value,
                                   int>::type = 0);

    allocator_type get_allocator() const { return m_alloc; }

  private:
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

namespace __details {

template <size_t Align>
struct aligned_chunk {
    alignas(Align) char x;
};

template <> struct aligned_chunk<1> { char x; };
template <> struct aligned_chunk<2> { short x; };
template <> struct aligned_chunk<4> { int x; };
template <> struct aligned_chunk<8> { long long x; };

} // end namespace __details

END_NAMESPACE_XPMR

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

template <class Allocator, size_t MaxAlignment>
template <class Allocator2>
inline
XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::resource_adaptor_imp(
    Allocator2&& a2, typename
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type)
    : m_alloc(forward<Allocator2>(a2))
{
}

template <class Allocator, size_t MaxAlignment>
template <size_t Align>
void *XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
             aligned_allocate(size_t bytes)
{
    typedef __details::aligned_chunk<Align> chunk;
    size_t chunks = (bytes + Align - 1) / Align;

    typedef typename allocator_traits<Allocator>::
        template rebind_traits<chunk> chunk_traits;
    typename chunk_traits::allocator_type rebound(m_alloc);
    return chunk_traits::allocate(rebound, chunks);
}

template <class Allocator, size_t MaxAlignment>
template <size_t Align>
void XPMR::resource_adaptor_imp<Allocator, MaxAlignment>::
aligned_deallocate(void *p, size_t  bytes)
{
    typedef __details::aligned_chunk<Align> chunk;
    size_t chunks = (bytes + Align - 1) / Align;

    typedef  typename allocator_traits<Allocator>::
        template rebind_traits<chunk> chunk_traits;
    typename chunk_traits::allocator_type rebound(m_alloc);
    return chunk_traits::deallocate(rebound, static_cast<chunk*>(p), chunks);
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

#define ALLOC_CASE(n) \
    case (1ULL << (n)): if constexpr ((1ULL << (n)) <= MaxAlignment)      \
        return aligned_allocate<(1ULL << (n))>(bytes)

    switch (alignment) {
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

    // Assert that `alignment` is a power of 2
    assert(0 == (alignment & (alignment - 1)));

#define DEALLOC_CASE(n) \
    case (1ULL << (n)): if constexpr ((1ULL << (n)) <= MaxAlignment)      \
        return aligned_deallocate<(1ULL << (n))>(p, bytes)

    switch (alignment) {
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
            throw bad_alloc{};
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

#endif // ! defined(INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H)

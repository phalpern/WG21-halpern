/* polymorphic_allocator.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H
#define INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H

#include <allocator_traits.h>
#include <uses_allocator_wrapper.h>
#include <xstd.h>
#include <atomic>
#include <memory>
#include <stdlib.h>
#include <new>
#include <utility>

BEGIN_NAMESPACE_XSTD

using namespace std;

typedef double max_align_t;

#if __cplusplus < 201703L
enum byte : unsigned char { };
#endif

namespace pmr {

// Abstract base class for allocator resources.
class memory_resource
{
    static atomic<memory_resource *> s_default_resource;

    friend memory_resource *set_default_resource(memory_resource *);
    friend memory_resource *get_default_resource();

  public:
    virtual ~memory_resource();

    void* allocate(size_t bytes, size_t alignment = 0)
        { return do_allocate(bytes, alignment); }
    void  deallocate(void *p, size_t bytes, size_t alignment = 0)
        { do_deallocate(p, bytes, alignment); }

    // 'is_equal' is needed because polymorphic allocators are sometimes
    // produced as a result of type erasure.  In that case, two different
    // instances of a polymorphic_memory_resource may actually represent
    // the same underlying allocator and should compare equal, even though
    // their addresses are different.
    bool is_equal(const memory_resource& other) const
        { return do_is_equal(other); }

  private:
    virtual void* do_allocate(size_t bytes, size_t alignment) = 0;
    virtual void  do_deallocate(void *p, size_t bytes, size_t alignment) = 0;
    virtual bool do_is_equal(const memory_resource& other) const = 0;
};

inline
bool operator==(const memory_resource& a, const memory_resource& b)
{
    // The call 'is_equal' because some polymorphic allocators are produced as
    // a result of type erasure.  In that case, 'a' and 'b' may contain
    // 'memory_resource's with different addresses which, nevertheless,
    // should compare equal.
    return &a == &b || a.is_equal(b);
}

inline
bool operator!=(const memory_resource& a, const memory_resource& b)
{
    return ! (a == b);
}

// Adaptor to make a polymorphic allocator resource type from an STL allocator
// type.
template <typename Allocator, size_t MaxAlignment>
class resource_adaptor_imp : public memory_resource
{
    Allocator m_alloc;

    template <size_t Align>
    void *aligned_allocate(size_t bytes);

    template <size_t Align>
    void aligned_deallocate(void *p, size_t bytes);

  public:
    typedef Allocator allocator_type;

    resource_adaptor_imp() = default;

    resource_adaptor_imp(const resource_adaptor_imp&) = default;

    template <class Allocator2>
    resource_adaptor_imp(Allocator2&& a2, typename
                         enable_if<is_convertible<Allocator2, Allocator>::value,
                                   int>::type = 0);

    allocator_type get_allocator() const { return m_alloc; }

  private:
    virtual void *do_allocate(size_t bytes, size_t alignment);
    virtual void do_deallocate(void *p, size_t bytes, size_t alignment);

    virtual bool do_is_equal(const memory_resource& other) const;
};

// This alias ensures that 'resource_adaptor<SomeAlloc<T>>' and
// 'resource_adaptor<SomeAlloc<U>>' are always the same type, whether or not
// 'T' and 'U' are the same type.
template <class Allocator, size_t MaxAlignment = alignof(max_align_t)>
using resource_adaptor = resource_adaptor_imp<
    typename std::allocator_traits<Allocator>::template rebind_alloc<std::byte>,
    MaxAlignment>;

// An allocator resource that uses '::operator new' and '::operator delete' to
// manage memory is created by adapting 'std::allocator':
using new_delete_resource = resource_adaptor<std::allocator<std::byte>>;

// Return a pointer to a global instance of 'new_delete_resource'.
new_delete_resource *new_delete_resource_singleton();

// Get the current default resource
memory_resource *get_default_resource();

// Set the default resource
memory_resource *set_default_resource(memory_resource *r);

// STL allocator that holds a pointer to a polymorphic allocator resource.
template <class Tp = byte>
class polymorphic_allocator
{
    memory_resource* m_resource;

  public:
    typedef Tp value_type;

    // g++-4.6.3 does not use allocator_traits in shared_ptr, so we have to
    // provide an explicit rebind.
    template <typename U>
    struct rebind { typedef polymorphic_allocator<U> other; };

    polymorphic_allocator();
    polymorphic_allocator(memory_resource *r);

    template <class U>
    polymorphic_allocator(const polymorphic_allocator<U>& other);

    Tp *allocate(size_t n);
    void deallocate(Tp *p, size_t n);

    void* allocate_bytes(size_t nbytes,
                         size_t alignment = alignof(max_align_t));
    void deallocate_bytes(void* p, size_t nbytes,
                          size_t alignment = alignof(max_align_t));

    template <class T>
      T* allocate_object(size_t n = 1);
    template <class T>
      void deallocate_object(T* p, size_t n = 1);

    template <class T, class... CtorArgs>
      T* new_object(CtorArgs&&... ctor_args);
    template <class T>
      void delete_object(T* p);

    template <typename T, typename... Args>
      void construct(T* p, Args&&... args);

    // Specializations for pair using piecewise constructionw
    template <class T1, class T2>
      void construct(std::pair<T1,T2>* p);
    template <class T1, class T2, class U, class V>
      void construct(std::pair<T1,T2>* p, U&& x, V&& y);
    template <class T1, class T2, class U, class V>
      void construct(std::pair<T1,T2>* p, const std::pair<U, V>& pr);
    template <class T1, class T2, class U, class V>
      void construct(std::pair<T1,T2>* p, std::pair<U, V>&& pr);

    template <typename T>
      void destroy(T* p);

    // Return a default-constructed allocator
    polymorphic_allocator select_on_container_copy_construction() const;

    memory_resource *resource() const;
};

template <class T1, class T2>
bool operator==(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b);

template <class T1, class T2>
bool operator!=(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b);

namespace __details {

template <size_t Align> struct aligned_chunk;

template <> struct aligned_chunk<1> { char x; };
template <> struct aligned_chunk<2> { short x; };
template <> struct aligned_chunk<4> { int x; };
template <> struct aligned_chunk<8> { long long x; };
template <> struct aligned_chunk<16> { __attribute__((aligned(16))) char x; };
template <> struct aligned_chunk<32> { __attribute__((aligned(32))) char x; };
template <> struct aligned_chunk<64> { __attribute__((aligned(64))) char x; };

template <typename Tp>
struct calc_alignment
{
    char a;
    Tp   x;
public:
    calc_alignment();
    calc_alignment(const calc_alignment&);
    ~calc_alignment();
};

// #define alignof(T) \
//     (sizeof(XSTD::pmr::__details::calc_alignment<T>) - sizeof(T))

} // end namespace __details

} // end namespace pmr

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

inline
pmr::memory_resource::~memory_resource()
{
}

inline
pmr::memory_resource *
pmr::get_default_resource()
{
    memory_resource *ret =
        pmr::memory_resource::s_default_resource.load();
    if (nullptr == ret)
        ret = new_delete_resource_singleton();
    return ret;
}

inline
pmr::memory_resource *
pmr::set_default_resource(pmr::memory_resource *r)
{
    if (nullptr == r)
        r = new_delete_resource_singleton();

    // TBD, should use an atomic swap
    pmr::memory_resource *prev = get_default_resource();
    pmr::memory_resource::s_default_resource.store(r);
    return prev;
}

template <class Allocator, size_t MaxAlignment>
template <class Allocator2>
inline
pmr::resource_adaptor_imp<Allocator, MaxAlignment>::resource_adaptor_imp(
    Allocator2&& a2, typename
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type)
    : m_alloc(forward<Allocator2>(a2))
{
}

template <class Allocator, size_t MaxAlignment>
template <size_t Align>
void *pmr::resource_adaptor_imp<Allocator, MaxAlignment>::
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
void pmr::resource_adaptor_imp<Allocator, MaxAlignment>::
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
void *pmr::resource_adaptor_imp<Allocator, MaxAlignment>::
do_allocate(size_t bytes, size_t alignment)
{
    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > MaxAlignment)
            alignment = MaxAlignment;
    }

#define ALLOC_CASE(n) \
    case (1ULL << (n)): if constexpr (MaxAlignment <= (1ULL << (n)))      \
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
void pmr::resource_adaptor_imp<Allocator, MaxAlignment>::
do_deallocate(void *p, size_t  bytes, size_t  alignment)
{
    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > MaxAlignment)
            alignment = MaxAlignment;
    }

#define DEALLOC_CASE(n) \
    case (1ULL << (n)): if constexpr (MaxAlignment <= (1ULL << (n)))      \
        return aligned_deallocate<(1ULL << (n))>(bytes)

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
bool pmr::resource_adaptor_imp<Allocator, MaxAlignment>::
do_is_equal(const memory_resource& other) const
{
    const resource_adaptor_imp *other_p =
        dynamic_cast<const resource_adaptor_imp*>(&other);

    if (other_p)
        return this->m_alloc == other_p->m_alloc;
    else
        return false;
}


template <class Tp>
inline
pmr::polymorphic_allocator<Tp>::polymorphic_allocator()
    : m_resource(get_default_resource())
{
}

template <class Tp>
inline
pmr::polymorphic_allocator<Tp>::polymorphic_allocator(pmr::memory_resource *r)
    : m_resource(r ? r : get_default_resource())
{
}

template <class Tp>
    template <class U>
inline
pmr::polymorphic_allocator<Tp>::polymorphic_allocator(
    const pmr::polymorphic_allocator<U>& other)
    : m_resource(other.resource())
{
}

template <class Tp>
inline
Tp *pmr::polymorphic_allocator<Tp>::allocate(size_t n)
{
    return static_cast<Tp*>(m_resource->allocate(n * sizeof(Tp), alignof(Tp)));
}

template <class Tp>
inline
void pmr::polymorphic_allocator<Tp>::deallocate(Tp *p, size_t n)
{
    m_resource->deallocate(p, n * sizeof(Tp), alignof(Tp));
}

template <class Tp> inline
void* pmr::polymorphic_allocator<Tp>::allocate_bytes(size_t nbytes,
                                                     size_t alignment)
{
    return m_resource->allocate(nbytes, alignment);
}

template <class Tp> inline
void pmr::polymorphic_allocator<Tp>::deallocate_bytes(void* p, size_t nbytes,
                                                      size_t alignment)
{
    m_resource->deallocate(p, nbytes, alignment);
}

template <class Tp>
template <class T> inline
T* pmr::polymorphic_allocator<Tp>::allocate_object(size_t n)
{
    return static_cast<T*>(allocate_bytes(n * sizeof(T), alignof(T)));
}

template <class Tp>
template <class T> inline
  void pmr::polymorphic_allocator<Tp>::deallocate_object(T* p, size_t n)
{
    deallocate_bytes(p, n * sizeof(T), alignof(T));
}

template <class Tp>
template <class T, class... CtorArgs>
T* pmr::polymorphic_allocator<Tp>::new_object(CtorArgs&&... ctor_args)
{
    void* p = allocate_object<T>();
    try {
        construct(p, std::forward<CtorArgs>(ctor_args)...);
    } catch (...) {
        m_resource->deallocate(p, sizeof(T), alignof(T));
        throw;
    }
}

template <class Tp>
template <class T>
void pmr::polymorphic_allocator<Tp>::delete_object(T* p)
{
    destroy(p);
    deallocate_object(p);
}

template <class Tp>
template <typename T, typename... Args>
void pmr::polymorphic_allocator<Tp>::construct(T* p, Args&&... args)
{
    using XSTD::uses_allocator_construction_wrapper;
    typedef uses_allocator_construction_wrapper<T> Wrapper;
    ::new((void*) p) Wrapper(allocator_arg, *this, std::forward<Args>(args)...);
}

// Specializations to pass inner_allocator to pair::first and pair::second
template <class Tp>
template <class T1, class T2>
void pmr::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p)
{
    // TBD: Should really use piecewise construction here
    construct(addressof(p->first));
    try {
        construct(addressof(p->second));
    }
    catch (...) {
        destroy(addressof(p->first));
        throw;
    }
}

template <class Tp>
template <class T1, class T2, class U, class V>
void pmr::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
                                                     U&& x, V&& y)
{
    // TBD: Should really use piecewise construction here
    construct(addressof(p->first), std::forward<U>(x));
    try {
        construct(addressof(p->second), std::forward<V>(y));
    }
    catch (...) {
        destroy(addressof(p->first));
        throw;
    }
}

template <class Tp>
template <class T1, class T2, class U, class V>
void pmr::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
                                                     const std::pair<U, V>& pr)
{
    // TBD: Should really use piecewise construction here
    construct(addressof(p->first), pr.first);
    try {
        construct(addressof(p->second), pr.second);
    }
    catch (...) {
        destroy(addressof(p->first));
        throw;
    }
}

template <class Tp>
template <class T1, class T2, class U, class V>
void pmr::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
                                                     std::pair<U, V>&& pr)
{
    // TBD: Should really use piecewise construction here
    construct(addressof(p->first), std::move(pr.first));
    try {
        construct(addressof(p->second), std::move(pr.second));
    }
    catch (...) {
        destroy(addressof(p->first));
        throw;
    }
}

template <class Tp>
template <typename T>
void pmr::polymorphic_allocator<Tp>::destroy(T* p)
{
    p->~T();
}

template <class Tp>
inline
pmr::polymorphic_allocator<Tp>
pmr::polymorphic_allocator<Tp>::select_on_container_copy_construction()
    const
{
    return pmr::polymorphic_allocator<Tp>();
}

template <class Tp>
inline
pmr::memory_resource *
pmr::polymorphic_allocator<Tp>::resource() const
{
    return m_resource;
}

template <class T1, class T2>
inline
bool pmr::operator==(const pmr::polymorphic_allocator<T1>& a,
                           const pmr::polymorphic_allocator<T2>& b)
{
    // 'operator==' for 'memory_resource' first checks for equality of
    // addresses and calls 'is_equal' only if the addresses differ.  The call
    // 'is_equal' because some polymorphic allocators are produced as a result
    // of type erasure.  In that case, 'a' and 'b' may contain
    // 'memory_resource's with different addresses which, nevertheless,
    // should compare equal.
    return *a.resource() == *b.resource();
}

template <class T1, class T2>
inline
bool pmr::operator!=(const pmr::polymorphic_allocator<T1>& a,
                           const pmr::polymorphic_allocator<T2>& b)
{
    return *a.resource() != *b.resource();
}

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H)

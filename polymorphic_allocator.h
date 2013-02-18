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

BEGIN_NAMESPACE_XSTD

using namespace std;

typedef double max_align_t;

namespace polyalloc {

// Abstract base class for allocator resources.
class allocator_resource
{
    static atomic<allocator_resource *> s_default_resource;

    static allocator_resource *init_default_resource();

  public:
    virtual ~allocator_resource();
    virtual void* allocate(size_t bytes, size_t alignment = 0) = 0;
    virtual void  deallocate(void *p, size_t bytes, size_t alignment = 0) = 0;

    // 'is_equal' is needed because polymorphic allocators are sometimes
    // produced as a result of type erasure.  In that case, two different
    // instances of a polymorphic_allocator_resource may actually represent
    // the same underlying allocator and should compare equal, even though
    // their addresses are different.
    virtual bool is_equal(const allocator_resource& other) const = 0;

    static allocator_resource *default_resource();
    static void set_default_resource(allocator_resource *r);
};

inline
bool operator==(const allocator_resource& a, const allocator_resource& b)
{
    return a.is_equal(b);
}

inline
bool operator!=(const allocator_resource& a, const allocator_resource& b)
{
    return ! a.is_equal(b);
}

// Adaptor make a polymorphic allocator resource type from an STL allocator
// type.
template <class Allocator>
class resource_adaptor_imp : public allocator_resource
{
    typename allocator_traits<Allocator>::
        template rebind_alloc<max_align_t>::_Base m_alloc;

    template <size_t Align>
    void *do_allocate(size_t bytes);

    template <size_t Align>
    void do_deallocate(void *p, size_t bytes);

  public:
    typedef Allocator allocator_type;

    resource_adaptor_imp();

    resource_adaptor_imp(const resource_adaptor_imp&) = default;

    template <class Allocator2>
    resource_adaptor_imp(Allocator2&& a2, typename
                         enable_if<is_convertible<Allocator2, Allocator>::value,
                                   int>::type = 0);

    virtual void *allocate(size_t bytes, size_t alignment = 0);
    virtual void deallocate(void *p, size_t bytes, size_t alignment = 0);

    virtual bool
    is_equal(const allocator_resource& other) const;

    allocator_type get_allocator() const { return m_alloc; }
};

// This alias ensures that 'resource_adaptor<T>' and
// 'resource_adaptor<U>' are always the same type, whether or not
// 'T' and 'U' are the same type.
#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
template <class Allocator>
using resource_adaptor = resource_adaptor_imp<
    allocator_traits<Allocator>::rebind_alloc<char>>;
#define POLYALLOC_RESOURCE_ADAPTOR(Alloc) \
    XSTD::polyalloc::resource_adaptor<Alloc >
#else
template <class Allocator>
struct resource_adaptor_mf
{
    typedef resource_adaptor_imp<typename
        allocator_traits<Allocator>::template rebind_alloc<char>::_Base > type;
};
#define POLYALLOC_RESOURCE_ADAPTOR(Alloc) \
    typename XSTD::polyalloc::resource_adaptor_mf<Alloc >::type
#endif

// STL allocator that holds a pointer to a polymorphic allocator resource.
template <class Tp>
class polymorphic_allocator
{
    allocator_resource* m_resource;

  public:
    typedef Tp value_type;

    // g++-4.6.3 does not use allocator_traits in shared_ptr, so we have to
    // provide an explicit rebind.
    template <typename U>
    struct rebind { typedef polymorphic_allocator<U> other; };

    polymorphic_allocator();
    polymorphic_allocator(allocator_resource *r);

    template <class U>
    polymorphic_allocator(const polymorphic_allocator<U>& other);

    Tp *allocate(size_t n);
    void deallocate(Tp *p, size_t n);

    template <typename T, typename... Args>
      void construct(T* p, Args&&... args);

    // Specializations to pass inner_allocator to pair::first and pair::second
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

    allocator_resource *resource() const;
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

#define alignof(T) \
    (sizeof(XSTD::polyalloc::__details::calc_alignment<T>) - sizeof(T))

} // end namespace __details
    
} // end namespace polyalloc

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

inline
polyalloc::allocator_resource::~allocator_resource()
{
}

inline
polyalloc::allocator_resource *
polyalloc::allocator_resource::default_resource()
{
    allocator_resource *ret = s_default_resource.load();
    if (nullptr == ret)
        ret = init_default_resource();
    return ret;
}

inline
void polyalloc::allocator_resource::set_default_resource(
    polyalloc::allocator_resource *r)
{
    s_default_resource.store(r);
}

template <class Allocator>
inline
polyalloc::resource_adaptor_imp<Allocator>::resource_adaptor_imp()
{
}

template <class Allocator>
    template <class Allocator2>
inline
polyalloc::resource_adaptor_imp<Allocator>::resource_adaptor_imp(
    Allocator2&& a2, typename
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type)
    : m_alloc(forward<Allocator2>(a2))
{
}

template <class Allocator>
template <size_t Align>
void *polyalloc::resource_adaptor_imp<Allocator>::do_allocate(size_t bytes)
{
    typedef __details::aligned_chunk<Align> chunk;
    size_t chunks = (bytes + Align - 1) / Align;
    
    typedef  typename allocator_traits<Allocator>::
        template rebind_traits<chunk> chunk_traits;
    typename chunk_traits::allocator_type rebound(m_alloc);
    return chunk_traits::allocate(rebound, chunks);
}

template <class Allocator>
template <size_t Align>
void polyalloc::resource_adaptor_imp<Allocator>::do_deallocate(void   *p,
                                                               size_t  bytes)
{
    typedef __details::aligned_chunk<Align> chunk;
    size_t chunks = (bytes + Align - 1) / Align;
    
    typedef  typename allocator_traits<Allocator>::
        template rebind_traits<chunk> chunk_traits;
    typename chunk_traits::allocator_type rebound(m_alloc);
    return chunk_traits::deallocate(rebound, static_cast<chunk*>(p), chunks);
}

template <class Allocator>
void *polyalloc::resource_adaptor_imp<Allocator>::allocate(size_t bytes,
                                                           size_t alignment)
{
    static const size_t max_natural_alignment = sizeof(max_align_t);

    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > max_natural_alignment)
            alignment = max_natural_alignment;
    }

    switch (alignment) {
      case 1: return do_allocate<1>(bytes);
      case 2: return do_allocate<2>(bytes);
      case 4: return do_allocate<4>(bytes);
      case 8: return do_allocate<8>(bytes);
      case 16: return do_allocate<16>(bytes);
      case 32: return do_allocate<32>(bytes);
      case 64: return do_allocate<64>(bytes);
      default: {
          size_t chunks = (bytes + sizeof(void*) + alignment - 1) / 64;
          size_t chunkbytes = chunks * 64;
          void *original = do_allocate<64>(chunkbytes);

          // Make room for original pointer storage
          char *p  = static_cast<char*>(original) + sizeof(void*);  

          // Round up to nearest alignment boundary
          p += alignment - 1;
          p -= (size_t(p)) & (alignment - 1);

          // Store original pointer in word before allocated pointer
          reinterpret_cast<void**>(p)[-1] = original;

          return p;
      }
    }
}

template <class Allocator>
void polyalloc::resource_adaptor_imp<Allocator>::deallocate(void   *p,
                                                            size_t  bytes,
                                                            size_t  alignment)
{
    static const size_t max_natural_alignment = sizeof(max_align_t);

    if (0 == alignment) {
        // Choose natural alignment for 'bytes'
        alignment = ((bytes ^ (bytes - 1)) >> 1) + 1;
        if (alignment > max_natural_alignment)
            alignment = max_natural_alignment;
    }

    switch (alignment) {
      case 1: do_deallocate<1>(p, bytes); break;
      case 2: do_deallocate<2>(p, bytes); break;
      case 4: do_deallocate<4>(p, bytes); break;
      case 8: do_deallocate<8>(p, bytes); break;
      case 16: do_deallocate<16>(p, bytes); break;
      case 32: do_deallocate<32>(p, bytes); break;
      case 64: do_deallocate<64>(p, bytes); break;
      default: {
          size_t chunks = (bytes + sizeof(void*) + alignment - 1) / 64;
          size_t chunkbytes = chunks * 64;
          void *original = reinterpret_cast<void**>(p)[-1];
          
          do_deallocate<64>(original, chunkbytes);
      }
    }
}

template <class Allocator>
bool polyalloc::resource_adaptor_imp<Allocator>::is_equal(
    const allocator_resource& other) const
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
polyalloc::polymorphic_allocator<Tp>::polymorphic_allocator()
    : m_resource(allocator_resource::default_resource())
{
}

template <class Tp>
inline
polyalloc::polymorphic_allocator<Tp>::polymorphic_allocator(
    polyalloc::allocator_resource *r)
    : m_resource(r)
{
}

template <class Tp>
    template <class U>
inline
polyalloc::polymorphic_allocator<Tp>::polymorphic_allocator(
    const polyalloc::polymorphic_allocator<U>& other)
    : m_resource(other.resource())
{
}

template <class Tp>
inline
Tp *polyalloc::polymorphic_allocator<Tp>::allocate(size_t n)
{
    return static_cast<Tp*>(m_resource->allocate(n * sizeof(Tp), alignof(Tp)));
}

template <class Tp>
inline
void polyalloc::polymorphic_allocator<Tp>::deallocate(Tp *p, size_t n)
{
    m_resource->deallocate(p, n * sizeof(Tp), alignof(Tp));
}

template <class Tp>
template <typename T, typename... Args>
void polyalloc::polymorphic_allocator<Tp>::construct(T* p, Args&&... args)
{
    using XSTD::uses_allocator_construction_wrapper;
    typedef uses_allocator_construction_wrapper<T> Wrapper;
    ::new((void*) p) Wrapper(allocator_arg, *this, std::forward<Args>(args)...);
}

// Specializations to pass inner_allocator to pair::first and pair::second
template <class Tp>
template <class T1, class T2>
void polyalloc::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p)
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
void polyalloc::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
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
void polyalloc::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
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
void polyalloc::polymorphic_allocator<Tp>::construct(std::pair<T1,T2>* p,
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
void polyalloc::polymorphic_allocator<Tp>::destroy(T* p)
{
    p->~T();
}

template <class Tp>
inline
polyalloc::polymorphic_allocator<Tp>
polyalloc::polymorphic_allocator<Tp>::select_on_container_copy_construction()
    const
{
    return polyalloc::polymorphic_allocator<Tp>();
}

template <class Tp>
inline
polyalloc::allocator_resource *
polyalloc::polymorphic_allocator<Tp>::resource() const
{
    return m_resource;
}

template <class T1, class T2>
inline
bool polyalloc::operator==(const polyalloc::polymorphic_allocator<T1>& a,
                           const polyalloc::polymorphic_allocator<T2>& b)
{
    if (a.resource() == b.resource())
        return true;
    else
        // This test is needed because some polymorphic allocators are
        // produced as a result of type erasure.  In that case, 'a' and 'b'
        // may contain 'allocator_resource's with different
        // addresses which, nevertheless, should compare equal.
        return a.resource()->is_equal(*b.resource());
}

template <class T1, class T2>
inline
bool polyalloc::operator!=(const polyalloc::polymorphic_allocator<T1>& a,
                           const polyalloc::polymorphic_allocator<T2>& b)
{
    return ! (a == b);
}

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H)

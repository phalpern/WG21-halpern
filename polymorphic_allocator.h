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
template <class Allocator>
class resource_adaptor_imp : public memory_resource
{
    typename allocator_traits<Allocator>::
        template rebind_alloc<max_align_t>::_Base m_alloc;

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

// This alias ensures that 'resource_adaptor<T>' and
// 'resource_adaptor<U>' are always the same type, whether or not
// 'T' and 'U' are the same type.
#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
template <class Allocator>
using resource_adaptor = resource_adaptor_imp<
    allocator_traits<Allocator>::rebind_alloc<char>>;
#define PMR_RESOURCE_ADAPTOR(Alloc) \
    XSTD::pmr::resource_adaptor<Alloc >
#else
template <class Allocator>
struct resource_adaptor_mf
{
    typedef resource_adaptor_imp<typename
        allocator_traits<Allocator>::template rebind_alloc<char>::_Base > type;
};
#define PMR_RESOURCE_ADAPTOR(Alloc) \
    typename XSTD::pmr::resource_adaptor_mf<Alloc >::type
#endif

// An allocator resource that uses '::operator new' and '::operator delete' to
// manage memory is created by adapting 'std::allocator':
typedef PMR_RESOURCE_ADAPTOR(std::allocator<char>) new_delete_resource;

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

template <class Allocator>
    template <class Allocator2>
inline
pmr::resource_adaptor_imp<Allocator>::resource_adaptor_imp(
    Allocator2&& a2, typename
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type)
    : m_alloc(forward<Allocator2>(a2))
{
}

template <class Allocator>
template <size_t Align>
void *pmr::resource_adaptor_imp<Allocator>::aligned_allocate(size_t bytes)
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
void pmr::resource_adaptor_imp<Allocator>::aligned_deallocate(void   *p,
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
void *pmr::resource_adaptor_imp<Allocator>::do_allocate(size_t bytes,
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
      case 1: return aligned_allocate<1>(bytes);
      case 2: return aligned_allocate<2>(bytes);
      case 4: return aligned_allocate<4>(bytes);
      case 8: return aligned_allocate<8>(bytes);
      case 16: return aligned_allocate<16>(bytes);
      case 32: return aligned_allocate<32>(bytes);
      case 64: return aligned_allocate<64>(bytes);
      default: {
          size_t chunks = (bytes + sizeof(void*) + alignment - 1) / 64;
          size_t chunkbytes = chunks * 64;
          void *original = aligned_allocate<64>(chunkbytes);

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
void pmr::resource_adaptor_imp<Allocator>::do_deallocate(void   *p,
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
      case 1: aligned_deallocate<1>(p, bytes); break;
      case 2: aligned_deallocate<2>(p, bytes); break;
      case 4: aligned_deallocate<4>(p, bytes); break;
      case 8: aligned_deallocate<8>(p, bytes); break;
      case 16: aligned_deallocate<16>(p, bytes); break;
      case 32: aligned_deallocate<32>(p, bytes); break;
      case 64: aligned_deallocate<64>(p, bytes); break;
      default: {
          size_t chunks = (bytes + sizeof(void*) + alignment - 1) / 64;
          size_t chunkbytes = chunks * 64;
          void *original = reinterpret_cast<void**>(p)[-1];

          aligned_deallocate<64>(original, chunkbytes);
      }
    }
}

template <class Allocator>
bool pmr::resource_adaptor_imp<Allocator>::do_is_equal(
    const memory_resource& other) const
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
pmr::polymorphic_allocator<Tp>::polymorphic_allocator(
    pmr::memory_resource *r)
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

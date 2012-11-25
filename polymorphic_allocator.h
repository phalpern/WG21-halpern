// polymorphic_allocator.h                  -*-C++-*-

#ifndef INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H
#define INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H

namespace std {

// Abstract base class for allocator resources.
// Equivalent to bslma_Allocator and bslma_Default.
class polymorphic_allocator_resource
{
    static atomic<polymorphic_allocator_resource *> s_default_resource;

  public:
    virtual ~polymorphic_allocator_resource();
    virtual void* allocate(size_t bytes, size_t alignment = 0) = 0;
    virtual void  deallocate(void *p, size_t bytes) = 0;

    virtual polymorphic_allocator_resource*
    select_on_container_copy_construction() const = 0;

    // Not clear if these are useful.  The 'polymorphic_allocator' adaptor
    // that converts a 'polymorphic_allocator_resource' into an STL-style
    // allocator cannot use these functions because these traits must be
    // compile-time traits in STL-style allocators.
    // virtual bool propagate_on_container_move_assignment() const = 0;
    // virtual bool propagate_on_container_copy_assignment() const = 0;
    // virtual bool propagate_on_container_swap() const = 0;

    virtual bool
    is_equal(const polymorphic_allocator_resource& other) const = 0;

    static polymorphic_allocator_resource *default_resource();
    static set_default_resource(polymorphic_allocator_resource *r);
};

inline
bool operator==(const polymorphic_allocator_resource& a,
                const polymorphic_allocator_resource& b)
{
    return a.is_equal(b);
}

// Adaptor make a polymorphic allocator resource type from an STL allocator
// type.
template <class Allocator>
class polymorphic_allocator_adaptor_imp : public polymorphic_allocator_resource
{
    allocator_traits<Allocator>::rebind_alloc<max_align_t> alloc;

  public:
    typedef Allocator allocator_type;

    polymorphic_allocator_adaptor_imp();

    template <class Allocator2>
    polymorphic_allocator_adaptor_imp(Allocator2&& a2,
        enable_if<is_convertible<Allocator2, Allocator>::value,
                  int>::type = 0);

    virtual void *allocate(size_t bytes, size_t alignment = 0);
    virtual void deallocate(void *p, size_t bytes, size_t alignment = 0);

    virtual polymorphic_allocator_resource*
    select_on_container_copy_construction() const;
    virtual bool propagate_on_container_move_assignment() const;
    virtual bool propagate_on_container_copy_assignment() const;
    virtual bool propagate_on_container_swap() const;

    virtual bool
    is_equal(const polymorphic_allocator_resource& other) const = 0;
};

// This alias ensures that 'polymorphic_allocator_adaptor<T>' and
// 'polymorphic_allocator_adaptor<U>' are always the same type, whether or not
// 'T' and 'U' are the same type.
template <class Allocator>
using polymorphic_allocator_adaptor = polymorphic_allocator_adaptor_imp<
    allocator_traits<Allocator>::rebind_alloc<char>>;

// STL allocator that holds a pointer to a polymorphic allocator resource.
// Equivalent to bsl::allocator except not a scoped allocator.
template <class Tp>
class polymorphic_allocator
{
    polymorphic_allocator_resource* d_resource;

  public:
    typedef Tp value_type;

    polymorphic_allocator();
    polymorphic_allocator(polymorphic_allocator_resource *r);

    template <class U>
    polymorphic_allocator(const polymorphic_allocator<U>& other);

    Tp *allocate(size_t n);
    void deallocate(Tp *p, size_t n);

    polymorphic_allocator_resource *resource() const;
};

template <class T1, class T2>
bool operator==(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b);

template <class T1, class T2>
bool operator!=(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b);


// Scoped allocator that holds a pointer to a polymorphic allocator resource.
// Equivalent to bsl::allocator.
template <class Tp>
using scoped_polymorphic_allocator =
    scoped_allocator_adaptor<polymorphic_allocator<Tp>>;

// Some possible refinements:
//
// 0. All class names are tentative.
//
// 1. If we want the polymorphic allocator to be scoped by default, then
//    rename 'polymorphic_allocator' to 'basic_polymorphic_allocator' and
//    rename 'scoped_polymorphic_allocator' to 'polymorphic_allocator'.
//
// 2. Rename 'polymorphic_allocator_adaptor' to
//    'polymorphic_allocator_adaptor_imp' and create an alias:
//
//    template <class Allocator>
//    using polymorphic_allocator_adaptor =
//        polymorphic_allocator_adaptor_imp<
//            allocator_traits<Allocator>::rebind_alloc<unspecified>>>
//
//    With this change 'polymorphic_allocator_adaptor<xyz<foo>>' will be the
//    same type as 'polymorphic_allocator_adaptor<xyz<bar>>'

///////////////////////////////////////////////////////////////////////////////
// INLINE AND TEMPLATE FUNCTION IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

inline
polymorphic_allocator_resource::~polymorphic_allocator_resource()
{
}

// TBD: Move to .cpp
atomic<polymorphic_allocator_resource *>
polymorphic_allocator_resource::s_default_resource(nullptr);

// TBD: Move to .cpp
polymorphic_allocator_resource *
polymorphic_allocator_resource::init_default_resource()
{
    // If no default resource has been set, then set 's_default_resource' to
    // the default default resource == the new/delete allocator.
    static once_flag init_once;
    static polymorphic_allocator_resource *default_resource_p;
    call_once(init_once, [&default_resource_p]{
            static
                polymorphic_resource_adaptor<allocator<char>> default_resource;
            default_resource_p = &default_resource;
        });
    polymorphic_allocator_resource *ret = simple_resource_p;
    s_default_resource.compare_exchange_strong(ret, nullptr);
    return ret;
}

inline
polymorphic_allocator_resource *
polymorphic_allocator_resource::default_resource()
{
    polymorphic_allocator_resource *ret = s_default_resource.load()
    if (nullptr == ret)
        ret = init_default_resource();
    return ret;
}

inline
void polymorphic_allocator_resource::set_default_resource(
    polymorphic_allocator_resource *r)
{
    s_default_resource.store(r);
}

template <class Allocator>
inline
polymorphic_allocator_adaptor_imp<Allocator>::
    polymorphic_allocator_adaptor_imp()
{
}

template <class Allocator>
    template <class Allocator2>
inline
polymorphic_allocator_adaptor_imp<Allocator>::
    polymorphic_allocator_adaptor_imp(
    Allocator2&& a2,
    enable_if<is_convertible<Allocator2, Allocator>::value, int>::type)
    : alloc(forward<Allocator2>(a2))
{
}

template <class Allocator>
template <size_t Align>
void *polymorphic_allocator_adaptor_imp<Allocator>::do_allocate(size_t bytes)
{
    struct chunk { char alignas(Align) a[Align]; };
    size_t chunks = (bytes + Align - 1) / Align;
    
    typedef  allocator_traits<Allocator>::rebind_traits<chunk> chunk_traits;
    return chunk_traits::allocate(alloc, chunks);
}

template <class Allocator>
void *polymorphic_allocator_adaptor_imp<Allocator>::allocate(size_t bytes,
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
        // All the way up
      case (size_t) 1 << 63: return do_allocate<(size_t) 1 << 63>(bytes);
      default: return do_allocate<max_natural_alignment>(bytes);
    }
}

template <class Allocator>
template <size_t Align>
void *polymorphic_allocator_adaptor_imp<Allocator>::do_allocate(size_t bytes,
                                                                void  *p);
{
    struct chunk { char alignas(Align) a[Align]; };
    size_t chunks = (bytes + Align - 1) / Align;
    
    typedef  allocator_traits<Allocator>::rebind_traits<chunk> chunk_traits;
    chunk_traits::deallocate(alloc, static_cast<chunk*>(p), chunks);
}

template <class Allocator>
void polymorphic_allocator_adaptor_imp<Allocator>::deallocate(void  *p,
                                                              size_t bytes,
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
      case 1: do_deallocate<1>(p, bytes);
      case 2: do_deallocate<2>(p, bytes);
      case 4: do_deallocate<4>(p, bytes);
      case 8: do_deallocate<8>(p, bytes);
      case 16: do_deallocate<16>(p, bytes);
      case 32: do_deallocate<32>(p, bytes);
      case 64: do_deallocate<64>(p, bytes);
        // All the way up
      case (size_t) 1 << 63: do_deallocate<(size_t) 1 << 63>(p, bytes);
      default: do_deallocate<max_natural_alignment>(p, bytes);
    }
}

template <class Tp>
inline
polymorphic_allocator<Tp>::polymorphic_allocator()
    : d_resource(default_allocator_resource())
{
}

template <class Tp>
inline
polymorphic_allocator<Tp>::polymorphic_allocator(
    polymorphic_allocator_resource *r)
    : d_resource(r)
{
}

template <class Tp>
    template <class U>
inline
polymorphic_allocator<Tp>::polymorphic_allocator(
    const polymorphic_allocator<U>& other)
    : d_resource(other.d_resource)
{
}

template <class Tp>
inline
Tp *polymorphic_allocator<Tp>::allocate(size_t n)
{
    d_resource.allocate(n * sizeof(Tp), align_of(Tp));
}

template <class Tp>
inline
void polymorphic_allocator<Tp>::deallocate(Tp *p, size_t n)
{
    d_resource.deallocate(p, n * sizeof(Tp), align_of(Tp));
}

template <class Tp>
inline
polymorphic_allocator_resource *polymorphic_allocator<Tp>::resource() const
{
    return d_resource;
}

template <class Tp>
inline
polymorphic_allocator
polymorphic_allocator::select_on_container_copy_construction()
{
    return d_resource->select_on_container_copy_construction();

    // ALTERNATIVE:
    // // Never propagate on copy construction.
    // return polymorphic_allocator();
}

template <class T1, class T2>
inline
bool operator==(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b)
{
    if (a.resource() == b.resource())
        return true;
    else
        return a.resource().is_equal(b.resource());
}

template <class T1, class T2>
inline
bool operator!=(const polymorphic_allocator<T1>& a,
                const polymorphic_allocator<T2>& b)
{
    return ! (a == b);
}

} // end namespace std

#endif // ! defined(INCLUDED_POLYMORPHIC_ALLOCATOR_DOT_H)

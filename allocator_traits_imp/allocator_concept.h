#ifndef INCLUDED_ALLOCATOR_TRAITS_DOT_H
#define INCLUDED_ALLOCATOR_TRAITS_DOT_H

#include <xstd.h>

BEGIN_NAMESPACE_XSTD

concept Allocator<typename Alloc>
{
    typedef object-type value_type;

    typedef pointer-like-type pointer;
    typedef pointer-like-type const_pointer;

    typedef pointer-like-type void_pointer;
    typedef pointer-like-type const_void_pointer;

    typedef integer-type difference_type;
    typedef integer-type size_type;

    template <typename T> using rebind_type = rebind-template;

    // Required constructor
    template <typename T>
      Alloc::Alloc(const rebind_type<T>& other);

    // Allocator propagation on construction
    static Alloc select_on_container_copy_construction(const Alloc& rhs);

    // Allocator propagation functions.  Return true if *this was modified.
    bool on_container_copy_assignment(const Alloc& rhs);
    bool on_container_move_assignment(Alloc&& rhs);
    bool on_container_swap(Alloc& other);

    // Allocate a single object
    pointer Alloc::allocate();
    pointer Alloc::allocate(const_void_pointer hint);
    // Optional for contiguous allocators:
    pointer Alloc::allocate(size_type n);
    pointer Alloc::allocate(size_type n, const_void_pointer hint);

    void Alloc::deallocate(pointer p);
    // Optional for contiguous allocators:
    void Alloc::deallocate(pointer p, size_type n);

    template <typename T, typename... Args>
      void Alloc::construct(T* p, Args&&... args);

    template <typename T>
      void Alloc::destroy(T* p);

    size_type Alloc::max_size() const;

    pointer Alloc::address(value_type& r) const;
    const_pointer Alloc::address(const value_type& r) const;

    bool operator==(const Alloc& a, const Alloc& b);
    bool operator!=(const Alloc& a, const Alloc& b);
};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_TRAITS_DOT_H)

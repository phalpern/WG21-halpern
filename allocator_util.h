/* allocator_util.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_ALLOCATOR_UTIL_DOT_H
#define INCLUDED_ALLOCATOR_UTIL_DOT_H

#ifndef INCLUDED_ALLOCATOR_TRAITS_DOT_H
#include <allocator_traits.h>
#endif

BEGIN_NAMESPACE_XSTD

namespace __details {

// Use inheritence to take advantage of the Empty Base Optimization (EBO).
template <typename Alloc, typename Tp>
class __allocator_wrapper : public Alloc
{
    Tp data_;

public:
//     template <typename A, typename... Args>
//     explicit __allocator_wrapper(const A& a, Args&&... args)
// 	: Alloc(a), data_(std::forward<Args>(args)...) { }
    template <typename A, typename... Args>
    explicit __allocator_wrapper(A&& a, Args&&... args)
	: Alloc(std::forward<A>(a)), data_(std::forward<Args>(args)...) { }

    Tp& data() { return data_; }
    const Tp& data() const { return data_; }

    Alloc& allocator()             { return *this; }
    const Alloc& allocator() const { return *this; }

    typedef Alloc allocator_type;
#if 0 // Unneeded
    typedef allocator_traits<Alloc> traits;

    typedef typename traits::pointer	  pointer;
    typedef typename traits::const_pointer const_pointer;

    typedef typename traits::difference_type difference_type;
    typedef typename traits::size_type       size_type;

    pointer allocate(size_type n)
	{ return traits::allocate(allocator(), n); }
    pointer allocate(size_type n, const_pointer hint)
	{ return traits::allocate(allocator(), n, hint); }

    void deallocate(pointer p, size_type n)
	{ traits::deallocate(allocator(), p, n); }

    template <typename T, typename... Args>
      void construct(T* p, Args&&... args)
	{ traits::construct(allocator(), p, XSTD::forward<Args>(args)...);}
    template <typename T>
      void destroy(T* p)
	{ traits::destroy(allocator(), p); }

    size_type max_size() const
	{ return traits::max_size(allocator()); }

//     template <typename T> T&       dereference(pointer p)
// 	{ return traits::dereference(p); }
//     template <typename T> const T& dereference(const_pointer p)
// 	{ return traits::dereference(p); }
//     template <typename T> T*       raw_pointer(pointer p)
// 	{ return traits::raw_pointer(p); }
//     template <typename T> const T* raw_pointer(const_pointer p)
// 	{ return traits::raw_pointer(p); }

    template <typename T> pointer address(T& r) const
	{ return traits::address(allocator(), r); }
    template <typename T> const_pointer address(const T& r) const
	{ return traits::address(allocator(), r); }

    Alloc select_on_container_copy_construction() const
	{ return traits::select_on_container_copy_construction(allocator()); }
    void do_on_container_copy_assignment(const Alloc& from)
	{ traits::do_on_container_copy_assignment(allocator(), from); }
    void do_on_container_move_assignment(Alloc&& from)
	{ traits::do_on_container_move_assignment(allocator(), from); }
    void do_on_container_swap(Alloc& other)
	{ traits::do_on_container_swap(allocator(), other); }
#endif // Unneeded
}; // end class __allocator_wrapper

} // end namespace __details

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_UTIL_DOT_H)

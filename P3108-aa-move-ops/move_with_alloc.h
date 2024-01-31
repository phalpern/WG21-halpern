/* move_with_alloc.h                                                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P3091: Better lookups for `map` and `unordered_map`
 */

#ifndef INCLUDED_MOVE_WITH_ALLOC
#define INCLUDED_MOVE_WITH_ALLOC

#include <utility>
#include <concepts>

namespace std::experimental {

template <typename Tp>
concept _AllocatorAware = requires (const Tp& v) {
  typename Tp::allocator_type;
  {v.get_allocator()} -> std::convertible_to<typename Tp::allocator_type>;
};

template <_AllocatorAware Tp>
Tp move_construct_with_allocator(Tp& from, const typename Tp::allocator_type& alloc)
{
  if (from.get_allocator() == alloc)
    return std::move(from);
  else
    return make_obj_using_allocator<Tp>(alloc, from);  // invoke extended copy constructor
}

}  // close std::experimental

#endif // ! defined(INCLUDED_MOVE_WITH_ALLOC)

// Local Variables:
// c-basic-offset: 2
// End:

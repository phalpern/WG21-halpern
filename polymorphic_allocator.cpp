/* polymorphic_allocator.cpp                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <polymorphic_allocator.h>
#include <mutex>

BEGIN_NAMESPACE_XSTD

atomic<polyalloc::allocator_resource *>
polyalloc::allocator_resource::s_default_resource(nullptr);

polyalloc::allocator_resource *
polyalloc::allocator_resource::init_default_resource()
{
    // If no default resource has been set, then set 's_default_resource' to
    // the default default resource == the new/delete allocator.
    static once_flag init_once;
    static allocator_resource *default_resource_p;
    [&]{ int x; }();
    call_once(init_once, [&]{
            static
                polymorphic_resource_adaptor<allocator<char>> default_resource;
            default_resource_p = &default_resource;
        });
    allocator_resource *ret = simple_resource_p;
    s_default_resource.compare_exchange_strong(ret, nullptr);
    return ret;
}

END_NAMESPACE_XSTD

// end polymorphic_allocator.cpp

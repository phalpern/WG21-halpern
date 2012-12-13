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
    static once_flag init_once;
    static allocator_resource *default_resource_p;

    // Initialize the default default resource == the new/delete allocator.
    auto do_init = [&]{
        static resource_adaptor_imp<allocator<char>> default_resource;
        default_resource_p = &default_resource;
    };

    // call_once(init_once, do_init);  // Crashes in gcc 4.6.3
    if (! default_resource_p)
        do_init();

    // If no default resource has been set, then set 's_default_resource' to
    // the default default resource == the new/delete allocator.  If the
    // default resource has already been set, then this operation is a noop.
    allocator_resource *exp = nullptr;
    s_default_resource.compare_exchange_strong(exp, default_resource_p);

    // return the default default resource == the new/delete allocator.
    return default_resource_p;
}

END_NAMESPACE_XSTD

// end polymorphic_allocator.cpp

/* allocator_traits.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_REBIND_DOT_H
#define INCLUDED_REBIND_DOT_H

#include <xstd.h>

BEGIN_NAMESPACE_XSTD

template <class T, class U>
struct rebinder;

template <class T, class ...TN, class U, template <class, class ...> class Sp>
struct rebinder<Sp<T, TN...>, U>
{
   typedef Sp<U, TN...> type;

};

template <class T, class U>
struct rebinder<T*, U>
{
   typedef U* type;
};

#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
template <class T, class U> using rebind = typename rebinder<T,U>::type;
#endif

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_REBIND_DOT_H)

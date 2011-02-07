/* pointer_traits.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_POINTER_TRAITS_DOT_H
#define INCLUDED_POINTER_TRAITS_DOT_H

#include <xstd.h>
#include <memory_util.h>
#include <cstddef>

BEGIN_NAMESPACE_XSTD

namespace __details {
    template <typename _Tp> struct __first_param;

    template <template <typename, typename...> class _Tmplt, typename _Tp,
              typename... _Args>
    struct __first_param<_Tmplt<_Tp, _Args...> > {
        typedef _Tp type;
    };

    template <typename _Ptr, typename _Tp>
    struct __ptr_has_rebind {

        template <typename _X>
        static char test(int, typename _X::template rebind<_Tp>::other*);

        template <typename _X>
        static int test(_LowPriorityConversion<int>, void*);

        static const bool value = (1 == sizeof(test<_Ptr>(0, 0)));
    };

    // Implementation of pointer_traits<_Ptr>::rebind if _Ptr has
    // its own rebind template.
    template <typename _Ptr, typename _U,
              bool _HasRebind = __ptr_has_rebind<_Ptr,_U>::value >
    struct __ptr_rebinder
    {
        typedef typename _Ptr::template rebind<_U>::other other;
    };

    // Specialization of pointer_traits<_Ptr>::rebind if _Ptr does not
    // have its own rebind template but has a the form _Ptr<class T,
    // OtherArgs>, where OtherArgs comprises zero or more type parameters.
    // Many pointers fit this form, hence many pointers will get a
    // reasonable default for rebind.
    template <template <class, class...> class _Ptr, typename _Tp, class... _Tn,
              class _U>
    struct __ptr_rebinder<_Ptr<_Tp, _Tn...>,_U,false>
    {
        typedef _Ptr<_U, _Tn...> other;
    };

    _DEFAULT_TYPE_TMPLT(element_type);
    _DEFAULT_TYPE_TMPLT(difference_type);
} // end namespace __details

template <typename _Ptr> struct pointer_traits
{
    typedef _Ptr                                               pointer;
    typedef _DEFAULT_TYPE(_Ptr,element_type,
                          typename __details::__first_param<_Ptr>::type)
                                                               element_type;
    typedef _DEFAULT_TYPE(_Ptr,difference_type,std::ptrdiff_t) difference_type;

#ifdef TEMPLATE_ALIASES
    template <class _U> using rebind =
        typename __details::__ptr_rebinder<_Ptr,_U>::other;
#else
    template <class _U> struct rebind {
        typedef typename __details::__ptr_rebinder<_Ptr,_U>::other other;
    };
#endif

    static
    pointer pointer_to(typename __details::__unvoid<element_type>::type& r)
        { return _Ptr::pointer_to(r); }
};

template <typename _Tp>
struct pointer_traits<_Tp*>
{
    typedef _Tp            element_type;
    typedef _Tp*           pointer;
    typedef std::ptrdiff_t difference_type;

#ifdef TEMPLATE_ALIASES
    template <class _U> using rebind = _U*;
#else
    template <class _U> struct rebind {
        typedef _U* other;
    };
#endif

    static
    pointer pointer_to(typename __details::__unvoid<element_type>::type& r)
        { return xstd::addressof(r); }
};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_POINTER_TRAITS_DOT_H)

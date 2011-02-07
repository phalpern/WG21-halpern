/* allocator_traits.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_ALLOCATOR_TRAITS_DOT_H
#define INCLUDED_ALLOCATOR_TRAITS_DOT_H

#include <xstd.h>
#include <type_traits>

BEGIN_NAMESPACE_XSTD

template <typename _Tp>
inline _Tp* addressof(_Tp& __obj) {
    return static_cast<_Tp*>(
	static_cast<void*>(
	    const_cast<char*>(&reinterpret_cast<const char&>(__obj))));
}

template <typename _Tp>
inline _Tp* addressof(_Tp&& __obj) {
    return static_cast<_Tp*>(
	static_cast<void*>(
	    const_cast<char*>(&reinterpret_cast<const char&>(__obj))));
}

static struct allocator_arg_t { } const allocator_arg = { };

template <typename T>
inline T* pointer_rebind(void* p) { return static_cast<T*>(p); }

template <typename T>
inline const T* pointer_rebind(const void* p)
    { return static_cast<const T*>(p); }

namespace __details
{

template <typename _Tp>
struct _LowPriorityConversion
{
    // Convertible from _Tp with user-defined-conversion rank.
    _LowPriorityConversion(const _Tp&) { }
};

template<typename _Tp, typename _Alloc>
struct __uses_allocator_imp
{
    // Use SFINAE (Substitution Failure Is Not An Error) to detect the
    // presence of an 'allocator_type' nested type convertilble from _Alloc.

  private:
    template <typename _Up>
    static char __test(int, typename _Up::allocator_type);
        // Match this function if _TypeT::allocator_type exists and is
        // implicitly convertible from _Alloc

    template <typename _Up>
    static int __test(_LowPriorityConversion<int>,
		      _LowPriorityConversion<_Alloc>);
        // Match this function if _TypeT::allocator_type does not exist or is
        // not convertible from _Alloc.

    static _Alloc __alloc;  // Declared but not defined

  public:
    enum { value = sizeof(__test<_Tp>(0, __alloc)) == 1 };
};

#define _DEFAULT_TYPE_TMPLT(tname)                                            \
    template <typename _Tp, typename _Default>                                \
    struct __default_type_ ## tname {                                         \
                                                                              \
        template <typename _X>                                                \
        static char test(int, typename _X::tname*);                           \
                                                                              \
        template <typename _X>                                                \
        static int test(_LowPriorityConversion<int>, void*);                  \
                                                                              \
        struct _DefaultWrap { typedef _Default tname; };                      \
                                                                              \
        static const bool value = (1 == sizeof(test<_Tp>(0, 0)));             \
                                                                              \
        typedef typename                                                      \
            std::conditional<value,_Tp,_DefaultWrap>::type::tname type;       \
    }

#define _DEFAULT_TYPE(T,tname,def) \
    typename __details::__default_type_ ## tname<T,def>::type

_DEFAULT_TYPE_TMPLT(pointer);
_DEFAULT_TYPE_TMPLT(const_pointer);
_DEFAULT_TYPE_TMPLT(void_pointer);
_DEFAULT_TYPE_TMPLT(const_void_pointer);
_DEFAULT_TYPE_TMPLT(difference_type);
_DEFAULT_TYPE_TMPLT(size_type);

#define _DEFAULT_FUNC_TMPLT(fname,doret,default_imp)                          \
    template <typename _Tp, typename... A>                                    \
    auto __default_func_has_ ## fname(int, _Tp& v, A&&... args) ->            \
        decltype((v.fname(std::forward<A>(args)...),                          \
                          std::true_type()));                                 \
                                                                              \
    template <typename _Tp, typename... A>                                    \
    auto __default_func_has_ ## fname(_LowPriorityConversion<int>,            \
                                      _Tp& v, A&&... args) ->                 \
        std::false_type;                                                      \
                                                                              \
    template <typename _Ret, typename _Tp, typename A1, typename... Args>     \
    static _Ret __default_func_dispatch_ ## fname(std::true_type,             \
                                                  _Tp& v, A1&& a1,            \
                                                  Args&&... args) {           \
        doret v.fname(std::forward<A1>(a1), std::forward<Args>(args)...);     \
    }                                                                         \
                                                                              \
    template <typename _Ret, typename _Tp, typename A1, typename... Args>     \
    static _Ret __default_func_dispatch_ ## fname(std::false_type,            \
                                                  _Tp& v, A1&& a1,            \
                                                  Args&&... args) {           \
        default_imp                                                           \
    }                                                                         \
                                                                              \
    template <typename _Ret, typename _Tp, typename A1, typename... Args>     \
    static _Ret __default_func_call_ ## fname(_Tp& v, A1&& a1,                \
                                              Args&&... args) {               \
        decltype(__default_func_has_ ## fname(                                \
                     0, v, std::forward<A1>(a1),                              \
                     std::forward<Args>(args)...)) __flag;                    \
        doret __default_func_dispatch_ ## fname<_Ret>(__flag, v,              \
                                                 std::forward<A1>(a1),        \
                                                 std::forward<Args>(args)...);\
    }

#define _DEFAULT_FUNC(fname,R) __details::__default_func_call_ ## fname<R>

_DEFAULT_FUNC_TMPLT(allocate,return,{
        return v.allocate(a1);
    })

template <typename T, typename... A> inline void __call_ctor(T* p, A&&... a)
    { new ((void*) p) T(std::forward<A>(a)...); }
_DEFAULT_FUNC_TMPLT(construct,,{
        __call_ctor(a1, std::forward<Args>(args)...);
    })

template <typename T> inline void __call_dtor(T* p) { p->~T(); }
_DEFAULT_FUNC_TMPLT(destroy,,{ __call_dtor(a1); })

_DEFAULT_FUNC_TMPLT(max_size,return,{ return ~_Ret(0); })

_DEFAULT_FUNC_TMPLT(address,return,{ return addressof(a1); });

_DEFAULT_FUNC_TMPLT(select_on_container_copy_construction,return,{
        return v;
    })

_DEFAULT_FUNC_TMPLT(on_container_copy_assignment,return,{ return false; })
_DEFAULT_FUNC_TMPLT(on_container_move_assignment,return,{ return false; })
_DEFAULT_FUNC_TMPLT(on_container_swap,return,{ return false; })

} // end namespace __details

template<typename _Tp, typename _Alloc>
struct uses_allocator
    : std::integral_constant<bool,
			__details::__uses_allocator_imp<_Tp, _Alloc>::value>
{
    // Automatically detects if there exists a type _Tp::allocator_type
    // that is convertible from _Alloc.  Specialize for types that
    // don't have a nested allocator_type but which are none-the-less
    // constructible with _Alloc (e.g., using type erasure).
};

template <typename Alloc>
struct allocator_traits
{
    typedef Alloc allocator_type;

    typedef typename Alloc::value_type         value_type;

    typedef _DEFAULT_TYPE(Alloc,pointer,value_type*)             pointer;
    typedef _DEFAULT_TYPE(Alloc,const_pointer,const value_type*) const_pointer;
    typedef _DEFAULT_TYPE(Alloc,void_pointer,void*)              void_pointer;
    typedef _DEFAULT_TYPE(Alloc,const_void_pointer,const void*)  const_void_pointer;

    typedef _DEFAULT_TYPE(Alloc,difference_type,std::ptrdiff_t) difference_type;
    typedef _DEFAULT_TYPE(Alloc,size_type,std::size_t)          size_type;

#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
    template <typename T> using rebind_alloc =
        typename Alloc::template rebind<T>::other;
    template <typename T> using rebind_traits =
        allocator_traits<rebind_alloc<T> >;
#else // !TEMPLATE_ALIASES
    template <typename T>
    struct rebind_alloc : Alloc::template rebind<T>::other
    {
	typedef typename Alloc::template rebind<T>::other _Base;

        template <typename... Args>
	rebind_alloc(Args&&... args) : _Base(std::forward<Args>(args)...) { }
    };

    template <typename T>
    struct rebind_traits :
	allocator_traits<typename Alloc::template rebind<T>::other>
    {
    };
#endif // !TEMPLATE_ALIASES

    static pointer allocate(Alloc& a, size_type n)
        { return a.allocate(n); }
    static pointer allocate(Alloc& a, size_type n, const_void_pointer hint)
        { return _DEFAULT_FUNC(allocate,pointer)(a, n, hint); }

    static void deallocate(Alloc& a, pointer p, size_type n)
        { a.deallocate(p, n); }

    template <typename T, typename... Args>
    static void construct(Alloc& a, T* p, Args&&... args) {
        _DEFAULT_FUNC(construct,void)(a, p, std::forward<Args>(args)...);
    }

    template <typename T>
    static void destroy(Alloc& a, T* p) {
        _DEFAULT_FUNC(destroy,void)(a, p);
    }

    static size_type max_size(const Alloc& a)
        { _DEFAULT_FUNC(max_size,size_type)(a, 0); }

    static pointer address(const Alloc& a, value_type& r)
        { _DEFAULT_FUNC(address,pointer)(a, r); }
    static const_pointer address(const Alloc& a, const value_type& r)
        { _DEFAULT_FUNC(address,const_pointer)(a, r); }

    // Allocator propagation on construction
    static Alloc select_on_container_copy_construction(const Alloc& rhs) {
        return _DEFAULT_FUNC(select_on_container_copy_construction,Alloc)(rhs,rhs);
    }

    // Allocator propagation on assignment and swap.
    // Return true if lhs is modified.
    static bool on_container_copy_assignment(Alloc& lhs, const Alloc& rhs)
        { return _DEFAULT_FUNC(on_container_copy_assignment,bool)(lhs,rhs); }
    static bool on_container_move_assignment(Alloc& lhs, Alloc&& rhs)
        { return _DEFAULT_FUNC(on_container_move_assignment,bool)(lhs,std::move(rhs)); }
    static bool on_container_swap(Alloc& lhs, Alloc& rhs)
        { return _DEFAULT_FUNC(on_container_swap,bool)(lhs,rhs); }
};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_TRAITS_DOT_H)

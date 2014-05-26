// destructive_move.h                  -*-C++-*-
//
// Copyright 2014 Pablo Halpern.
// Free to use and redistribute (See accompanying file license.txt.)

#ifndef INCLUDED_DESTRUCTIVE_MOVE
#define INCLUDED_DESTRUCTIVE_MOVE

#include <type_traits>
#include <utility>
#include <new>
#include <cstring>

namespace std {

// Not implemented in g++ 4.8:
template <class T> struct is_trivially_move_constructible : is_trivial<T> { };

namespace experimental {

template <class T> struct is_trivially_destructive_movable;
template <class T> struct is_nothrow_destructive_movable;

template <class T>
inline
typename enable_if<is_trivially_destructive_movable<T>::value>::type
destructive_move(T* to, T* from) noexcept
{
    // Use bitwise copy
    memcpy(to, from, sizeof(T));
}

template <class T>
inline
typename enable_if<! is_trivially_destructive_movable<T>::value>::type
destructive_move(T* to, T* from)
    noexcept(is_nothrow_move_constructible<T>::value &&
             is_nothrow_destructible<T>::value)
{
    ::new((void*) to) T(std::move(*from));
    from->~T();
}

template <class T>
inline
typename enable_if<is_trivially_destructive_movable<T>::value>::type
destructive_move_array(T* to, T* from, size_t sz) noexcept
{
    // Bitwise move entire array
    memcpy(to, from, sz * sizeof(T));
}

template <class T>
inline
typename enable_if<(! is_trivially_destructive_movable<T>::value &&
           is_nothrow_destructive_movable<T>::value)>::type
destructive_move_array(T* to, T* from, size_t sz) noexcept
{
    // Destructively move each element (will not throw)
    for (size_t i = 0; i < sz; ++i)
        destructive_move(&to[i], &from[i]);
}        

template <class T>
inline
typename enable_if<! is_nothrow_destructive_movable<T>::value>::type
destructive_move_array(T* to, T* from, size_t sz) noexcept(false)
{
    size_t i;
    try {
        // Copy-construct each element (might throw)
        for (i = 0; i < sz; ++i)
            ::new((void*) &to[i]) T(from[i]);
    }
    catch (...) {
        // Exception thrown, unwind constructions
        while (i > 0)
            to[--i].~T();
        throw;
    }
        
    // Destroy 'from' array
    for (i = 0; i < sz; ++i)
        from[i].~T();
}

template <class T>
struct is_trivially_destructive_movable :
        integral_constant<bool, (is_trivially_move_constructible<T>::value &&
                                 is_trivially_destructible<T>::value)>
{
};

template <class T>
struct is_nothrow_destructive_movable :
        integral_constant<bool, noexcept(destructive_move((T*) nullptr,
                                                          (T*) nullptr))>
{
};

} // end namespace std
} // namespace experimental

#endif // ! defined(INCLUDED_DESTRUCTIVE_MOVE)

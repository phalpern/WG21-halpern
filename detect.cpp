/* detect.cpp                  -*-C++-*-
 *
 * Copyright (C) 2009 Halpern-Wight Software, Inc. All rights reserved.
 */

#include <iostream>
#include <string>
#include <utility>

struct true_type  { static const bool value = true;  };
struct false_type { static const bool value = false; };
struct cvt { cvt(int) { } };

std::ostream& operator<<(std::ostream& os, true_type) { return os << "true"; }
std::ostream& operator<<(std::ostream& os, false_type) { return os << "false"; }

template <typename T, typename... Args>
auto test_ctor(int, Args&&... args) -> decltype((T(args...), true_type()))
  { return true_type(); }

template <typename T, typename... Args>
auto test_ctor(cvt, Args&&...) -> false_type
  { return false_type(); }

template <typename T> T make_v();

template <typename T, typename... Args>
struct has_constructor_imp;

template <typename T>
struct has_constructor_imp<T>
{
    template <typename... Head>
    static auto f(int, Head&&... head) -> decltype(T(std::forward<Head>(head)...), true_type());

    template <typename... Head>
    static auto f(cvt, Head&&... head) -> false_type;
};

template <typename T, typename A1, typename... Tail>
struct has_constructor_imp<T, A1, Tail...>
{
    typedef has_constructor_imp<T,Tail...> Nest;

    template <typename... Head>
    static auto f(int,Head&&... head) ->
	decltype(Nest::f(0,std::forward<Head>(head)..., make_v<A1>()));
};

template <typename T, typename... Args>
struct has_constructor_imp2
{
    typedef decltype(has_constructor_imp<T, Args...>::f(0)) type;
};

template <typename T, typename... Args>
struct has_constructor : has_constructor_imp2<T,Args...>::type
{
};

int main()
{
    std::cout << test_ctor<int>(0,(short)0) << std::endl;
    std::cout << test_ctor<int>(0,"hello") << std::endl;
    std::cout << test_ctor<int>(0,std::cout) << std::endl;
    std::cout << test_ctor<std::string>(0,"hello") << std::endl;
    std::cout << test_ctor<std::string>(0,3,"hello") << std::endl;
    std::cout << test_ctor<std::string>(0,"hello",3) << std::endl;
    std::cout << test_ctor<std::string>(0,std::cout) << std::endl;
    std::cout << std::endl;

//     std::cout << has_constructor<int>() << std::endl;
//     std::cout << has_constructor<int,short>() << std::endl;
//     std::cout << has_constructor<int,short,short>() << std::endl;
//     std::cout << has_constructor<int,std::string>() << std::endl;

//     std::cout << has_constructor<std::string,const char*>() << std::endl;
//     std::cout << has_constructor<std::string,const char*,int&&>() << std::endl;
//     std::cout << has_constructor<std::string,std::ostream&>() << std::endl;

//     std::cout << has_constructor<std::ostream&,std::ostream&>() << std::endl;
}


/* End detect.cpp */

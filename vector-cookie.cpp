/* vector-cookie.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <iostream>
#include <utility>

namespace std {
namespace experimental {
namespace parallel {
inline namespace v2 {

struct sequential_execution_policy
{
    struct cookie_type { };
    cookie_type make_cookie() const { return cookie_type{}; }
};

struct vector_execution_policy
{
    struct cookie_type {
        template <typename F>
        auto vec_off(F&& f) const -> decltype(f());

        template <class T> class ordered_update_t;

        template <class T>
        ordered_update_t<T> ordered_update(T& ref) const;
    };
    cookie_type make_cookie() const { return cookie_type{}; }
};

template <typename F>
inline
auto vector_execution_policy::cookie_type::vec_off(F&& f) const -> decltype(f())
{
    return std::forward<F>(f)();
}

template<class T>
class vector_execution_policy::cookie_type::ordered_update_t {
  typedef vector_execution_policy::cookie_type cookie_type;
  cookie_type* cookie;
  T& ref;
public:
  ordered_update_t(cookie_type* c, T& loc) : cookie(c), ref(loc) { }
  template <class U>
  auto operator=(U rhs) { return cookie->vec_off([&]{ return ref=std::move(rhs); }); }
  template <class U>
    auto operator+=(U rhs){ return cookie->vec_off([&]{return ref+=std::move(rhs);}); }
  template <class U>
    auto operator-=(U rhs){ return cookie->vec_off([&]{return ref-=std::move(rhs);}); }
  template <class U>
    auto operator*=(U rhs){ return cookie->vec_off([&]{return ref*=std::move(rhs);}); }
  template <class U>
    auto operator/=(U rhs){ return cookie->vec_off([&]{return ref/=std::move(rhs);}); }
  template <class U>
    auto operator%=(U rhs){ return cookie->vec_off([&]{return ref%=std::move(rhs);}); }
  template <class U>
    auto operator>>=(U rhs){return cookie->vec_off([&]{return ref>>=std::move(rhs);});}
  template <class U>
    auto operator<<=(U rhs){return cookie->vec_off([&]{return ref<<=std::move(rhs);});}
  template <class U>
    auto operator&=(U rhs){ return cookie->vec_off([&]{return ref&=std::move(rhs);}); }
  template <class U>
    auto operator^=(U rhs){ return cookie->vec_off([&]{return ref^=std::move(rhs);}); }
  template <class U>
    auto operator|=(U rhs){ return cookie->vec_off([&]{return ref|=std::move(rhs);}); }
  auto operator++() { return cookie->vec_off([&]{ return ++ref; }); }
  auto operator++(int) { return cookie->vec_off([&]{ return ref++; }); }
  auto operator--() { return cookie->vec_off([&]{ return --ref; }); }
  auto operator--(int) { return cookie->vec_off([&]{ return ref--; }); }
};

template <class T>
inline
vector_execution_policy::cookie_type::ordered_update_t<T>
vector_execution_policy::cookie_type::ordered_update(T& ref) const
{
    return ordered_update_t<T>(this, ref);
}

struct unsequenced_execution_policy
{
    struct cookie_type { };
    cookie_type make_cookie() const { return cookie_type{}; }
};
struct parallel_execution_policy
{
    struct cookie_type { };
    cookie_type make_cookie() const { return cookie_type{}; }
};
struct parallel_unsequenced_execution_policy
{
    struct cookie_type { };
    cookie_type make_cookie() const { return cookie_type{}; }
};

constexpr sequential_execution_policy seq{};
constexpr vector_execution_policy vec{};
constexpr unsequenced_execution_policy unseq{};
constexpr parallel_execution_policy par{};
constexpr parallel_unsequenced_execution_policy par_unseq{};

template <typename ExPolicy, typename I, typename F>
void for_loop_imp(ExPolicy&& exec, decay_t<I> first, I last, F&& body, ...)
{
    for ( ; first != last; ++first)
        std::forward<F>(body)(first);
}

template <typename ExPolicy, typename I, typename F>
void for_loop_imp(ExPolicy&& exec, decay_t<I> first, I last, F&& body,
                  typename result_of<F(typename std::remove_cv_t<std::remove_reference_t<ExPolicy>>::cookie_type, I)>::type*)
{
    for ( ; first != last; ++first)
        std::forward<F>(body)(exec.make_cookie(), first);
}

template <typename ExPolicy, typename I, typename F>
void for_loop(ExPolicy&& exec, decay_t<I> first, I last, F&& body)
{
    for_loop_imp(std::forward<ExPolicy>(exec), first, last,
                 std::forward<F>(body), nullptr);
}

} // close namespace v2
} // close namespace parallel
} // close namespace experimental
} // close namespace std

int main()
{
    using namespace std::experimental;
    parallel::for_loop(parallel::vec, 0, 3, [&](int i){
            std::cout << "no cookie " << i << std::endl;
        });
    parallel::for_loop(parallel::vec, 3, 6, [&](auto cookie, int i){
            cookie.vec_off([&]{ std::cout << "cookie " << i << std::endl; });
        });
}

/* End vector-cookie.cpp */

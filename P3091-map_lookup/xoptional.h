/* xoptional.h                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_XOPTIONAL
#define INCLUDED_XOPTIONAL

#include <optional>

namespace std::experimental
{

// Implement trait from P2255R2
template <class T, class U>
struct reference_constructs_from_temporary :
  conjunction<disjunction<conjunction<is_lvalue_reference<T>,
                                      is_const<remove_reference_t<T>>>,
                          is_rvalue_reference<T>>,
              negation<is_reference<U>>,
              is_constructible<T, U>>
{
};

template <class T, class U>
inline constexpr bool reference_constructs_from_temporary_v =
  reference_constructs_from_temporary<T, U>::value;

// Get first type in a pack consisting of exactly one type.
template <class A0> struct __pack0 { using type = A0; };
template <class... Args> using __pack0_t = typename __pack0<Args...>::type;

template <class T>
class optional : public std::optional<T>
{
public:
  using std::optional<T>::optional;

  template <class U = T, class... Args>
  U or_construct(Args&&... args)
  {
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, Args...>,
                  "Cannot construct return value from arguments");
    static_assert(! is_lvalue_reference_v<U> || 1 == sizeof...(Args),
                  "Reference return type allows only one argument");
    if constexpr (is_lvalue_reference_v<U> && 1 == sizeof...(Args))
    {
      static_assert(is_convertible_v<add_pointer_t<__pack0_t<Args...>>,
                                    add_pointer_t<U>>,
                    "Cannot convert argument to return reference");
      static_assert(is_convertible_v<add_pointer_t<T>, add_pointer_t<U>>,
                    "Cannot convert value_type to return reference");
    }

    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
  }

  template <class U = T, class... Args>
  U or_construct(Args&&... args) const
  {
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, Args...>,
                  "Cannot construct return value from arguments");
    static_assert(! is_lvalue_reference_v<U> || 1 == sizeof...(Args),
                  "Reference return type allows only one argument");
    if constexpr (is_lvalue_reference_v<U> && 1 == sizeof...(Args))
    {
      static_assert(is_convertible_v<add_pointer_t<__pack0_t<Args...>>,
                                    add_pointer_t<U>>,
                    "Argument type and return reference are not compatible");
      static_assert(is_convertible_v<add_pointer_t<T>, add_pointer_t<U>>,
                    "value_type and return reference are not compatible");
    }

    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U or_construct(initializer_list<X> il, Args&&... args)
  {
    static_assert(! is_reference_v<U>,
                  "Reference return type does not permit initializer list");
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, initializer_list<X>, Args...>,
                  "Cannot construct return value from arguments");

    return this->has_value() ? U(**this) : U(il, std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U or_construct(initializer_list<X> il, Args&&... args) const
  {
    static_assert(! is_reference_v<U>,
                  "Reference return type does not permit initializer list");
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, initializer_list<X>, Args...>,
                  "Cannot construct return value from arguments");

    return this->has_value() ? U(**this) : U(il, std::forward<Args>(args)...);
  }

  // TBD: All monadic operations need to be re-implemented here, as the
  // inherited ones mandate functors that return `std::optional` instead of
  // `std::experimental::optional`.
};

template <class T> struct __is_optional_ref               : false_type {};
template <class T> struct __is_optional_ref<optional<T&>> : true_type {};

template <class T>
class optional<T&>
{
  T* m_val = nullptr;

public:
  using value_type = T&;

  // ?.?.1.2, constructors
  constexpr optional() noexcept = default;
  constexpr optional(nullopt_t) noexcept { }
  constexpr optional(const optional&) = default;
  constexpr optional(optional&&) noexcept = default;
  template<class U> requires (! __is_optional_ref<remove_cvref_t<U>>::value)
  constexpr optional(U&& v) : m_val(addressof(v))
    { static_assert(is_lvalue_reference_v<U>); }
  template<class U> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional(const optional<U&>& v)
    { if (v.has_value()) m_val = addressof(*v); }

  // ?.?.1.3, destructor
  constexpr ~optional() = default;

  // ?.?.1.4, assignment
  constexpr optional& operator=(nullopt_t) noexcept { m_val = nullptr; }
  constexpr optional& operator=(const optional&) noexcept = default;
  constexpr optional& operator=(optional&&) noexcept = default;
  template<class U = T> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional& operator=(U&& v) {
    static_assert(is_lvalue_reference_v<U>);
    m_val = addressof(v);
    return *this;
  }
  template<class U> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional& operator=(const optional<U>& v) {
    static_assert(is_lvalue_reference_v<U>);
    m_val = v.has_value() ? addressof(*v) : nullptr;
  }
  template<class U> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr T& emplace(U&& v) { return *this = v; }

  // ?.?.1.5, swap
  constexpr void swap(optional& rhs) noexcept
    { std::swap(m_val, rhs.m_val); }

  // ?.?.1.6, observers
  constexpr T* operator->() const noexcept { return m_val; }
  constexpr T& operator*() const noexcept  { return *m_val; }
  constexpr explicit operator bool() const noexcept { return m_val; }
  constexpr bool has_value() const noexcept { return m_val; }
  constexpr T& value() const {
    if (m_val)
      return *m_val;
    throw bad_optional_access();
  }
#ifdef P2988R3_value_or
  template<class U>
  constexpr T& value_or(U&& v) const {
    static_assert(is_lvalue_reference_v<U>,
                  "value_or argument must be an lvalue");
    return m_val ? *m_val : static_cast<T&>(v);
  }
#else
  template<class U>
  constexpr auto value_or(U&& v) const -> decltype(auto) {
    // static_assert(is_lvalue_reference_v<U>,
    //               "value_or argument must be an lvalue");
    using result = common_reference_t<T&, U&>;
    static_assert(is_lvalue_reference_v<result>,
                  "Argument and value_type must have a common reference type");
    static_assert(!reference_constructs_from_temporary_v<result, U>,
                  "Return-reference construction would dangle");
    static_assert(is_convertible_v<add_pointer_t<T>, add_pointer_t<result>>,
                  "value_type and return reference are not compatible");
    static_assert(is_convertible_v<add_pointer_t<U>, add_pointer_t<result>>,
                  "Argument type and return reference are not compatible");
    return m_val ? static_cast<result>(*m_val) : static_cast<result>(v);
  }
#endif

  template <class U = remove_cvref_t<T>, class... Args>
  U or_construct(Args&&... args) const
  {
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, Args...>,
                  "Cannot construct return value from arguments");
    static_assert(! is_lvalue_reference_v<U> || 1 == sizeof...(Args),
                  "Reference return type allows only one argument");
    if constexpr (is_lvalue_reference_v<U> && 1 == sizeof...(Args))
    {
      static_assert(is_lvalue_reference_v<__pack0_t<Args...>>,
                    "Argument must be an lvalue");
      static_assert(is_convertible_v<add_pointer_t<__pack0_t<Args...>>,
                                    add_pointer_t<U>>,
                    "Cannot convert argument to return reference");
      static_assert(is_convertible_v<add_pointer_t<T>, add_pointer_t<U>>,
                    "Cannot convert value_type to return reference");
    }

    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U or_construct(initializer_list<X> il, Args&&... args) const
  {
    static_assert(! is_reference_v<U>,
                  "Reference return type does not permit initializer list");
    static_assert(is_constructible_v<U, decltype(**this)>,
                  "Cannot construct return value from value_type");
    static_assert(is_constructible_v<U, initializer_list<X>, Args...>,
                  "Cannot construct return value from arguments");

    return this->has_value() ? U(**this) : U(il, std::forward<Args>(args)...);
  }

  // ?.?.1.7, monadic operations
  template<class F> constexpr auto and_then(F&& f) const {
    using U = invoke_result_t<F, T&>;
    return *m_val ? invoke(std::forward<F>(f), *m_val) : U(nullopt);
  }
  template<class F> constexpr optional or_else(F&& f) const {
    using U = invoke_result_t<F, T&>;
    return *m_val ? U(nullopt) : invoke(std::forward<F>(f), *m_val);
  }
  template<class F> constexpr auto transform(F&& f) const {
    using U = remove_cv_t<invoke_result_t<F, T&>>;
    using R = optional<U>;
    return *m_val ? R(invoke(std::forward<F>(f), *m_val)) : R(nullopt);
  }

  // ?.?.1.8, modifiers
  constexpr void reset() noexcept { m_val = nullptr; }
};

template<class T, class U>
constexpr bool operator==(const optional<T>& x, const optional<U>& y)
{
  if (x.has_value() != y.has_value())
    return false;
  else if (! x.has_value())
    return true;  // Both not engaged
  else
    return *x == *y;
}

template<class T, class U>
constexpr bool operator<(const optional<T>& x, const optional<U>& y)
{
  if (! y.has_value())
    return false;
  else if (! x.has_value())
    return true;
  else
    return *x < *y;
}

template<class T, class U>
constexpr bool operator>(const optional<T>& x, const optional<U>& y)
{
  return y < x;
}

template<class T, class U>
constexpr bool operator<=(const optional<T>& x, const optional<U>& y)
{
  return ! (y < x);
}

template<class T, class U>
constexpr bool operator>=(const optional<T>& x, const optional<U>& y)
{
  return ! (x < y);
}

template<class T, class U>
constexpr auto operator<=>(const optional<T>& x, const optional<U>& y)
  -> decltype(*x <=> *y)
{
  if (x.has_value() && y.has_value())
    return *x <=> *y;
  else
    return x.has_value() <=> y.has_value();
}

} // close namespace std::experimental

#endif // ! defined(INCLUDED_XOPTIONAL)

// Local Variables:
// c-basic-offset: 2
// End:

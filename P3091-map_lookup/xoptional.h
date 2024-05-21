/* xoptional.h                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_XOPTIONAL
#define INCLUDED_XOPTIONAL

#include <optional>

namespace std::experimental {

// Approximately implementation of trait from P2255R2
template <class T, class U>
inline constexpr bool reference_constructs_from_temporary_v =
  is_reference_v<T> && is_constructible_v<T, U> &&
  ((is_lvalue_reference_v<T> && is_rvalue_reference_v<U&&>) ||
   (is_rvalue_reference_v<T> && !is_rvalue_reference_v<U&&>));

template <class T, class U>
struct reference_constructs_from_temporary
  : std::integral_constant<bool,
                           reference_constructs_from_temporary_v<T, U>> {};

// Get first type in a pack consisting of exactly one type.
template <class A0>
struct __pack0 {
  using type = A0;
};

template <class... Args>
using __pack0_t = typename __pack0<Args...>::type;

template <class U, class Opt, class... Args>
U value_or_imp(Opt&& obj, Args&&... args) {
  using DerefType = decltype(*std::forward<Opt>(obj));

  static_assert(is_constructible_v<U, DerefType>,
                "Cannot construct return value from value_type");
  static_assert(is_constructible_v<U, Args...>,
                "Cannot construct return value from arguments");
  static_assert(!is_lvalue_reference_v<U> || 1 == sizeof...(Args),
                "Reference return type allows only one argument");
  if constexpr (is_lvalue_reference_v<U> && 1 == sizeof...(Args)) {
    using Arg = __pack0_t<Args...>;
    static_assert(!reference_constructs_from_temporary_v<U, DerefType>,
                  "Would construct a dangling reference from a temporary");
    static_assert(!reference_constructs_from_temporary_v<U, Arg>,
                  "Would construct a dangling reference from a temporary");
  }

  return obj.has_value() ? U(*std::forward<Opt>(obj))
    : U(std::forward<Args>(args)...);
}

template <class T>
class optional : public std::optional<T> {
public:
  using std::optional<T>::optional;

#ifdef __cpp_explicit_this_parameter

  template <class U = remove_cvref_t<T>, class Self, class... Args>
  U value_or(this Self&& self, Args&&... args) {
    return value_or_imp<U>(std::forward<Self>(self),
                           std::forward<Args>(args)...);
  }

  template <class U = remove_cvref_t<T>, class Self, class X, class... Args>
  U value_or(this Self&& self, initializer_list<X> il, Args&&... args) {
    return value_or_imp<U>(std::forward<Self>(self),
                           il, std::forward<Args>(args)...);
  }

#else

  template <class U = remove_cvref_t<T>, class... Args>
  U value_or(Args&&... args) & {
    return value_or_imp<U>(*this, std::forward<Args>(args)...);
  }

  template <class U = remove_cvref_t<T>, class... Args>
  U value_or(Args&&... args) const& {
    return value_or_imp<U>(*this, std::forward<Args>(args)...);
  }

  template <class U = remove_cvref_t<T>, class... Args>
  U value_or(Args&&... args) && {
    return value_or_imp<U>(std::move(*this), std::forward<Args>(args)...);
  }

  template <class U = remove_cvref_t<T>, class... Args>
  U value_or(Args&&... args) const&& {
    return value_or_imp<U>(std::move(*this), std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U value_or(initializer_list<X> il, Args&&... args) & {
    return value_or_imp<U>(*this, il, std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U value_or(initializer_list<X> il, Args&&... args) const& {
    return value_or_imp<U>(*this, il, std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U value_or(initializer_list<X> il, Args&&... args) && {
    return value_or_imp<U>(std::move(*this), il,
                           std::forward<Args>(args)...);
  }

  template <class U = T, class X, class... Args>
  U value_or(initializer_list<X> il, Args&&... args) const&& {
    return value_or_imp<U>(std::move(*this), il,
                           std::forward<Args>(args)...);
  }

#endif

  // TBD: All monadic operations need to be re-implemented here, as the
  // inherited ones mandate functors that return `std::optional` instead of
  // `std::experimental::optional`.
};

template <class T>
struct __is_optional_ref : false_type {};
template <class T>
struct __is_optional_ref<optional<T&>> : true_type {};

template <class T>
class optional<T&> {
  T* m_val = nullptr;

public:
  using value_type = T&;

  // ?.?.1.2, constructors
  constexpr optional() noexcept = default;
  constexpr optional(nullopt_t) noexcept {}
  constexpr optional(const optional&) = default;
  constexpr optional(optional&&) noexcept = default;
  template <class U>
  requires(!__is_optional_ref<remove_cvref_t<U>>::value)
  constexpr optional(U&& v) : m_val(addressof(v)) {
    static_assert(is_lvalue_reference_v<U>);
  }
  template <class U>
  requires(!is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional(const optional<U&>& v) {
    if (v.has_value()) m_val = addressof(*v);
  }

  // ?.?.1.3, destructor
  constexpr ~optional() = default;

  // ?.?.1.4, assignment
  constexpr optional& operator=(nullopt_t) noexcept { m_val = nullptr; }
  constexpr optional& operator=(const optional&) noexcept = default;
  constexpr optional& operator=(optional&&) noexcept = default;
  template <class U = T>
  requires(!is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional& operator=(U&& v) {
    static_assert(is_lvalue_reference_v<U>);
    m_val = addressof(v);
    return *this;
  }
  template <class U>
  requires(!is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional& operator=(const optional<U>& v) {
    static_assert(is_lvalue_reference_v<U>);
    m_val = v.has_value() ? addressof(*v) : nullptr;
  }
  template <class U>
  requires(!is_same_v<remove_cvref_t<U>, optional>)
  constexpr T& emplace(U&& v) {
    return *this = v;
  }

  // ?.?.1.5, swap
  constexpr void swap(optional& rhs) noexcept { std::swap(m_val, rhs.m_val); }

  // ?.?.1.6, observers
  constexpr T* operator->() const noexcept { return m_val; }
  constexpr T& operator*() const noexcept { return *m_val; }
  constexpr explicit operator bool() const noexcept { return m_val; }
  constexpr bool has_value() const noexcept { return m_val; }
  constexpr T& value() const {
    if (m_val) return *m_val;
    throw bad_optional_access();
  }

  template <class U = remove_cvref_t<T>, class... Args>
  U value_or(Args&&... args) const {
    return value_or_imp<U>(*this, std::forward<Args>(args)...);
  }

  template <class U = remove_cvref_t<T>, class X, class... Args>
  U value_or(initializer_list<X> il, Args&&... args) const {
    return value_or_imp<U>(*this, il, std::forward<Args>(args)...);
  }

  // ?.?.1.7, monadic operations
  template <class F>
  constexpr auto and_then(F&& f) const {
    using U = invoke_result_t<F, T&>;
    return *m_val ? invoke(std::forward<F>(f), *m_val) : U(nullopt);
  }
  template <class F>
  constexpr optional or_else(F&& f) const {
    using U = invoke_result_t<F, T&>;
    return *m_val ? U(nullopt) : invoke(std::forward<F>(f), *m_val);
  }
  template <class F>
  constexpr auto transform(F&& f) const {
    using U = remove_cv_t<invoke_result_t<F, T&>>;
    using R = optional<U>;
    return *m_val ? R(invoke(std::forward<F>(f), *m_val)) : R(nullopt);
  }

  // ?.?.1.8, modifiers
  constexpr void reset() noexcept { m_val = nullptr; }
};

template <class T, class U>
constexpr bool operator==(const optional<T>& x, const optional<U>& y) {
  if (x.has_value() != y.has_value())
    return false;
  else if (!x.has_value())
    return true;  // Both not engaged
  else
    return *x == *y;
}

template <class T, class U>
constexpr bool operator<(const optional<T>& x, const optional<U>& y) {
  if (!y.has_value())
    return false;
  else if (!x.has_value())
    return true;
  else
    return *x < *y;
}

template <class T, class U>
constexpr bool operator>(const optional<T>& x, const optional<U>& y) {
  return y < x;
}

template <class T, class U>
constexpr bool operator<=(const optional<T>& x, const optional<U>& y) {
  return !(y < x);
}

template <class T, class U>
constexpr bool operator>=(const optional<T>& x, const optional<U>& y) {
  return !(x < y);
}

template <class T, class U>
constexpr auto operator<=>(const optional<T>& x,
                           const optional<U>& y) -> decltype(*x <=> *y) {
  if (x.has_value() && y.has_value())
    return *x <=> *y;
  else
    return x.has_value() <=> y.has_value();
}

}  // namespace std::experimental

#endif  // ! defined(INCLUDED_XOPTIONAL)

// Local Variables:
// c-basic-offset: 2
// End:

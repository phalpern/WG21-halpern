/* xoptional.h                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_XOPTIONAL
#define INCLUDED_XOPTIONAL

#include <optional>
#include <maybe.h>

namespace std::experimental {

template <class T>
class optional : public std::optional<T> {
public:
  using std::optional<T>::optional;

  template <class U>
  requires (is_constructible_v<T, const U&>)
  explicit(! is_convertible_v<U, T>)
    optional(const optional<U>& rhs) { if (rhs) *this = *rhs; }

  template <class U>
  requires (is_constructible_v<T, U>)
  explicit(! is_convertible_v<U, T>)
    optional(optional<U>&& rhs) { if (rhs) *this = *std::move(rhs); }

  template <class U = remove_cv_t<T>>
  T value_or(U&& u) const& {
    return this->has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }
  template <class U = remove_cv_t<T>>
  T value_or(U&& u) && {
    return this->has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }

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
  constexpr optional(nullopt_t) noexcept : optional() {}
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
  template <class U = remove_cv_t<T>>
  remove_cv_t<T> value_or(U&& u) const {
    using X = remove_cv_t<T>;
    return this->has_value() ? value() : static_cast<X>(std::forward<U>(u));
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

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

template <class T>
class optional : public std::optional<T>
{
public:
  using std::optional<T>::optional;

  template <class U = T, class... Args>
  U or_construct(Args&&... args) {
    static_assert(is_constructible_v<U, T&>);
    static_assert(is_constructible_v<U, Args...>);
    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
  }

  template <class U = T, class... Args>
  U or_construct(Args&&... args) const {
    static_assert(is_constructible_v<U, const T&>);
    static_assert(is_constructible_v<U, Args...>);
    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
  }

  // TBD: All monadic operations need to be re-implemented here, as the
  // inherited ones mandate functors that return `std::optional` instead of
  // `std::experimental::optional`.
};

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
  template<class U> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional(U&& v) : m_val(addressof(v))
    { static_assert(is_lvalue_reference_v<U>); }
  template<class U> requires (! is_same_v<remove_cvref_t<U>, optional>)
  constexpr optional(const optional<U>& v)
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
  template<class U>
  constexpr auto value_or(U&& v) const -> decltype(auto) {
    static_assert(is_lvalue_reference_v<U>,
                  "value_or argument must be an lvalue");
    using result = common_reference_t<T&, U&>;
    static_assert(is_lvalue_reference_v<result>,
                  "Argument and value_type must have a common reference type");
    return m_val ? static_cast<result>(*m_val) : static_cast<result>(v);
  }

  template <class U = remove_cvref_t<T>, class... Args>
  U or_construct(Args&&... args) {
    static_assert(is_constructible_v<U, T&>);
    static_assert(is_constructible_v<U, Args...>);
    return this->has_value() ? U(**this) : U(std::forward<Args>(args)...);
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

} // close namespace std::experimental

#endif // ! defined(INCLUDED_XOPTIONAL)

// Local Variables:
// c-basic-offset: 2
// End:

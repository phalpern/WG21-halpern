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
   (is_rvalue_reference_v<T> && !is_rvalue_reference_v<U&&>) ||
   ! is_constructible_v<remove_cvref_t<T>&, remove_cvref_t<U>&>);

template <class T, class U>
struct reference_constructs_from_temporary
  : std::integral_constant<bool,
                           reference_constructs_from_temporary_v<T, U>> {};


template <class T>
class optional : public std::optional<T> {
public:
  using std::optional<T>::optional;


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

// Get first type in a pack containing only one type.
template <class A0> struct __pack0 { using type = A0; };
template <class... Args> using __pack0_t = typename __pack0<Args...>::type;

// `maybe` concept per
template <class T>
concept maybe = requires(const T a, T b) {
  bool(a);
  *b;
};

template <class R = void, maybe T, class... U>
constexpr auto value_or(T&& m, U&&... u) -> decltype(auto)
{
  // Construct the return value from either `*m` or `forward<U>(u)... )`.

  // Find the type returned by dereferencing `m`
  using DerefType = decltype(*forward<T>(m));   // Often a reference type.
  using ValueType = remove_cvref_t<DerefType>;  // Non-reference value

  // If `U...` represents exactly one argument type, then `RetCalc::type` is
  // the common type of `ValueType` and `U`; otherwise `RetCalc::type` is
  // `ValueType`.  The result of this alias is a struct having a `type` member,
  // which is not "evaluated" unless needed.  If `R` is explicitly non-`void`,
  // `RetCalc::type` is never evaluated, so no error is reported in cases where
  // `U...` has length 1 but `common_type` fails to produce a type.
  using RetCalc = conditional_t<sizeof...(U) == 1,
                                common_type<ValueType, remove_cvref_t<U&&>...>,
                                type_identity<ValueType>>;

  // If `R` is non-void, the return type is exactly `R`, otherwise it is
  // `RetCalc::type`.
  static_assert(! is_same_v<R, void> || requires { typename RetCalc::type; },
                "No common type between value type and argument type");
  using Ret = typename conditional_t<is_same_v<R, void>,
                                     RetCalc, type_identity<R>>::type;

  // Check the mandates
  static_assert(is_constructible_v<Ret, DerefType>,
                "Cannot construct return type from value type");
  static_assert(is_constructible_v<Ret, U...>,
                "Cannot construct return type from argument types");
  static_assert(!is_reference_v<Ret> || 1 == sizeof...(U),
                "Reference return type requires exactly one argument");

  if constexpr (is_lvalue_reference_v<Ret> && 1 == sizeof...(U)) {
    using U0 = __pack0_t<U...>;
    static_assert(!reference_constructs_from_temporary_v<Ret, DerefType>,
                  "Would construct a dangling reference from a temporary");
    static_assert(!reference_constructs_from_temporary_v<Ret, U0>,
                  "Would construct a dangling reference from a temporary");
  }

  return bool(m) ? static_cast<Ret>(*forward<T>(m)) : Ret(forward<U>(u)...);
}

template <class R = void, maybe T, class IT, class... U>
constexpr auto value_or(T&& m, initializer_list<IT> il, U&&... u) -> decltype(auto)
{
  // Find the type returned by dereferencing `m`
  using DerefType = decltype(*forward<T>(m));   // Often a reference type.
  using ValueType = remove_cvref_t<DerefType>;  // Non-reference value

  using Ret = conditional_t<is_same_v<R, void>, ValueType, R>;

  // Check the mandates
  static_assert(is_constructible_v<Ret, DerefType>,
                "Cannot construct return type from value type");
  static_assert(is_constructible_v<Ret, initializer_list<IT>, U...>,
                "Cannot construct return type from argument types");

  return bool(m) ? static_cast<Ret>(*m) : Ret(il, forward<U>(u)...);
}

}  // namespace std::experimental

#endif  // ! defined(INCLUDED_XOPTIONAL)

// Local Variables:
// c-basic-offset: 2
// End:

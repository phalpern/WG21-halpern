/* maybe.h                                                            -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_MAYBE
#define INCLUDED_MAYBE

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

namespace imp {

// Get first type in a pack containing only one type.
template <class A0> struct __pack0 { using type = A0; };
template <class... Args> using __pack0_t = typename __pack0<Args...>::type;

} // close namespace imp (within std::experimental)

// `maybe` concept per P1255
template <class T>
concept maybe = requires(const T a) {
  bool(a);
  *a;
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
                                common_type<ValueType, remove_cvref_t<U>...>,
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
    using U0 = imp::__pack0_t<U...>;
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

template <class R = void, maybe T, class U>
constexpr auto reference_or(T&& m, U&& u) -> decltype(auto)
{
  // Construct the return value from either `*m` or `forward<U>(u)... )`.

  // Find the type returned by dereferencing `m`
  using DerefType = decltype(*forward<T>(m));   // Often a reference type.

  // If `U...` represents exactly one argument type, then `RetCalc::type` is
  // the common type of `ValueType` and `U`; otherwise `RetCalc::type` is
  // `ValueType`.  The result of this alias is a struct having a `type` member,
  // which is not "evaluated" unless needed.  If `R` is explicitly non-`void`,
  // `RetCalc::type` is never evaluated, so no error is reported in cases where
  // `U...` has length 1 but `common_type` fails to produce a type.
  using RetCalc = common_reference<DerefType, U>;

  // If `R` is non-void, the return type is exactly `R`, otherwise it is
  // `RetCalc::type`.
  static_assert(! is_same_v<R, void> || requires { typename RetCalc::type; },
                "No common type between ref type and argument type");
  using Ret = typename conditional_t<is_same_v<R, void>,
                                     RetCalc, type_identity<R>>::type;

  // Check the mandates
  static_assert(is_constructible_v<Ret, DerefType>,
                "Cannot construct return type from value type");
  static_assert(is_constructible_v<Ret, U>,
                "Cannot construct return type from argument type");

  static_assert(!reference_constructs_from_temporary_v<Ret, DerefType>,
                "Would construct a dangling reference from a temporary");
  static_assert(!reference_constructs_from_temporary_v<Ret, U>,
                "Would construct a dangling reference from a temporary");

  return bool(m) ? static_cast<Ret>(*forward<T>(m)) : static_cast<Ret>(forward<U>(u));
}

template <class R = void, maybe T, class I>
constexpr auto or_invoke(T&& m, I&& invocable) -> decltype(auto)
{
  // Construct the return value from either `*m` or `forward<U>(u)... )`.

  // Find the type returned by dereferencing `m`
  using DerefType = decltype(*forward<T>(m));   // Often a reference type.
  using InvokeRet = invoke_result_t<I>;

  // If `U...` represents exactly one argument type, then `RetCalc::type` is
  // the common type of `ValueType` and `U`; otherwise `RetCalc::type` is
  // `ValueType`.  The result of this alias is a struct having a `type` member,
  // which is not "evaluated" unless needed.  If `R` is explicitly non-`void`,
  // `RetCalc::type` is never evaluated, so no error is reported in cases where
  // `U...` has length 1 but `common_type` fails to produce a type.
  using RetCalc = common_type<DerefType, InvokeRet>;

  // If `R` is non-void, the return type is exactly `R`, otherwise it is
  // `RetCalc::type`.
  static_assert(! is_same_v<R, void> || requires { typename RetCalc::type; },
                "No common type between value type and invokable return type");
  using Ret = typename conditional_t<is_same_v<R, void>,
                                     RetCalc, type_identity<R>>::type;

  // Check the mandates
  static_assert(is_constructible_v<Ret, DerefType>,
                "Cannot construct return type from value type");
  static_assert(is_constructible_v<Ret, InvokeRet>,
                "Cannot construct return type from invokable return type");

  static_assert(!reference_constructs_from_temporary_v<Ret, DerefType>,
                "Would construct a dangling reference from a temporary");
  static_assert(!reference_constructs_from_temporary_v<Ret, InvokeRet>,
                "Would construct a dangling reference from a temporary");

  return bool(m) ? static_cast<Ret>(*forward<T>(m)) : static_cast<Ret>(invocable());
}

}  // close namespace std::experimental

#endif // ! defined(INCLUDED_MAYBE)

// Local Variables:
// c-basic-offset: 2
// End:

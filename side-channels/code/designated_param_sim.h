/* designated_param_sim.cpp                                           -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <utility>
#include <type_traits>
#include <concepts>
#include <iostream>

#include "type_list.h"

// Used for template debugging.  `print<T> x;` will display an error message
// that includes the name of `T`.
template <class T> struct print;

namespace designated_params {

/// Compile-time representation of a string used to designate a parameter or
/// argument.  Two `designator_t` objects represent the same designator if they
/// have the same type.  A constexpr object of type `designator_t` is typically
/// generated using the `_des` UDL in the `literals` namespace, below.
template <char...>
struct designator_t
{
  template <class Des2>
  friend constexpr bool operator==(designator_t, Des2)
    { return std::is_same_v<designator_t, Des2>; }

  template <class Des2>
  friend constexpr bool operator!=(designator_t d1, Des2 d2)
    { return ! (d1 == d2); }
};

template <char... C>
constexpr inline designator_t<C...> designator_v{};

namespace details {

/// Designator placeholder for non-designated types.
constexpr inline designator_t<> null_designator{};

/// Type trait evaluating to `true_type` if `T` is a specialization of
/// `designator_t`. Primary template evalutes to `false_type`.
template <class T>
struct is_designator : std::false_type { };

/// Partial specialization for `designator_t` evaluates to `true_type`.
/// Note that a `designator_t` having zero characters (i.e., the
/// type of `null_designator`) is not considered a designator by this trait.
template <char C0, char... C>
struct is_designator<designator_t<C0, C...>> : std::true_type { };

/// Placeholder argument for parameter having no default argument.
struct no_default_arg_t { };
constexpr inline no_default_arg_t no_default_arg{};

} // close namespace details

/// Using this concept to constrain a template allows the use of `designator`
/// as though it were a concrete type, i.e., `designator Des` specifies that
/// `D` is a specialization of `designator_t`, where its (immutable,
/// compile-time) value is carried by its type.
template <class D>
concept designator = details::is_designator<D>::value;

template <designator auto DesV> class designated_arg_factory;  // Forward decl

/// Representation of a designated argument, i.e., a runtime value with a
/// desginator.
template <designator auto DesV, class T>
class designated_arg
{
  T&& m_arg;
  constexpr designated_arg(T&& arg) : m_arg(std::forward<T>(arg)) { }
  friend class designated_arg_factory<DesV>;
public:
  static constexpr inline auto designator_v = DesV;
  using value_type                          = T;

  constexpr designated_arg(const designated_arg& rhs)
    : m_arg(std::forward<T>(rhs.m_arg)) { }

  /// Return the runtime value without the designator.
  T&& get() && { return std::forward<T>(m_arg); }
};

/// Factory class template for generating a `designated_arg`.  Typically, an
/// object of this class is created using the `_arg` UDL in the literal
/// namespace, below. For example `"value"_arg(3.2)` will create a
/// `designated_arg` having designator "value" and value 3.2. There are
/// actually two objects generated: an empty `designated_arg_factory` with
/// designator "value", which is used to generate a `designated_arg` holding
/// a `double` having value 3.2.
template <designator auto DesV>
struct designated_arg_factory
{
  template <class T>
  constexpr designated_arg<DesV, T&&> operator()(T&& value)
    { return std::forward<T>(value); }
};

template <class Arg>
struct is_designated_arg : std::false_type {};

template <auto DesV, class T>
struct is_designated_arg<designated_arg<DesV, T>> : std::true_type {};

template <class Arg>
constexpr inline bool is_designated_arg_v = is_designated_arg<Arg>::value;

/// Representation of a designated parameter with optional default argument
/// value.
template <class T, auto DesV, bool IsPositional,
          auto DefaultArg = details::no_default_arg>
requires (designator<decltype(DesV)> or DesV == details::null_designator)
class designated_param_t
{
#if 0  // TBD remove if designated_param does not represent a runtime value
  T&& m_value;
#endif

public:
  static constexpr inline auto designator_v = DesV;
  using value_type                          = T;
  using default_arg_t                       = decltype(DefaultArg);

  static constexpr inline bool is_designated =
    (designator_v != details::null_designator);
  static constexpr inline bool is_positional = IsPositional;
  static constexpr inline bool has_default_arg =
    ! std::is_same_v<default_arg_t, details::no_default_arg_t>;

#if 0  // TBD remove if designated_param does not represent a runtime value
  /// No default constructor unless there is a default argument value.
  constexpr designated_param_t()
  requires(! has_default_arg)  = delete;

  /// Default constructor when the default argument is a constant value of
  /// structural type convertible to `T`.
  constexpr designated_param_t()
    requires (std::is_convertible_v<default_arg_t, T>)
    : m_value(std::forward<T>(DefaultArg)) { }

  /// Default constructor when the default argument is specified as a functor
  /// returning a type convertible to `T`.
  constexpr designated_param_t()
    requires (std::is_convertible_v<std::invoke_result_t<default_arg_t>, T&&>)
    : m_value(std::forward<T>(DefaultArg())) { }

  /// Construct from a designated argument having the same designator as this
  /// parameter.
  template <class U = T>
    // TBD: Allow non-reference conversion; this will require constructing a
    // temporary `T` in a member buffer.
    requires (std::is_convertible_v<U&, T&>)
  constexpr designated_param_t(designated_arg<DesV, U>&& arg)  // IMPLICIT
    : m_value(std::move(arg).get()) { }

  /// Construct from a non-designated argument (if positional).
  template <class U = T>
    requires (std::is_convertible_v<U&, T&>)
    // TBD: Allow non-reference conversion; this will require constructing a
    // temporary `T` in a member buffer.
  constexpr designated_param_t(U&& arg)                      // IMPLICIT
    requires (IsPositional)
    : m_value(std::forward<T>(arg)) { }

  /// Return the value bound to this parameter.
  T&& get() const { return std::forward<T>(m_value); }
#endif // 0
};

/// Abbreviation for apositional non-designated parameter.
template <class T, auto DefaultArg = details::no_default_arg>
using pp = designated_param_t<T, details::null_designator, true, DefaultArg>;

/// Abbreviation for a positional designated parameter.
template <class T, designator auto DesV,
          auto DefaultArg = details::no_default_arg>
using dpp = designated_param_t<T, DesV, true, DefaultArg>;

/// Abbreviation for a non-positional designated parameter.
template <class T, designator auto DesV,
          auto DefaultArg = details::no_default_arg>
using dp = designated_param_t<T, DesV, false, DefaultArg>;

namespace details {

template <class T>
struct  has_no_default : std::true_type { };

template <class T, auto DesV, bool IsPositional, auto DefaultArg>
struct  has_no_default<designated_param_t<T, DesV, IsPositional, DefaultArg>>
  : std::is_same<decltype(DefaultArg), no_default_arg_t>::type
{
};

/// Nested template
template <auto DesV>
struct match_des
{
  template <class T>
  struct apply : std::bool_constant<DesV == T::designator_v> {};
};

/// Match the designated arguments in the `Args` list to the parameters in
/// `Params`. The number of unconsumed parameters having no default values is
/// passed in `paramsLeft`. Return `false` if an argument is not matched to a
/// parameter or if, after recursing `paramsLeft` does not go down to zero.
template <class Params, class Args>
consteval bool match_da(std::size_t paramsLeft)
{
  if constexpr (0 == Args::size()) {
    return (0 == paramsLeft);  // Succeed if all parameters were matched
  }
  else {
    static constexpr std::size_t param_idx =
      type_list_find_v<Params,
                       match_des<Args::head::designator_v>::template apply>;
    if constexpr (param_idx >= Params::size()) {
      // Did not find a parameter with the same designator.
      // TBD: Match variadic param here.
      return false;
    }
    else if constexpr (std::is_convertible_v<
                         typename Args::head::value_type,
                         typename type_list_element_t<param_idx,
                                                      Params>::value_type
                       >)
    {
      // Matched one designated argument with one designated parameter,
      // including convertability of the value type.  Recurse on remaining
      // arguments.

      // If the matched parameter did not have a default argument, reduce the
      // count of unmatched parameters by one.
      paramsLeft -=
        type_list_element_t<param_idx, Params>::has_default_arg ? 0 : 1;
      return match_da<Params, typename Args::tail>(paramsLeft);
    }
    else {
      // Found matching designator, but convertibility test failed.
      return false;
    }
  }
}

/// Match the designated arguments in the `Args` list to the parameters in
/// `Params`. Return `false` if an argument is not matched to a
/// parameter or if, after recursing there are unmatched parameters not having
/// default argument values.
template <class Params, class Args>
consteval bool match_da()
{
  // Count number of designated params left that don't have default arg then
  // call a recursive function to process the remaining args.
  return match_da<Params, Args>(type_list_count_v<Params, has_no_default>);
}

/// Match the undesignated positional arguments in the `Args` list to the
/// corresponding `Params`.  Return `false` if there's a mismtach; otherwise,
/// pass the remaining arguments and parameters to `match_dp`.
template <class Params, class Args>
consteval bool match_pa()
{
  if constexpr (0 == Args::size()) {
    // No more arguments
    return match_da<Params, Args>();
  }
  else if constexpr (is_designated_arg_v<typename Args::head>) {
    // No more positional arguments
    return match_da<Params, Args>();
  }
  else if constexpr (std::is_convertible_v<typename Args::head,
                                           typename Params::head::value_type>){
    // Matched one argument to one parameter; now match the rest, recursively.
    return match_pa<typename Params::tail, typename Args::tail>();
  }
  // TBD: Match variadic param here
  else {
    // Missmatch
    return false;
  }
}


} // close namespace details

template <class... Params>
class func_signature
{
public:
  /// Return true if this is a viable function (each argument matches a
  /// parameter and each parameter either matches an argument or has a default
  /// value).
  /// Mandates: `Params...` has been validated as being in valid.
  template <class... Args>
  consteval bool is_viable() const {
    return details::match_pa<type_list<Params...>, type_list<Args...>>();
  }
};

namespace literals {

// UDLs

/// UDL for a designator
template <class CharT, CharT... C>
constexpr designator_t<C...> operator""_des() { return {}; }

/// UDL to generate a designated argument.
/// Example: `"ident"_arg(value)` generates a designated argument having the
/// designator, "ident"_des, and the value, `value`.
template <class CharT, CharT... C>
constexpr designated_arg_factory<designator_v<C...>> operator""_arg()
{ return {}; }

} // close namespace literals
} // close namespace designated_params

// Local Variables:
// c-basic-offset: 2
// End:

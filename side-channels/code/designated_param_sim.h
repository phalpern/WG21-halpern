/* designated_param_sim.cpp                                           -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <utility>
#include <tuple>
#include <type_traits>
#include <concepts>

#include "type_list.h"

// This implementation uses a gcc extension (also available in clang) that
// allows string-UDL templates.  Disable warning about extension.
#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

namespace designated_params {

// Used for template debugging.  `print<T> x;` will display an error message
// that includes the name of `T`.
template <class T> struct print;
template <auto V> struct printv;

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

/// Trait has value `true` if `T` can bind to a temporary, i.e., if `T` is an
/// rvalue reference or a const lvalue reference.
template <class T>
struct would_bind_to_temporary
  : std::disjunction<std::is_same<T, T&&>, std::is_same<T, const T&>>
{
};

template <class T>
constexpr inline bool would_bind_to_temporary_v =
  would_bind_to_temporary<T>::value;

/// Metafunction has value `true` if a function having return type `To` can
/// return a variable or expression of type `From` without creating a dangling
/// reference to a materialized temporary object.
///
/// To evaluate to true, `From` must be convertible to `To` and, if `To` can
/// bind to a temporary (i.e., it has type `const T&` or `T&&`) then at least
/// one of the following must be hold:
///
///  1. `From` has a conversion operator to `To` OR
///  2. `From` is a reference type having a non-user-defined conversion to `To`
///
/// Otherwise, evaluate to false.
///
/// Note that this metafunction does not detect "internal" dangling, such as an
/// `operator T&` that returns a reference to a member variable or a
/// constructor taking a reference parameter and holding on to the reference.
template <class From, class To>
struct can_return_without_dangling : std::bool_constant<
  (std::is_convertible_v<From, To> &&
   (! would_bind_to_temporary_v<To> ||
    (requires (From&& x) { { x.operator To() }; }) ||
    (std::is_reference_v<From> &&
     std::is_convertible_v<std::remove_reference_t<From>*,
                           std::remove_reference_t<To>*>))) >
{
};

template <class From, class To>
constexpr inline bool can_return_without_dangling_v =
  can_return_without_dangling<From, To>::value;

} // close namespace details

/// Using this concept to constrain a template allows the use of `designator`
/// as though it were a concrete type, i.e., `designator Des` specifies that
/// `D` is a specialization of `designator_t`, where its (immutable,
/// compile-time) value is carried by its type.
template <class D>
concept designator = details::is_designator<D>::value;

template <designator auto DesV> struct designated_arg_factory;  // Forward decl

/// Representation of a designated argument, i.e., a runtime value with a
/// desginator.
template <designator auto DesV, class T>
class designated_arg
{
  T&& m_arg;
  constexpr designated_arg(T&& arg) : m_arg(std::forward<T>(arg)) { }
  friend struct designated_arg_factory<DesV>;
public:
  static constexpr inline auto designator_v = DesV;
  using value_type                          = T;

  /// Return the runtime value without the designator.
  constexpr T&& get() { return std::forward<T>(m_arg); }
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

/// Representation of a possibly-designated parameter with optional default
/// argument value. `DefaultArgSrc` is either the default argument value or a
/// functor that generates the default argument value.
template <class T, auto DesV, bool IsPositional, class DefaultArgT>
requires (designator<decltype(DesV)> or DesV == details::null_designator)
class param
{
  DefaultArgT m_default_arg;

public:
  static constexpr inline auto designator_v = DesV;
  using value_type                          = T;

  static constexpr inline bool is_designated =
    (designator_v != details::null_designator);
  static constexpr inline bool is_positional = IsPositional;
  static constexpr inline bool has_default_arg =
    ! std::is_same_v<DefaultArgT, details::no_default_arg_t>;

  /// Construct with no default argument.
  constexpr param() requires(! has_default_arg) { }

  /// Construct with default argument.
  /// Mandates that `DefaultArgT` either be convertible to `T` or be an
  /// invocable whose return value is convertible to `T`.
  constexpr explicit param(DefaultArgT&& da)
    requires (std::is_convertible_v<DefaultArgT, T>)
    : m_default_arg(std::forward<DefaultArgT>(da)) { }

  constexpr explicit param(DefaultArgT&& da)
    requires (std::is_convertible_v<std::invoke_result_t<DefaultArgT>, T>)
    : m_default_arg(std::forward<DefaultArgT>(da))
  {
  }

  /// Return the default argument stored in this parameter for cases where the
  /// default argument is not a functor.
  constexpr decltype(auto) default_arg() const
    requires (std::is_convertible_v<DefaultArgT, T>) {
    // TBD: Currently should not use this function if initializing a `T` would
    // require materializing a temporary. Assert that is not the case.
    static_assert(details::can_return_without_dangling_v<DefaultArgT, T>,
                  "Not implemented. Default argument return would dangle.");
    return m_default_arg;
  }

  /// Return the default argument stored in this parameter for cases where the
  /// default argument is a functor.
  constexpr decltype(auto) default_arg() const
    requires (std::is_convertible_v<std::invoke_result_t<DefaultArgT>, T>) {
    using DAResult = std::invoke_result_t<DefaultArgT>;
    // TBD: Currently should not use this function if initializing a `T` would
    // require materializing a temporary. Assert that is not the case.
    static_assert(details::can_return_without_dangling_v<DAResult, T>,
                  "Not implemented. Default argument return would dangle.");
    return m_default_arg();
  }
};

/// Factory functions for generating `param` instances:
///  pp<T>(...)  returns a positional parameter. A designator and/or default
///              argument can be optionally specified.
///  npp<T>(...) returns a nonpositional parameter. A designator is required, a
///              default argument can be optionally specified.

/// Return a nondesignated positional parameter.
template <class T>
constexpr
param<T, details::null_designator, true, details::no_default_arg_t> pp()
{
  return {};
}

/// Return a nondesignated positional parameter with default argument.
template <class T, class DefArg = T>
requires (! designator<DefArg>)
constexpr param<T, details::null_designator, true, DefArg> pp(DefArg&& da)
{
  using ret_type = param<T, details::null_designator, true, DefArg>;
  return ret_type(std::forward<DefArg>(da));
}

/// Return a designated positional parameter.
template <class T, designator DesT>
constexpr param<T, DesT{}, true, details::no_default_arg_t> pp(DesT)
{
  return {};
}

/// Return a designated positional parameter with default argument.
template <class T, designator DesT, class DefArg = T>
constexpr param<T, DesT{}, true, DefArg> pp(DesT, DefArg&& da)
{
  using ret_type = param<T, DesT{}, true, DefArg>;
  return ret_type(std::forward<DefArg>(da));
}

/// Return a designated nonpositional parameter.
template <class T, designator DesT>
constexpr param<T, DesT{}, false, details::no_default_arg_t> npp(DesT)
{
  return {};
}

/// Return a designated nonpositional parameter with default argument.
template <class T, designator DesT, class DefArg = T>
constexpr param<T, DesT{}, false, DefArg> npp(DesT, DefArg&& da)
{
  using ret_type = param<T, DesT{}, false, DefArg>;
  return ret_type(std::forward<DefArg>(da));
}

namespace details {

template <class T>
struct  has_no_default : std::true_type { };

template <class T>
struct  has_no_default<const T> : has_no_default<T>::type { };

template <class T, auto DesV, bool IsPositional, class DefaultArgT>
struct  has_no_default<param<T, DesV, IsPositional, DefaultArgT>>
  : std::is_same<DefaultArgT, no_default_arg_t>::type
{
};

/// Remove cvref qualification from `designated_arg`, but leave other types
/// unchanged.
template <class A, class NA = std::remove_cvref_t<A>>
struct norm_arg
{
  using type = A;
};

template <class A, auto DesV, class T>
struct norm_arg<A, designated_arg<DesV, T>>
{
  using type = designated_arg<DesV, T>;
};

template <class A> using norm_arg_t = norm_arg<A>::type;

/// Nested template
template <auto DesV, class T>
struct match_des_apply : std::false_type { };

template <auto DesV, class T>
requires requires { T::designator_v; }
struct match_des_apply<DesV, T>
{
  using type = std::bool_constant<DesV == T::designator_v>;
};

template <auto DesV>
struct match_des
{
  template <class T>
  struct apply : match_des_apply<DesV, T>::type { };
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
  else if constexpr (Params::head::is_positional &&
                     std::is_convertible_v<typename Args::head,
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

template <size_t index, class Param, class... Args>
constexpr decltype(auto) arg_lookup(const Param& p, std::tuple<Args...>& args)
{
  using arg_type_list = type_list<std::remove_reference_t<Args>...>;

  if constexpr (index < type_list_find_v<arg_type_list, is_designated_arg>) {
    // Positional parameter matches non-designated (positional) argument having
    // the same index.
    return std::get<index>(args);
  }
  else if constexpr (Param::is_designated) {
    // Positional parameter matches designated argument by name.
    constexpr std::size_t arg_idx =
      type_list_find_v<arg_type_list,
                       match_des<Param::designator_v>::template apply>;
    if constexpr (arg_idx < arg_type_list::size()) {
      return std::get<arg_idx>(args).get();
    }
    else {
      // No match, return default argument value
      return p.default_arg();
    }
  }
  else {
    // No match, return default argument value
    return p.default_arg();
  }
}

} // close namespace details

/// Representation of a function signature.
template <class... Params>
class func_signature
{
  // TBD: use static_assert to validate that Params... is a valid set.

  using param_values = std::tuple<typename Params::value_type...>;

  std::tuple<Params...> m_params;

  template <std::size_t... indexes, class ArgTuple>
  constexpr param_values
  get_param_values_imp(std::index_sequence<indexes...>, ArgTuple& args) const
  {
    return param_values(details::arg_lookup<indexes>(std::get<indexes>(m_params),
                                                     args)...);
  }

public:
  /// Construct from a set of parameters.
  /// `ParamArgs` needs to be pretty much the same as `Params` but is a
  /// template parmeter so as to deduce lvalue/rvalue category.
  template <class... ParamArgs>
  requires (sizeof...(ParamArgs) == sizeof...(Params))
  constexpr func_signature(ParamArgs&&... pa)
    : m_params(std::forward<ParamArgs>(pa)...) { }

  /// Return true if this is a viable function for the specified argument list.
  /// A function is viable if each argument matches a parameter and each
  /// parameter either matches an argument or has a default value. If an
  /// argument is not convertible to the parameter type, it is not considered a
  /// match.
  /// Mandates: `Params...` has been validated as being in valid.
  template <class... Args>
  consteval bool is_viable() const {
    return details::match_pa<type_list<Params...>,
                             type_list<details::norm_arg_t<Args>...>>();
  }

  /// Bind the specified `args` to the formal parameters. The returned `tuple`
  /// is suitable for structured binding to local variables.
  template <class... Args>
  constexpr param_values get_param_values(Args&&... args) const
  {
    auto arg_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
    using indexes  = std::make_index_sequence<sizeof...(Params)>;
    return get_param_values_imp(indexes{}, arg_tuple);
  }
};

/// Deduction guide for `func_signature`.  Note that, since `Params...` is a
/// pack of `param` objects that wrap the actual parameter types, removing
/// references from `Params` does not affect the real parameter types in the
/// signature.
template <class... Params>
func_signature(Params&&... p) ->
  func_signature<std::remove_reference_t<Params>...>;

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

/* designated_param_sim.cpp                                           -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <utility>
#include <type_traits>
#include <concepts>
#include <iostream>

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
};

namespace details {

/// Designator placeholder for non-designated types.
using null_designator_t = designator_t<>;
constexpr inline null_designator_t null_designator{};

/// Type trait evaluating to `true_type` if `T` is a specialization of
/// `designator_t`. Primary template evalutes to `false_type`.
template <class T>
struct is_designator : std::false_type { };

/// Partial specialization for `designator_t` evaluates to `true_type`.
/// Note that a `designator_t` having zero characters (i.e., the
/// `null_designator_t` type) is not considered a designator by this trait.
template <char C0, char... C>
struct is_designator<designator_t<C0, C...>> : std::true_type { };


/// Placeholder argument for parameter having no default argument.
struct no_default_arg_t { };
constexpr inline no_default_arg_t no_default_arg{};

} // close namespace details

/// Using this concept to constrain a template allows the use of `designator`
/// as though it were a concrete type, i.e., `designator Des` specifies that
/// `Des` is a specialization of `designator_t`, where its (immutable,
/// compile-time) value is carried by its type.
template <class D>
concept designator = details::is_designator<D>::value;

template <designator Des> class designated_arg_factory;  // Forward decl

/// Representation of a designated argument, i.e., a runtime value with a
/// desginator.
template <designator Des, class T>
class designated_arg
{
  T&& m_arg;
  constexpr designated_arg(T&& arg) : m_arg(std::forward<T>(arg)) { }
  friend class designated_arg_factory<Des>;
public:
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
template <designator Des>
struct designated_arg_factory
{
  template <class T>
  constexpr designated_arg<Des, T&&> operator()(T&& value)
    { return std::forward<T>(value); }
};

/// Representation of a designated parameter with optional default argument
/// value.
template <class T, designator auto DesV, bool IsPositional,
          auto DefaultArg = details::no_default_arg>
class designated_param_t
{
  using Des         = decltype(DesV);
  using DefaultArgT = decltype(DefaultArg);

  T&& m_value;

public:
  static constexpr inline bool is_designated =
    ! std::is_same_v<Des, details::null_designator_t>;
  static constexpr inline bool is_positional = IsPositional;
  static constexpr inline bool has_default_arg =
    ! std::is_same_v<DefaultArgT, details::no_default_arg_t>;

  /// No default constructor unless there is a default argument value.
  constexpr designated_param_t()
  requires(! has_default_arg)  = delete;

  /// Default constructor when the default argument is a constant value of
  /// structural type convertible to `T`.
  constexpr designated_param_t()
    requires (std::is_convertible_v<DefaultArgT, T>)
    : m_value(std::forward<T>(DefaultArg)) { }

  /// Default constructor when the default argument is specified as a functor
  /// returning a type convertible to `T`.
  constexpr designated_param_t()
    requires (std::is_convertible_v<std::invoke_result_t<DefaultArgT>, T&&>)
    : m_value(std::forward<T>(DefaultArg())) { }

  /// Construct from a designated argument having the same designator as this
  /// parameter.
  template <class U = T>
  requires (std::is_convertible_v<U&, T&>)
  constexpr designated_param_t(designated_arg<Des, U>&& arg)  // IMPLICIT
    : m_value(std::move(arg).get()) { }

  /// Construct from a non-designated argument (if positional).
  template <class U = T>
  requires (std::is_convertible_v<U&, T&>)
  constexpr designated_param_t(U&& arg)                      // IMPLICIT
    requires (IsPositional)
    : m_value(std::forward<T>(arg)) { }

  /// Return the value bound to this parameter.
  T&& get() const { return std::forward<T>(m_value); }
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

template <class... Params>
struct func_prototype
{
  template <class... Args>
  constexpr bool is_viable() const { return false; }
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
constexpr designated_arg_factory<designator_t<C...>> operator""_arg()
{ return {}; }

} // close namespace literals
} // close namespace designated_params

// Local Variables:
// c-basic-offset: 2
// End:

/* no_dangle.cpp                                                      -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <type_traits>
#include <utility>

template <class T>
struct __convertible_to_T {
  operator T();
};


/// Return true if `From` is convertible to `To` without user-defined
/// conversions.
template <class From, class To>
struct is_convertible_without_udc
  : std::is_convertible<__convertible_to_T<From>, To> { };

template <class From, class To>
inline constexpr bool is_convertible_without_udc_v =
  is_convertible_without_udc<From, To>::value;

// /// Return true if `From` is convertible to `To` without risk of creating a
// /// dangling reference.  Assumes that if `T` is convertible to `U&`, then the
// /// resulting reference has a lifetime no longer than the original.
// template <class From, class To>
// struct is_convertible_without_dangling
//   : std::conjunction<std::is_convertible<__convertible_to_T<From>, To>,
//                      std::disjunction<std::is_reference_v<From>,
//                                       std::negate<std::is_reference<To>>>
// { };

// template <class From, class To>
// inline constexpr bool is_convertible_without_dangling_v =
//   is_convertible_without_dangling<From, To>::value;


struct X { };

struct D : X { };

struct Y { operator X() const { return {}; } };

struct Z { X x; operator X&() { return x; } };

struct R { X x; operator X&&() { return std::move(x); } };

static_assert( std::is_convertible_v<D,  X>);
static_assert( std::is_convertible_v<D&, X>);
static_assert(!std::is_convertible_v<D,  X&>);
static_assert( std::is_convertible_v<D&, X&>);
static_assert( std::is_convertible_v<D,  const X&>);
static_assert( std::is_convertible_v<D&, const X&>);

static_assert( std::is_convertible_v<Y,  X>);
static_assert( std::is_convertible_v<Y&, X>);
static_assert(!std::is_convertible_v<Y,  X&>);
static_assert(!std::is_convertible_v<Y&, X&>);
static_assert( std::is_convertible_v<Y,  const X&>);
static_assert( std::is_convertible_v<Y&, const X&>);

static_assert( std::is_convertible_v<Z,  X>);
static_assert( std::is_convertible_v<Z&, X>);
static_assert( std::is_convertible_v<Z,  X&>);
static_assert( std::is_convertible_v<Z&, X&>);
static_assert( std::is_convertible_v<Z,  const X&>);
static_assert( std::is_convertible_v<Z&, const X&>);

static_assert( is_convertible_without_udc_v<D,  X>);
static_assert( is_convertible_without_udc_v<D&, X>);
static_assert(!is_convertible_without_udc_v<D,  X&>);
static_assert( is_convertible_without_udc_v<D&, X&>);
static_assert( is_convertible_without_udc_v<D,  const X&>);
static_assert( is_convertible_without_udc_v<D&, const X&>);

static_assert(!is_convertible_without_udc_v<Y,  X>);
static_assert(!is_convertible_without_udc_v<Y&, X>);
static_assert(!is_convertible_without_udc_v<Y,  X&>);
static_assert(!is_convertible_without_udc_v<Y&, X&>);
static_assert(!is_convertible_without_udc_v<Y,  const X&>);
static_assert(!is_convertible_without_udc_v<Y&, const X&>);

static_assert(!is_convertible_without_udc_v<Z,  X>);
static_assert(!is_convertible_without_udc_v<Z&, X>);
static_assert(!is_convertible_without_udc_v<Z,  X&>);
static_assert(!is_convertible_without_udc_v<Z&, X&>);
static_assert(!is_convertible_without_udc_v<Z,  const X&>);
static_assert(!is_convertible_without_udc_v<Z&, const X&>);

template <class From, class To>
struct convertible_without_dangling_imp {
  // Primary template used when `To` is not a lifetime-extending reference
  // (i.e., `T&&` or `const T&`)
  static inline constexpr bool value = is_convertible_v<From, To>;
};

template <class From, class To>
  requires (is_convertible_v<From, const To&>)
struct convertible_without_dangling_imp<From, const To&> {
  // Specialization for conversion from `From` to const lvalue ref `To`

  static constexpr int  check(To&&);  // Preferred if temp created
  static constexpr char check(const To&); // Preferred if reference conversion

  static inline constexpr bool value =
    std::is_lvalue_reference_v<From> &&
    1 == sizeof(check(std::declval<From&>()));
};

template <class From, class To>
  requires (is_convertible_v<From, To&&>)
struct convertible_without_dangling_imp<From, To&&> {
  // Specialization for conversion from `From` to rvalue ref `To`
  static inline constexpr bool value =
    std::is_reference_v<From> &&
    is_convertible_without_udc_v<From&&, To&&>;
};

template <class From, class To>
inline constexpr bool convertible_without_dangling_imp_v = convertible_without_dangling_imp<From, To>::value;

static_assert(! convertible_without_dangling_imp_v<D&, const X&>);
static_assert(  convertible_without_dangling_imp_v<Y&, const X&>);
static_assert(! convertible_without_dangling_imp_v<Z&, const X&>);

static_assert(  convertible_without_dangling_imp_v<Y&&, X&&>); // capitalize on this being ambiguous
static_assert(  convertible_without_dangling_imp_v<R&&, X&&>);

const X& f()
{
  Y y;
  const X& ret = y;
  return ret;
}

int main()
{
}

// Local Variables:
// c-basic-offset: 2
// End:

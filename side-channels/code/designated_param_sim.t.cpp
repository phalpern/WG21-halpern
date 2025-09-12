/* designated_param_sim.t.cpp                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <designated_param_sim.h>
#include <string>
#include <iostream>

static int error_count = 0;

void test_assert_failure(const char* file, int line, const char* expr)
{
  ++error_count;
  std::cout << file << ':' << line << ": test assertion failed: " << expr
            << std::endl;
}

#define TEST_ASSERT(C)                                                  \
  if (C) (void) 0; else test_assert_failure(__FILE__, __LINE__, #C)

#define TEST_EQ(V1, V2)                                         \
  if ((V1) == (V2)) (void) 0;                                   \
  else test_assert_failure(__FILE__, __LINE__, #V1 " == " #V2)

#define TEST_SAME(T1, T2)                                               \
  static_assert(std::is_same_v<T1, T2>,                                 \
                "Types " #T1 " and " #T2 " should be the same")

#define TEST_TYPE_IS(VAL, T) TEST_SAME(decltype(VAL), T)

namespace dp = designated_params;
using namespace dp::literals;

// Test `_des` UDL
constexpr auto h = "hello"_des;
constexpr auto x = "x"_des;

// extern function to suppress unused-variable warnings
void suppress_test_warnings1()
{
  (void) h;
  (void) x;
}

// Parameter declarations
constexpr auto pp1    = dp::pp<const double&>();      // const double& a
constexpr auto pp2    = dp::pp<const char*>(nullptr); // const char *b =nullptr
constexpr auto xparam = dp::npp<int>("x"_des);        // int .x
// TBD: Workound for inability to use default arguments that would materialize
// temporaries when used.
struct Five { operator int&&() const {
  static int r = 5; return std::move(r); }
} five;
constexpr auto yparam = dp::npp<int&&>("y"_des, five);// int&& .y = 5
//constexpr auto yparam = dp::npp<int&&>("y"_des, 5);   // int&& .y = 5
constexpr auto zparam = dp::pp<std::string>("z"_des,  // string .z = "hello"
  [] -> std::string { return "hello"; });

using xparam_t = decltype(xparam);
using yparam_t = decltype(yparam);
using pp1_t    = decltype(pp1   );
using pp2_t    = decltype(pp2   );
using zparam_t = decltype(zparam);

// Designated-argument types
auto xarg   = "x"_arg(short(9));   // .x = short(9)
auto nxarg  = "x"_arg(nullptr);    // .x = nullptr
auto yarg   = "y"_arg(99);         // .y = 99
auto zarg   = "z"_arg("bye");      // .z = "bye"

using xarg_t  = decltype(xarg );
using nxarg_t = decltype(nxarg);
using yarg_t  = decltype(yarg );
using zarg_t  = decltype(zarg );

// White-box incremental testing
namespace test_details {

using namespace dp::details;

// test `can_return_without_dangling`
struct B { };
struct W : B { };
struct X { operator       W()   const; };
struct Y { operator const W&()  const; };
struct Z { operator       W&&() const; };

static_assert(  can_return_without_dangling_v<int, int>);
static_assert(! can_return_without_dangling_v<int, int&>);
static_assert(! can_return_without_dangling_v<int, const int&>);
static_assert(! can_return_without_dangling_v<int, int&&>);
static_assert(  can_return_without_dangling_v<W  , W  >);
static_assert(  can_return_without_dangling_v<W  , B  >);
static_assert(  std::is_convertible_v<W  , B  >);
static_assert(  can_return_without_dangling_v<W& , B  >);
static_assert(  can_return_without_dangling_v<W& , B& >);
static_assert(  can_return_without_dangling_v<W& , const B&>);
static_assert(  can_return_without_dangling_v<W&&, const B&>);
static_assert(  can_return_without_dangling_v<X  , W  >);
static_assert(! can_return_without_dangling_v<X  , const W&>);
static_assert(! can_return_without_dangling_v<X  , W&&>);
static_assert(  can_return_without_dangling_v<X& , W  >);
static_assert(! can_return_without_dangling_v<X& , const W&>);
static_assert(! can_return_without_dangling_v<X& , W&&>);
static_assert(  can_return_without_dangling_v<X&&, W  >);
static_assert(! can_return_without_dangling_v<X&&, const W&>);
static_assert(! can_return_without_dangling_v<X&&, W&&>);
static_assert(  can_return_without_dangling_v<Y  , W  >);
static_assert(  can_return_without_dangling_v<Y  , const W&>);
static_assert(! can_return_without_dangling_v<Y  , W&&>);
static_assert(  can_return_without_dangling_v<Y& , W  >);
static_assert(  can_return_without_dangling_v<Y& , const W&>);
static_assert(! can_return_without_dangling_v<Y& , W&&>);
static_assert(  can_return_without_dangling_v<Y&&, W  >);
static_assert(  can_return_without_dangling_v<Y&&, const W&>);
static_assert(! can_return_without_dangling_v<Y&&, W&&>);
static_assert(  can_return_without_dangling_v<Z  , W  >);
static_assert(! can_return_without_dangling_v<Z  , const W&>);
static_assert(  can_return_without_dangling_v<Z  , W&&>);
static_assert(  can_return_without_dangling_v<Z& , W  >);
static_assert(! can_return_without_dangling_v<Z& , const W&>);
static_assert(  can_return_without_dangling_v<Z& , W&&>);
static_assert(  can_return_without_dangling_v<Z&&, W  >);
static_assert(! can_return_without_dangling_v<Z&&, const W&>);
static_assert(  can_return_without_dangling_v<Z&&, W&&>);

// test `has_no_default`
static_assert(  has_no_default<xparam_t>::value);
static_assert(! has_no_default<yparam_t>::value);

// test `match_des`
static_assert(  match_des<"x"_des>::apply<xparam_t>::value);
static_assert(  match_des<"y"_des>::apply<yparam_t>::value);
static_assert(! match_des<"x"_des>::apply<yparam_t>::value);

// basic test for `match_da`
static_assert(  match_da<type_list<>, type_list<>>());
static_assert(! match_da<type_list<>, type_list<xarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<>>());
static_assert(  match_da<type_list<yparam_t>, type_list<>>());
static_assert(  match_da<type_list<xparam_t>, type_list<xarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<nxarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<yarg_t>>());
static_assert(! match_da<type_list<xparam_t, yparam_t>, type_list<yarg_t>>());
static_assert(  match_da<type_list<xparam_t, yparam_t>, type_list<xarg_t>>());
static_assert(  match_da<type_list<xparam_t, yparam_t>,
                type_list<xarg_t, yarg_t>>());
static_assert(! match_da<type_list<xparam_t, yparam_t>,
              type_list<nxarg_t, yarg_t>>());

// test parameter list
using params = type_list<pp1_t, pp2_t, zparam_t, xparam_t, yparam_t>;

// basic test for `match_pa`
static_assert(! match_pa<params, type_list<>>());
static_assert(! match_pa<params, type_list<int>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg_t, yarg_t>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4],
                                           xarg_t, yarg_t>>());
static_assert(  match_pa<params, type_list<float, xarg_t, yarg_t, zarg_t>>());
static_assert(  match_pa<params, type_list<float, yarg_t, xarg_t, zarg_t>>());
static_assert(! match_pa<params, type_list<yarg_t, xarg_t, zarg_t>>());
static_assert(! match_pa<params, type_list<float, yarg_t, zarg_t>>());
static_assert(  match_pa<params, type_list<float, xarg_t, zarg_t>>());
// The following should fail because argument `z` is specified both
// positionally and designated.
static_assert(! match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg_t, yarg_t, zarg_t>>());

}

// constexpr auto params1 =
//   dp::func_signature<pp1_t, pp2_t, zparam_t, xparam_t, yparam_t>{};

// A function signature for testing.  First argument is the same as pp1, but
// we need to test using an rvalue, which is more typical.
constexpr dp::func_signature sig1 = {
  dp::pp<const double&>(), pp2, zparam, xparam, yparam
};

static_assert(! sig1.is_viable<>());
static_assert(! sig1.is_viable<int>());
static_assert(  sig1.is_viable<int, const char(&)[4], const char*,
                               xarg_t, yarg_t>());
static_assert(  sig1.is_viable<int, const char(&)[4], xarg_t, yarg_t>());
static_assert(  sig1.is_viable<float, xarg_t, yarg_t, zarg_t>());
static_assert(  sig1.is_viable<float, yarg_t, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<yarg_t, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<float, yarg_t, zarg_t>());
static_assert(  sig1.is_viable<float, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<int, const char(&)[4], const char*,
                               xarg_t, yarg_t, zarg_t>()); // dup z

/// Default `type_base_name` implementation uses `typeid`.
template <class T>
constexpr const char* base_type_name(const T&) { return typeid(T).name(); }

// template <std::size_t SZ>
// constexpr const char* base_type_name(const char(&)[SZ])
// {
//   return "const char[]";
// }

// This macro generates an overload of `base_type_name` that returns the string
// "T" when invoked with an object of type `T`. The use of `type_identity`
// treats `T` as a single type rather than a sequence of tokens, thus
// preventing syntax errors when `T` is a complex type such as `const char*` or
// `int []`.
#define BASE_TYPE_NAME(T) \
  constexpr const char* base_type_name(const std::type_identity_t<T>&) { return #T; }

BASE_TYPE_NAME(char)
BASE_TYPE_NAME(char*)
BASE_TYPE_NAME(const char*)
BASE_TYPE_NAME(short)
BASE_TYPE_NAME(int)
BASE_TYPE_NAME(long)
BASE_TYPE_NAME(float)
BASE_TYPE_NAME(double)
BASE_TYPE_NAME(std::string)
BASE_TYPE_NAME(std::nullptr_t)
BASE_TYPE_NAME(char[])

template <class T>
void print_type_and_value(const T& v)
{
  std::cout << '(';
  if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    std::cout << "const ";

  std::cout << base_type_name(v);

  if constexpr (std::is_lvalue_reference_v<T>)
    std::cout << "&";
  else if constexpr (std::is_rvalue_reference_v<T>)
    std::cout << "&&";

  std::cout << ") " << v << std::endl;
}

template <class T>
void print_type_and_value(T&& v)
{
  print_type_and_value<T>(v);
}

#define PTV(V) do {                                                     \
    std::cout << #V << " = "; print_type_and_value<decltype(V)>(V);     \
  } while (false)


template <class... Args>
auto f1(Args&&... args) requires (sig1.is_viable<Args...>())
{
  auto [p1, p2, z, x, y] = sig1.get_param_values(std::forward<Args>(args)...);

  TEST_TYPE_IS(p1, const double&);
  TEST_TYPE_IS(p2, const char*);
  TEST_TYPE_IS(z , std::string);
  TEST_TYPE_IS(x , int);
  TEST_TYPE_IS(y , int&&);

#ifdef TRACE  // For debugging only
  std::cout << "\nf1 argument values:\n";
  PTV(p1);
  PTV(p2);
  PTV(z );
  PTV(x );
  PTV(y );
#endif

  return std::tuple{ p1, p2, z, x, y };
}

int main()
{
  {
    double d = 1.2;
    auto [p1, p2, z, x, y] = sig1.get_param_values(d, "a", "b",
                                                   "y"_arg(99),
                                                   "x"_arg(short(9)));

    TEST_TYPE_IS(p1, const double&);
    TEST_TYPE_IS(p2, const char*);
    TEST_TYPE_IS(z , std::string);
    TEST_TYPE_IS(x , int);
    TEST_TYPE_IS(y , int&&);

    d = 3.4;  // Ensure that the actual reference was captured
    TEST_EQ(std::tuple(3.4, "a", "b", 9, 99), std::tuple(p1, p2, z, x, y));
  }
  {
    constexpr double d = 1.2;
    auto [p1, p2, z, x, y] = sig1.get_param_values(d, "a",
                                                   "y"_arg(99),
                                                   "x"_arg(short(9)));

    TEST_TYPE_IS(p1, const double&);
    TEST_TYPE_IS(p2, const char*);
    TEST_TYPE_IS(z , std::string);
    TEST_TYPE_IS(x , int);
    TEST_TYPE_IS(y , int&&);

    TEST_EQ(std::tuple(1.2, "a", "hello", 9, 99), std::tuple(p1, p2, z, x, y));
  }

  TEST_EQ(std::tuple(1.2, "a", "b", 9, 99),
          f1(1.2,  "a", "b", "y"_arg(99), "x"_arg(short(9))));

  TEST_EQ(std::tuple(3.4, "c", "hello", 8, 88),
          f1(3.4,  "c", "x"_arg(short(8)), "y"_arg(88)));

  TEST_EQ(std::tuple(5.6, "d", "bye", 7, 5),
          f1(5.6,  "d", "x"_arg(7), "z"_arg("bye")));

  return error_count;
}


// Local Variables:
// c-basic-offset: 2
// End:

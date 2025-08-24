/* designated_param_sim.t.cpp                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <designated_param_sim.h>
#include <string>

namespace dp = designated_params;
using namespace dp::literals;

constexpr auto h = "hello"_des;
constexpr auto x = "x"_des;

int default_x = 14;  // Not a compile-time constant

// Parameter declarations
using xparam = dp::dp<int, "x"_des>;          // int .x
using yparam = dp::dp<int&&, "y"_des, 5>;     // int&& .y = 5
using pp1    = dp::pp<double>;                // double a
using pp2    = dp::pp<const char*, nullptr>;  // const char b = nullptr
using zparam = dp::dpp<std::string, "z"_des,  // std::string .z = "hello"
  [] -> std::string { return "hello"; }>;

// Designated-argument types
using xarg   = decltype("x"_arg(short(9)));   // .x = short(9)
using nxarg  = decltype("x"_arg(nullptr));    // .x = nullptr
using yarg   = decltype("y"_arg(99));         // .y = 99
using zarg   = decltype("z"_arg("bye"));      // .z = "bye"

// White-box incremental testing
namespace test_details {

using namespace dp::details;

// test `has_no_default`
static_assert(  has_no_default<xparam>::value);
static_assert(! has_no_default<yparam>::value);

// test `match_des`
static_assert(  match_des<"x"_des>::apply<xparam>::value);
static_assert(  match_des<"y"_des>::apply<yparam>::value);
static_assert(! match_des<"x"_des>::apply<yparam>::value);

// basic test for `match_da`
static_assert(  match_da<type_list<>, type_list<>>());
static_assert(! match_da<type_list<>, type_list<xarg>>());
static_assert(! match_da<type_list<xparam>, type_list<>>());
static_assert(  match_da<type_list<yparam>, type_list<>>());
static_assert(  match_da<type_list<xparam>, type_list<xarg>>());
static_assert(! match_da<type_list<xparam>, type_list<nxarg>>());
static_assert(! match_da<type_list<xparam>, type_list<yarg>>());
static_assert(! match_da<type_list<xparam, yparam>, type_list<yarg>>());
static_assert(  match_da<type_list<xparam, yparam>, type_list<xarg>>());
static_assert(  match_da<type_list<xparam, yparam>,
                type_list<xarg, yarg>>());
static_assert(! match_da<type_list<xparam, yparam>,
              type_list<nxarg, yarg>>());

// test parameter list
using params = type_list<pp1, pp2, zparam, xparam, yparam>;

// basic test for `match_pa`
static_assert(! match_pa<params, type_list<>>());
static_assert(! match_pa<params, type_list<int>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg, yarg>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4],
                                           xarg, yarg>>());
static_assert(  match_pa<params, type_list<float, xarg, yarg, zarg>>());
static_assert(  match_pa<params, type_list<float, yarg, xarg, zarg>>());
static_assert(! match_pa<params, type_list<yarg, xarg, zarg>>());
static_assert(! match_pa<params, type_list<float, yarg, zarg>>());
static_assert(  match_pa<params, type_list<float, xarg, zarg>>());
static_assert(! match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg, yarg, zarg>>()); // dup z

}

constexpr auto params1 =
  dp::func_signature<pp1, pp2, zparam, xparam, yparam>{};

static_assert(! params1.is_viable<>());
static_assert(! params1.is_viable<int>());
static_assert(  params1.is_viable<int, const char(&)[4], const char*,
                                  xarg, yarg>());
static_assert(  params1.is_viable<int, const char(&)[4], xarg, yarg>());
static_assert(  params1.is_viable<float, xarg, yarg, zarg>());
static_assert(  params1.is_viable<float, yarg, xarg, zarg>());
static_assert(! params1.is_viable<yarg, xarg, zarg>());
static_assert(! params1.is_viable<float, yarg, zarg>());
static_assert(  params1.is_viable<float, xarg, zarg>());
static_assert(! params1.is_viable<int, const char(&)[4], const char*,
                                  xarg, yarg, zarg>()); // dup z

int main()
{
}

// Local Variables:
// c-basic-offset: 2
// End:

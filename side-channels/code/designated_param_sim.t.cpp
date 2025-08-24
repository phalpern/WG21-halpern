/* designated_param_sim.t.cpp                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <designated_param_sim.h>

namespace dp = designated_params;
using namespace dp::literals;

constexpr auto h = "hello"_des;
constexpr auto x = "x"_des;

int default_x = 14;  // Not a compile-time constant

int f(dp::dp<int, "x"_des, [](){ return default_x; }> a = {})
{
  return a.get();
}

const char* f(dp::dpp<const char (&)[], "x"_des> a)
{
  return a.get();
}

int main()
{
  auto fx5 = f("x"_arg(5));
  // auto fy5 = f("y"_arg(5));   // Arg mismatch
  auto fn  = f();

  std::cout << fx5 << ", " << fn << std::endl;
  std::cout << f("hello") << ", " << f("x"_arg("goodbye")) << std::endl;
}

// Local Variables:
// c-basic-offset: 2
// End:

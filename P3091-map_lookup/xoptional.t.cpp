/* xoptional.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <xoptional.h>
#include <cassert>

namespace xstd = std::experimental;

int main()
{
  const int zero  = 0;
  int       one   = 1;
  int       three = 3;

  xstd::optional<int> iopt1;
  assert(! iopt1.has_value());
  assert(iopt1.value_or(0) == 0);
  assert(iopt1.or_construct(0) == 0);
  assert( iopt1.or_construct<const int&>(zero) == 0);
  assert(&iopt1.or_construct<const int&>(zero) == &zero);
  assert( iopt1.or_construct<int&>(one) == 1);
  assert(&iopt1.or_construct<int&>(one) == &one);

  iopt1 = 3;
  assert(iopt1.has_value());
  assert(*iopt1 == 3);
  assert(iopt1.value_or(0) == 3);
  assert(iopt1.or_construct(0) == 3);
  assert( iopt1.or_construct<const int&>(zero) == 3);
  assert(&iopt1.or_construct<const int&>(zero) == &*iopt1);
  assert( iopt1.or_construct<int&>(one) == 3);
  assert(&iopt1.or_construct<int&>(one) == &*iopt1);

  xstd::optional<int&> iropt1;
  assert(! iropt1.has_value());
  assert(iropt1.value_or(zero) == 0);
  assert(&iropt1.value_or(zero) == &zero);
  assert(iropt1.or_construct(0) == 0);
  assert( iropt1.or_construct<const int&>(zero) == 0);
  assert(&iropt1.or_construct<const int&>(zero) == &zero);
  assert( iropt1.or_construct<int&>(one) == 1);
  assert(&iropt1.or_construct<int&>(one) == &one);

  iropt1 = three;
  assert(iropt1.has_value());
  assert(*iropt1 == 3);
  assert(iropt1.value_or(zero) == 3);
  assert(&iropt1.value_or(zero) == &three);
  assert(iropt1.or_construct(0) == 3);
  assert( iropt1.or_construct<const int&>(zero) == 3);
  assert(&iropt1.or_construct<const int&>(zero) == &three);
  assert( iropt1.or_construct<int&>(one) == 3);
  assert(&iropt1.or_construct<int&>(one) == &three);
}

// Local Variables:
// c-basic-offset: 2
// End:

/* xoptional.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <xoptional.h>
#include <cassert>

// Usage: expect<type>(value, theMap.get(key).or_construct(def));
template <class EXP_T, class VAL_T, class TEST_T>
void expect(const VAL_T& exp, TEST_T&& v)
{
  static_assert(std::is_same_v<EXP_T, TEST_T>, "Wrong return type");
  assert(exp == v);
}

namespace xstd = std::experimental;

int main()
{
  const int zero  = 0;
  int       one   = 1;
  int       three = 3;

  xstd::optional<int> iopt1;
  assert(! iopt1.has_value());
  expect<      int >(0,      iopt1.value_or(0));
  expect<      int >(0,      iopt1.or_construct(0));
  expect<const int&>(0,      iopt1.or_construct<const int&>(zero));
  expect<const int*>(&zero, &iopt1.or_construct<const int&>(zero));
  expect<      int&>(1,      iopt1.or_construct<      int&>(one));
  expect<      int*>(&one,  &iopt1.or_construct<      int&>(one));

  iopt1 = 3;
  assert(iopt1.has_value());
  expect<      int&>(3,       *iopt1);
  expect<      int >(3,        iopt1.value_or(0));
  expect<      int >(3,        iopt1.or_construct(0));
  expect<const int&>(3,        iopt1.or_construct<const int&>(zero));
  expect<const int*>(&*iopt1, &iopt1.or_construct<const int&>(zero));
  expect<      int&>(3,        iopt1.or_construct<      int&>(one));
  expect<      int*>(&*iopt1, &iopt1.or_construct<      int&>(one));

  xstd::optional<int&> iropt1;
  assert(! iropt1.has_value());
  expect<const int&>(0,      iropt1.value_or(zero));
  expect<const int*>(&zero, &iropt1.value_or(zero));
  expect<      int >(0,      iropt1.or_construct(0));
  expect<const int&>(0,      iropt1.or_construct<const int&>(zero));
  expect<const int*>(&zero, &iropt1.or_construct<const int&>(zero));
  expect<      int&>(1,      iropt1.or_construct<      int&>(one));
  expect<      int*>(&one,  &iropt1.or_construct<      int&>(one));

  iropt1 = three;
  assert(iropt1.has_value());
  expect<      int&>(3,      *iropt1);
  expect<const int&>(3,       iropt1.value_or(zero));
  expect<const int*>(&three, &iropt1.value_or(zero));
  expect<      int >(3,       iropt1.or_construct(0));
  expect<const int&>(3,       iropt1.or_construct<const int&>(zero));
  expect<const int*>(&three, &iropt1.or_construct<const int&>(zero));
  expect<      int&>(3,       iropt1.or_construct<      int&>(one));
  expect<      int*>(&three, &iropt1.or_construct<      int&>(one));

  expect<const int&>(3, xstd::optional<const int&>(iropt1).value_or(zero));
}

// Local Variables:
// c-basic-offset: 2
// End:

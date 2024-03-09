/* xoptional.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <xoptional.h>
#include <vector>
#include <cassert>

// Usage: expect<type>(value, theMap.get(key).or_construct(def));
template <class EXP_T, class VAL_T, class TEST_T>
void expect(const VAL_T& exp, TEST_T&& v)
{
  static_assert(std::is_same_v<EXP_T, TEST_T>, "Wrong return type");
  assert(exp == v);
}

namespace xstd = std::experimental;

struct IntRefProxy
{
  int m_val = 0;

  operator int&()             { return m_val; }
  operator const int&() const { return m_val; }
};

int main()
{
  const int zero  = 0;
  int       one   = 1;
  int       three = 3;

  xstd::optional<int> iopt1;
  const xstd::optional<int>& IOPT1 = iopt1;
  assert(! iopt1.has_value());
  expect<      int >(0,      iopt1.value_or(0));
  expect<      int >(0,      iopt1.or_construct(0));
  expect<      int >(0,      iopt1.or_construct(IntRefProxy{}));
  expect<const int&>(0,      iopt1.or_construct<const int&>(zero));
//  expect<const int&>(0,      iopt1.or_construct<const int&>(IntRefProxy{}));
  expect<const int*>(&zero, &iopt1.or_construct<const int&>(zero));
  expect<      int&>(1,      iopt1.or_construct<      int&>(one));
  expect<      int*>(&one,  &iopt1.or_construct<      int&>(one));
//  expect<const int&>(0,      iopt1.or_construct<const int&>(zero, 0));
  expect<      int >(0,      IOPT1.value_or(0));
  expect<      int >(0,      IOPT1.or_construct(0));
  expect<const int&>(0,      IOPT1.or_construct<const int&>(zero));
  expect<const int*>(&zero, &IOPT1.or_construct<const int&>(zero));

  iopt1 = 3;
  assert(iopt1.has_value());
  expect<      int&>(3,       *iopt1);
  expect<      int >(3,        iopt1.value_or(0));
  expect<      int >(3,        iopt1.or_construct(0));
  expect<const int&>(3,        iopt1.or_construct<const int&>(zero));
  expect<const int*>(&*iopt1, &iopt1.or_construct<const int&>(zero));
  expect<      int&>(3,        iopt1.or_construct<      int&>(one));
  expect<      int*>(&*iopt1, &iopt1.or_construct<      int&>(one));
  expect<const int&>(3,       *IOPT1);
  expect<      int >(3,        IOPT1.value_or(0));
  expect<      int >(3,        IOPT1.or_construct(0));
  expect<const int&>(3,        IOPT1.or_construct<const int&>(zero));
  expect<const int*>(&*iopt1, &IOPT1.or_construct<const int&>(zero));

  xstd::optional<int&> iropt1;
  IntRefProxy          irp;
  assert(! iropt1.has_value());
  expect<const int&>(0,      iropt1.value_or(zero));
  expect<const int*>(&zero, &iropt1.value_or(zero));
// expect<      int&>(0,      iropt1.value_or(IntRefProxy{}));
// expect<      int&>(0,      iropt1.value_or(irp));
  expect<      int >(0,      iropt1.or_construct(0));
  expect<const int&>(0,      iropt1.or_construct<const int&>(zero));
//  expect<const int&>(0,      iropt1.or_construct<const int&>(0));
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

  xstd::optional<const int&>        ciropt1;
  const xstd::optional<const int&>& CIROPT1 = ciropt1;
  assert(! CIROPT1.has_value());
  expect<const int&>(0,      CIROPT1.value_or(zero));
  expect<const int*>(&zero, &CIROPT1.value_or(zero));
  expect<      int >(0,      CIROPT1.or_construct(0));
  expect<const int&>(0,      CIROPT1.or_construct<const int&>(zero));
  expect<const int*>(&zero, &CIROPT1.or_construct<const int&>(zero));

  ciropt1 = three;
  assert(CIROPT1.has_value());
  expect<const int&>(3,      *CIROPT1);
  expect<const int&>(3,       CIROPT1.value_or(zero));
  expect<const int*>(&three, &CIROPT1.value_or(zero));
  expect<      int >(3,       CIROPT1.or_construct(0));
  expect<const int&>(3,       CIROPT1.or_construct<const int&>(zero));
  expect<const int*>(&three, &CIROPT1.or_construct<const int&>(zero));

  using int_vec = std::vector<int>;
  xstd::optional<int_vec> ov;
  expect<int_vec>(int_vec{1, 2, 3}, ov.or_construct({1, 2, 3}));
//  expect<int_vec&>(int_vec{1, 2, 3}, ov.or_construct<int_vec&>({1, 2, 3}));

  xstd::optional<int_vec&>        orv;
  const xstd::optional<int_vec&>& ORV = orv;
  expect<int_vec>(int_vec{1, 2, 3}, ORV.or_construct({1, 2, 3}));
  int_vec v98{9, 8};
  expect<int_vec*>(&v98, &ORV.or_construct<int_vec&>(v98));
}

// Local Variables:
// c-basic-offset: 2
// End:

/* xoptional.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <xoptional.h>
#include <vector>
#include <cassert>

// Usage: expect<type>(value, theMap.get(key).value_or(def));
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

constexpr int prval() { return 0; }  // Generate an int prvalue

template <class Opt, class Ref>
void test_value_or()
{
  using Val      = int;
  using ConstVal = const Val;
  using ConstRef = ConstVal&;                // Might be reference to const
  using Ptr      = std::remove_reference_t<Ref>*;
  using ConstPtr = ConstVal*;                // Might be pointer to const

  int       lval  = 1;  // modifiable int lvalue
  const int clval = 2;  // const int lvalue
  short     slval = 3;  // non-int lvalue

  int       exp   = 4;  // expected value of engaged optional

  ///////////// DISENGAGED /////////////
  {
    Opt obj;
    assert(! obj.has_value());

    // No expicit return type
    expect<     Val>(prval(), obj.value_or(prval()));
    expect<     Val>(lval,    obj.value_or(lval));
    expect<     Val>(clval,   obj.value_or(clval));
    expect<     Val>(slval,   obj.value_or(slval));

    // Explicit rvalue return type
    expect<    long>(prval(), obj.template value_or<    long>(prval()));
    expect<    long>(lval,    obj.template value_or<    long>(lval));
    expect<    long>(clval,   obj.template value_or<    long>(clval));
    expect<    long>(slval,   obj.template value_or<    long>(slval));

    // Explicit lvalue (might or might not be const)
    expect<     Ref>( lval,   obj.template value_or<     Ref>(lval));
    expect<     Ptr>(&lval,  &obj.template value_or<     Ref>(lval));
    if constexpr (std::is_const_v<Ref>)
    {
      // If Ref is const lvalue, then try with const lvalue arg
      expect<     Ref>( clval,  obj.template value_or<     Ref>(clval));
      expect<     Ptr>(&clval, &obj.template value_or<     Ref>(clval));
    }

    // Explicit const lvalue
    expect<ConstRef>( lval,   obj.template value_or<ConstRef>(lval));
    expect<ConstPtr>(&lval,  &obj.template value_or<ConstRef>(lval));
    expect<ConstRef>( clval,  obj.template value_or<ConstRef>(clval));
    expect<ConstPtr>(&clval, &obj.template value_or<ConstRef>(clval));

#ifdef NEGATIVE_TEST
    // These should all fail to compile

    // Can't return lvalue reference to rvalue
    expect<     Ref>(prval(), obj.template value_or<     Ref>(prval()));
    expect<ConstRef>(prval(), obj.template value_or<ConstRef>(prval()));

    // Can't convert reference from one lvalue to another
    expect<     Ref>(slval,   obj.template value_or<     Ref>(slval));
    expect<ConstRef>(slval,   obj.template value_or<ConstRef>(slval));
#endif // NEGATIVE_TEST
  }

  ///////////// ENGAGED /////////////
  {
    Opt obj(exp);
    assert(obj.has_value());

    const int* expPtr =
      std::is_reference_v<typename Opt::value_type> ? &exp : &*obj;

    // No expicit return type
    expect<     Val>(exp,     obj.value_or(prval()));
    expect<     Val>(exp,     obj.value_or(lval));
    expect<     Val>(exp,     obj.value_or(clval));
    expect<     Val>(exp,     obj.value_or(slval));

    // Explicit rvalue return type
    expect<    long>(exp,     obj.template value_or<    long>(prval()));
    expect<    long>(exp,     obj.template value_or<    long>(lval));
    expect<    long>(exp,     obj.template value_or<    long>(clval));
    expect<    long>(exp,     obj.template value_or<    long>(slval));

    // Explicit lvalue
    expect<     Ref>(exp,     obj.template value_or<     Ref>(lval));
    expect<     Ptr>(expPtr, &obj.template value_or<     Ref>(lval));
    if constexpr (std::is_const_v<Ref>)
    {
      // If Ref is const lvalue, then try with const lvalue arg
      expect<     Ref>(exp,     obj.template value_or<     Ref>(clval));
      expect<     Ptr>(expPtr, &obj.template value_or<     Ref>(clval));
    }

    // Explicit const lvalue
    expect<ConstRef>(exp,     obj.template value_or<ConstRef>(lval));
    expect<ConstPtr>(expPtr, &obj.template value_or<ConstRef>(lval));
    expect<ConstRef>(exp,     obj.template value_or<ConstRef>(clval));
    expect<ConstPtr>(expPtr, &obj.template value_or<ConstRef>(clval));

#ifdef NEGATIVE_TEST
    // These should all fail to compile

    // Can't return lvalue reference to rvalue
    expect<     Ref>(exp,     obj.template value_or<     Ref>(prval()));
    expect<ConstRef>(exp,     obj.template value_or<ConstRef>(prval()));

    // Can't convert reference from one lvalue to another
    expect<     Ref>(exp,     obj.template value_or<     Ref>(slval));
    expect<ConstRef>(exp,     obj.template value_or<ConstRef>(slval));
#endif // NEGATIVE_TEST
  }
}

int main()
{
  // Test `value_or` with one argument
  test_value_or<      xstd::optional<int       >, int      &>();
  test_value_or<const xstd::optional<int       >, int const&>();
  test_value_or<      xstd::optional<int      &>, int      &>();
  test_value_or<      xstd::optional<int const&>, int const&>();
  test_value_or<const xstd::optional<int      &>, int      &>();
  test_value_or<const xstd::optional<int const&>, int const&>();

  using int_vec = std::vector<int>;
  xstd::optional<int_vec> ov;

  // Test `value_or` with multiple arguments
  expect<int_vec>(int_vec{3, 3, 3}, ov.value_or(3, 3));

  // Test `value_or` with initializer list argument
  expect<int_vec>(int_vec{1, 2, 3}, ov.value_or({1, 2, 3}));
#ifdef NEGATIVE_TEST
  expect<int_vec&>(int_vec{1, 2, 3}, ov.value_or<int_vec&>({1, 2, 3}));
#endif

  xstd::optional<int_vec&>        orv;
  const xstd::optional<int_vec&>& ORV = orv;
  expect<int_vec>(int_vec{1, 2, 3}, ORV.value_or({1, 2, 3}));
  int_vec v98{9, 8};
  expect<int_vec*>(&v98, &ORV.value_or<int_vec&>(v98));
}

// Local Variables:
// c-basic-offset: 2
// End:

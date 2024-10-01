/* xoptional.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <xoptional.h>
#include <vector>
#include <cassert>

// Usage: expect<expected-type>(expected-value, expr);
template <class EXP_T, class VAL_T, class TEST_T>
void expect(const VAL_T& exp, TEST_T&& v)
{
  static_assert(std::is_same_v<EXP_T, TEST_T>, "Wrong expression type");
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
  constexpr bool IsRefT = std::is_reference_v<typename Opt::value_type>;

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
    expect<     Val>(prval(), value_or(obj, prval()));
    expect<     Val>(lval,    value_or(obj, lval));
    expect<     Val>(clval,   value_or(obj, clval));
    expect<     Val>(slval,   value_or(obj, slval));
    expect<     Val>(prval(), value_or(Opt(), prval()));
    expect<     Val>(lval,    value_or(Opt(), lval));
    expect<     Val>(clval,   value_or(Opt(), clval));
    expect<     Val>(slval,   value_or(Opt(), slval));

    // Explicit rvalue return type
    expect<    long>(prval(), value_or<    long>(obj, prval()));
    expect<    long>(lval,    value_or<    long>(obj, lval));
    expect<    long>(clval,   value_or<    long>(obj, clval));
    expect<    long>(slval,   value_or<    long>(obj, slval));
    expect<    long>(prval(), value_or<    long>(Opt(), prval()));
    expect<    long>(lval,    value_or<    long>(Opt(), lval));
    expect<    long>(clval,   value_or<    long>(Opt(), clval));
    expect<    long>(slval,   value_or<    long>(Opt(), slval));

    // Explicit lvalue (might or might not be const)
    expect<     Ref>( lval,   value_or<     Ref>(obj, lval));
    expect<     Ptr>(&lval,  &value_or<     Ref>(obj, lval));
    if constexpr (std::is_const_v<Ref>)
    {
      // If Ref is const lvalue, then try with const lvalue arg
      expect<     Ref>( clval,  value_or<     Ref>(obj, clval));
      expect<     Ptr>(&clval, &value_or<     Ref>(obj, clval));
    }
    if constexpr (IsRefT)
    {
      expect<     Ref>( lval,  value_or<     Ref>(Opt(), lval));
      expect<     Ptr>(&lval, &value_or<     Ref>(Opt(), lval));
    }

    // Explicit const lvalue
    expect<ConstRef>( lval,   value_or<ConstRef>(obj, lval));
    expect<ConstPtr>(&lval,  &value_or<ConstRef>(obj, lval));
    expect<ConstRef>( clval,  value_or<ConstRef>(obj, clval));
    expect<ConstPtr>(&clval, &value_or<ConstRef>(obj, clval));
    if constexpr (IsRefT)
    {
      expect<ConstRef>( lval,   value_or<ConstRef>(Opt(), lval));
      expect<ConstPtr>(&lval,  &value_or<ConstRef>(Opt(), lval));
      expect<ConstRef>( clval,  value_or<ConstRef>(Opt(), clval));
      expect<ConstPtr>(&clval, &value_or<ConstRef>(Opt(), clval));
    }
  }

  ///////////// ENGAGED /////////////
  {
    Opt obj(exp);
    assert(obj.has_value());

    const int* expPtr = IsRefT ? &exp : &*obj;

    // No expicit return type
    expect<     Val>(exp,     value_or(obj, prval()));
    expect<     Val>(exp,     value_or(obj, lval));
    expect<     Val>(exp,     value_or(obj, clval));
    expect<     Val>(exp,     value_or(obj, slval));
    expect<     Val>(exp,     value_or(Opt(exp), prval()));
    expect<     Val>(exp,     value_or(Opt(exp), lval));
    expect<     Val>(exp,     value_or(Opt(exp), clval));
    expect<     Val>(exp,     value_or(Opt(exp), slval));

    // Explicit rvalue return type
    expect<    long>(exp,     value_or<    long>(obj, prval()));
    expect<    long>(exp,     value_or<    long>(obj, lval));
    expect<    long>(exp,     value_or<    long>(obj, clval));
    expect<    long>(exp,     value_or<    long>(obj, slval));
    expect<    long>(exp,     value_or<    long>(Opt(exp), prval()));
    expect<    long>(exp,     value_or<    long>(Opt(exp), lval));
    expect<    long>(exp,     value_or<    long>(Opt(exp), clval));
    expect<    long>(exp,     value_or<    long>(Opt(exp), slval));

    // Explicit lvalue
    expect<     Ref>(exp,     value_or<     Ref>(obj, lval));
    expect<     Ptr>(expPtr, &value_or<     Ref>(obj, lval));
    if constexpr (std::is_const_v<Ref>)
    {
      // If Ref is const lvalue, then try with const lvalue arg
      expect<     Ref>(exp,     value_or<     Ref>(obj, clval));
      expect<     Ptr>(expPtr, &value_or<     Ref>(obj, clval));
    }
    if constexpr (IsRefT)
    {
      expect<     Ref>(exp,     value_or<     Ref>(Opt(exp), lval));
      expect<     Ptr>(expPtr, &value_or<     Ref>(Opt(exp), lval));
    }

    // Explicit const lvalue
    expect<ConstRef>(exp,     value_or<ConstRef>(obj, lval));
    expect<ConstPtr>(expPtr, &value_or<ConstRef>(obj, lval));
    expect<ConstRef>(exp,     value_or<ConstRef>(obj, clval));
    expect<ConstPtr>(expPtr, &value_or<ConstRef>(obj, clval));
    if constexpr (IsRefT)
    {
      expect<ConstRef>(exp,     value_or<ConstRef>(Opt(exp), lval));
      expect<ConstPtr>(expPtr, &value_or<ConstRef>(Opt(exp), lval));
      expect<ConstRef>(exp,     value_or<ConstRef>(Opt(exp), clval));
      expect<ConstPtr>(expPtr, &value_or<ConstRef>(Opt(exp), clval));
    }

// #define NEGATIVE_TEST
#ifdef NEGATIVE_TEST
    // These should all fail to compile

    if constexpr (! IsRefT)
    {
      // Can't return lvalue reference to object within rvalue `optional`
      (void) value_or<Ref>(Opt(), lval);
      (void) value_or<ConstRef>(Opt(), lval);
    }

    // Can't return lvalue reference to rvalue
    (void) value_or<     Ref>(obj, prval());
    (void) value_or<ConstRef>(obj, prval());

    // Can't convert reference from one lvalue to another
    (void) value_or<     Ref>(obj, slval);
    (void) value_or<ConstRef>(obj, slval);
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

  // Test that rvalue reference return is possible
  (void) value_or(xstd::optional<int>(), prval());

  using int_vec  = std::vector<int>;
  xstd::optional<int_vec> ov;

  // Test `value_or` with multiple arguments
  expect<int_vec>(int_vec{2, 2, 2}, value_or(ov, 3, 2));

  // Test `value_or` with initializer list argument
  expect<int_vec>(int_vec{1, 2, 3}, value_or(ov, {1, 2, 3}));
#ifdef NEGATIVE_TEST
  expect<int_vec&>(int_vec{1, 2, 3}, value_or<int_vec&>(ov, {1, 2, 3}));
#endif

  xstd::optional<int_vec&>        orv;
  const xstd::optional<int_vec&>& ORV = orv;
  expect<int_vec>(int_vec{1, 2, 3}, value_or(ORV, {1, 2, 3}));
  int_vec v98{9, 8};
  expect<int_vec*>(&v98, &value_or<int_vec&>(ORV, v98));
}

// Local Variables:
// c-basic-offset: 2
// End:

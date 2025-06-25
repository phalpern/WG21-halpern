/* tests.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Use tests for P3091 Better lookups for `map` and `unordered_map`
 */

#include <xmap.h>
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <span>
#include <functional>
#include <cassert>

template <class Key, class T, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using xmap = std::experimental::map<Key, T, Compare, Allocator>;

template <class T> using xoptional = std::experimental::optional<T>;


// Usage: expect<type>(value, theMap.get(key).value_or(def));
template <class EXP_T, class VAL_T, class TEST_T>
void expect(const VAL_T& exp, TEST_T&& v)
{
  static_assert(std::is_same_v<EXP_T, TEST_T>, "Wrong return type");
  assert(exp == v);
}

class NotDefaultConstructible
{
  // This class does not have a default ctor
  int m_value;

public:
  explicit NotDefaultConstructible(int v) : m_value(v) { }

  int value() const { return m_value; }

  friend std::strong_ordering operator<=>(NotDefaultConstructible a,
                                          NotDefaultConstructible b) = default;

  friend bool operator==(NotDefaultConstructible a, int b)
    { return a.value() == b; }
};

class NonCopyable
{
  // This class does not have a default ctor
  int m_value;

public:
  explicit NonCopyable(int v) : m_value(v) { }

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;

  int value() const { return m_value; }

  friend std::strong_ordering operator<=>(const NonCopyable& a,
                                          const NonCopyable& b) = default;
};

bool operator==(const NonCopyable& a, int b) { return a.value() == b;}

class NonCopyableDerived : public NonCopyable
{
public:
  explicit NonCopyableDerived(int v) : NonCopyable(v) { }
};

void test_get()
{
  using std::experimental::value_or;

  {
    xmap<std::string, std::string>        m1;
    xmap<std::string, std::string> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    expect<std::string>("world",     value_or(M1.get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(M1.get("goodbye"), "everybody"));
    // (void) M1["hello"]);  // Does not compile

    expect<std::string>("world",     value_or(M1.get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(M1.get("goodbye"), "everybody"));
    expect<std::string>("",          value_or(M1.get("goodbye")));

    // Works on nonconst map
    expect<std::string>("world",     value_or(m1.get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(m1.get("goodbye"), "everybody"));

    // equivalent, but could modify map and returns by reference:
    expect<std::string&>("world",    m1["hello"]);

    // Works on rvalue map
    expect<std::string>("world",     value_or(std::move(m1).get("hello"),  "everybody"));
    expect<std::string>("everybody", value_or(std::move(m1).get("goodbye"),"everybody"));

    // Does not modify map, even when value was not found
    assert(1 == M1.size());
    assert("hello" == M1.begin()->first);
    assert("world" == M1.begin()->second);
  }

  {
    xmap<int, NotDefaultConstructible>        m2;
    xmap<int, NotDefaultConstructible> const &M2 = m2;
    m2.try_emplace(0, 5);
    assert(1 == M2.size());

    // (void) m2[0];       // Does not compile
    // (void) m2.get(0);   // Does not compile
    expect<NotDefaultConstructible>( 5, value_or<NotDefaultConstructible>(M2.get(0), 99));
    expect<NotDefaultConstructible>(99, value_or<NotDefaultConstructible>(M2.get(1), 99));
  }

  // Use std::less<void>, to validate `is_transparent` metaprogramming
  {
    xmap<std::string, std::string, std::less<>>        m1;
    xmap<std::string, std::string, std::less<>> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    expect<std::string>("world",     value_or(M1.get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(M1.get("goodbye"), "everybody"));
    // (void) M1["hello"]);  // Does not compile

    // Works on nonconst map
    expect<std::string>("world",     value_or(m1.get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(m1.get("goodbye"), "everybody"));

    // equivalent, but could modify map and returns by reference:
    expect<std::string&>("world",    m1["hello"]);

    // Works on rvalue map
    expect<std::string>("world",     value_or(std::move(m1).get("hello"), "everybody"));
    expect<std::string>("everybody", value_or(std::move(m1).get("goodbye"), "everybody"));

    // Does not modify map, even when value was not found
    assert(1 == M1.size());
    assert("hello" == M1.begin()->first);
    assert("world" == M1.begin()->second);
  }

  {
    xmap<int, NotDefaultConstructible, std::less<>>        m2;
    xmap<int, NotDefaultConstructible, std::less<>> const &M2 = m2;
    m2.try_emplace(0, 5);
    assert(1 == M2.size());

    // (void) m2[0];       // Does not compile
    // (void) m2.get(0);   // Does not compile
    expect<NotDefaultConstructible>( 5, value_or<NotDefaultConstructible>(M2.get(0), 99));
    expect<NotDefaultConstructible>(99, value_or<NotDefaultConstructible>(M2.get(1), 99));
  }
}

void test_get_ref()
{
  using std::experimental::value_or;

  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string& world = m3["hello"]; const std::string& WORLD = world;
    std::string  dummy("dummy");      const std::string& DUMMY = dummy;

    // Test basic `get_ref` functionality
    expect<      std::string&>("world", value_or<std::string&>(m3.get("hello"), dummy));
    expect<      std::string&>("dummy", value_or<std::string&>(m3.get("goodbye"), dummy));
    expect<const std::string&>("world", value_or<const std::string&>(M3.get("hello"), dummy));
    expect<const std::string&>("dummy", value_or<const std::string&>(M3.get("goodbye"), dummy));
    expect<const std::string&>("world", value_or<const std::string&>(m3.get("hello"), DUMMY));
    expect<const std::string&>("dummy", value_or<const std::string&>(m3.get("goodbye"), DUMMY));
    expect<const std::string&>("world", value_or<const std::string&>(M3.get("hello"), DUMMY));
    expect<const std::string&>("dummy", value_or<const std::string&>(M3.get("goodbye"), DUMMY));

    // Verify that the address of the return value is correct
    expect<      std::string*>(&WORLD, &value_or<std::string&>(m3.get("hello"), dummy));
    expect<      std::string*>(&DUMMY, &value_or<std::string&>(m3.get("goodbye"), dummy));
    expect<const std::string*>(&WORLD, &value_or<const std::string&>(M3.get("hello"), dummy));
    expect<const std::string*>(&DUMMY, &value_or<const std::string&>(M3.get("goodbye"), dummy));
    expect<const std::string*>(&WORLD, &value_or<const std::string&>(m3.get("hello"), DUMMY));
    expect<const std::string*>(&DUMMY, &value_or<const std::string&>(m3.get("goodbye"), DUMMY));
    expect<const std::string*>(&WORLD, &value_or<const std::string&>(M3.get("hello"), DUMMY));
    expect<const std::string*>(&DUMMY, &value_or<const std::string&>(M3.get("goodbye"), DUMMY));

    // The following are unsafe and deliberately yield a warning or error:
    // (void) value_or(m3.get("goodbye"), "dummy");
    // (void) value_or(std::string(m3.get("goodbye"), "dummy"));
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) value_or(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable&>(ZERO, value_or<NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), zero));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(m4.get(1), ZERO));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable*>(&ZERO, &value_or<NonCopyable&>(m4.get(1), zero));

    assert(33 == value_or<NonCopyable&>(m4.get(3), zero).value());
    assert(0  == value_or<NonCopyable&>(m4.get(1), zero).value());
  }

  // Repeat with std::less<void> to test `is_transparent` metaprogramming
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string  dummy("dummy");

    // Test basic `get_ref` functionality
    expect<      std::string&>("world", value_or<std::string&>(m3.get("hello"), dummy));
    expect<      std::string&>("dummy", value_or<std::string&>(m3.get("goodbye"), dummy));
    expect<const std::string&>("world", value_or<const std::string&>(M3.get("hello"), dummy));
    expect<const std::string&>("dummy", value_or<const std::string&>(M3.get("goodbye"), dummy));
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyable, std::less<>>        m4;
    xmap<int, NonCopyable, std::less<>> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) value_or<NonCopyable&>(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable&>(ZERO, value_or<NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), zero));
  }
}

void test_get_ref_derived()
{
  // Pass derived as second argument
  {
    NonCopyableDerived zero{0}; const NonCopyableDerived& ZERO = zero;

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) value_or<const NonCopyable&>(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable&>(ZERO, value_or<NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), zero));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(m4.get(1), ZERO));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable*>(&ZERO, &value_or<NonCopyable&>(m4.get(1), zero));

    assert(33 == value_or<NonCopyable&>(m4.get(3), zero).value());
    assert(0  == value_or<NonCopyable&>(m4.get(1), zero).value());
  }

  // Use derived as mapped-to type
  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyableDerived>        m4;
    xmap<int, NonCopyableDerived> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) value_or<const NonCopyable&>(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable&>(ZERO, value_or<NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), zero));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), zero));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(m4.get(1), ZERO));
    expect<const NonCopyable&>(E3  , value_or<const NonCopyable&>(M4.get(3), ZERO));
    expect<const NonCopyable&>(ZERO, value_or<const NonCopyable&>(M4.get(1), ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &value_or<NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable*>(&ZERO, &value_or<NonCopyable&>(m4.get(1), zero));

    assert(33 == value_or<NonCopyable&>(m4.get(3), zero).value());
    assert(0  == value_or<NonCopyable&>(m4.get(1), zero).value());
  }

}

void test_get_as()
{
  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string_view dummy("dummy");

    assert("world" == value_or<std::string_view>(m3.get("hello"), "dummy"));
    assert("dummy" == value_or<std::string_view>(m3.get("goodbye"), "dummy"));
    assert("dummy" == value_or<std::string_view>(m3.get("goodbye"), dummy));
  }

  // Repeat with std::less<void>
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string_view dummy("dummy");

    assert("world" == value_or<std::string_view>(m3.get("hello"), "dummy"));
    assert("dummy" == value_or<std::string_view>(m3.get("goodbye"), "dummy"));
    assert("dummy" == value_or<std::string_view>(m3.get("goodbye"), dummy));
  }

  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    auto q1 = value_or<xoptional<std::string&>>(m3.get("hello"));
    assert(q1);
    expect<std::string&>("world", q1.value());

    auto q2 = value_or<xoptional<std::string&>>(m3.get("badkey"));
    assert(! q2);
  }

}

void test_get_as_ref()
{
  using string = std::string;

  {
    xmap<string, string>        m3;
    xmap<string, string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    string& world = m3["hello"]; const string& WORLD = world;
    string  dummy("dummy");      const string& DUMMY = dummy;

    expect<      string*>(&WORLD, &value_or<string&>(m3.get("hello"), dummy));
    expect<      string*>(&DUMMY, &value_or<string&>(m3.get("goodbye"), dummy));

    expect<      string&>(WORLD, value_or<string&>(m3.get("hello"), dummy));
    expect<      string&>(DUMMY, value_or<string&>(m3.get("goodbye"), dummy));
    expect<const string&>(WORLD, value_or<const string&>(m3.get("hello"), dummy));
    expect<const string&>(DUMMY, value_or<const string&>(m3.get("goodbye"), dummy));
    expect<const string&>(WORLD, value_or<const string&>(M3.get("hello"), dummy));
    expect<const string&>(DUMMY, value_or<const string&>(M3.get("goodbye"), dummy));
    expect<const string&>(WORLD, value_or<const string&>(m3.get("hello"), DUMMY));
    expect<const string&>(DUMMY, value_or<const string&>(m3.get("goodbye"), DUMMY));
    expect<const string&>(WORLD, value_or<const string&>(M3.get("hello"), DUMMY));
    expect<const string&>(DUMMY, value_or<const string&>(M3.get("goodbye"), DUMMY));

    // Shouldn't compile (const mismatch)
    // (void) value_or<string&>(m3.get("hello"), DUMMY);
    // (void) value_or<string&>(M3.get("hello"), dummy);
    // (void) value_or<string&>(M3.get("hello"), DUMMY);

    // The following are unsafe and would yield a warning or deliberate error:
    // (void) value_or<const string&>(m3.get("goodbye"), "dummy");
    // (void) value_or<const string&>(string(m3.get("goodbye"), "dummy"));
  }

  {
    NonCopyableDerived zero{0}; const NonCopyableDerived& ZERO = zero;

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyable& e3 = M4.at(3);

    // (void) value_or<NonCopyable&>(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable*>(&e3  , &value_or<      NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable*>(&zero, &value_or<      NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(e3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(zero, value_or<const NonCopyable&>(m4.get(1), ZERO));

    // Shouldn't compile
    // (void) value_or<      NonCopyable&>(m4.get(3), ZERO);
    // (void) value_or<      NonCopyable&>(M4.get(3), zero);
    // (void) value_or<NonCopyableDerived&>(m4.get(3), zero);

    assert(33 == value_or<NonCopyable&>(m4.get(3), zero).value());
    assert(0  == value_or<NonCopyable&>(m4.get(1), zero).value());
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyableDerived, std::less<>>        m4;
    xmap<int, NonCopyableDerived, std::less<>> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyableDerived& e3 = M4.at(3);

    // (void) value_or(m4.get(3), zero);      // shouldn't compile
    expect<      NonCopyable*>(&e3  , &value_or<      NonCopyable&>(m4.get(3), zero));
    expect<      NonCopyable*>(&zero, &value_or<      NonCopyable&>(m4.get(1), zero));
    expect<const NonCopyable&>(e3  , value_or<const NonCopyable&>(m4.get(3), ZERO));
    expect<const NonCopyable&>(zero, value_or<const NonCopyable&>(m4.get(1), ZERO));

    // Shouldn't compile
    // (void) value_or<       NonCopyable&>(m4.get(3), zero);
    // (void) value_or<       NonCopyable&>(M4.get(3), zero);
    // (void) value_or<NonCopyableDerived&>(m4.get(3), zero);
  }
}

void test_span()
{
  using K = int;
  using U = float;

  xmap<K, std::vector<U>> m{ { 99, { 9.8f, 10.9f } }, { 55, { 5.5f, 4.4f } } };
  std::array preset{ 1.2f, 3.4f, 5.6f };

  auto x = xoptional<std::span<U>>(m.get(0)).value_or(preset);
  assert(3 == x.size());
  assert(3.4f == x[1]);

  auto y = xoptional<std::span<U>>(m.get(99)).value_or(preset);
  assert(2 == y.size());

  auto z = xoptional<std::span<U>>(m.get(0)).value_or({});
  assert(0 == z.size());
}

// Test constexpr `get`
constexpr std::experimental::ArrayMap<int, 3> AM1({ 3, 2, 1 });
static_assert(AM1.get(1));

static_assert(2 == AM1.get(1).value());
static_assert(! AM1.get(10));

int main()
{
  test_get();
  test_get_ref();
  test_get_ref_derived();
  test_get_as();
  test_get_as_ref();
  test_span();
}

// Local Variables:
// c-basic-offset: 2
// End:

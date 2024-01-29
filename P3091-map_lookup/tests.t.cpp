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
#include <cassert>

template <class Key, class T, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using xmap = std::experimental::map<Key, T, Compare, Allocator>;

// Usage: expect<type>(value, theMap.get(key, def));
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
  {
    xmap<std::string, std::string>        m1;
    xmap<std::string, std::string> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    expect<std::string>("world",     M1.get("hello", "everybody"));
    expect<std::string>("everybody", M1.get("goodbye", "everybody"));
    // (void) M1["hello"]);  // Does not compile

    // Works on nonconst map
    expect<std::string>("world",     m1.get("hello", "everybody"));
    expect<std::string>("everybody", m1.get("goodbye", "everybody"));

    // equivalent, but could modify map and returns by reference:
    expect<std::string&>("world",    m1["hello"]);

    // Works on rvalue map
    expect<std::string>("world",     std::move(m1).get("hello", "everybody"));
    expect<std::string>("everybody", std::move(m1).get("goodbye","everybody"));

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
    expect<NotDefaultConstructible>( 5, M2.get(0, 99));
    expect<NotDefaultConstructible>(99, M2.get(1, 99));
  }

  // Use std::less<void>, to validate `is_transparent` metaprogramming
  {
    xmap<std::string, std::string, std::less<>>        m1;
    xmap<std::string, std::string, std::less<>> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    expect<std::string>("world",     M1.get("hello", "everybody"));
    expect<std::string>("everybody", M1.get("goodbye", "everybody"));
    // (void) M1["hello"]);  // Does not compile

    // Works on nonconst map
    expect<std::string>("world",     m1.get("hello", "everybody"));
    expect<std::string>("everybody", m1.get("goodbye", "everybody"));

    // equivalent, but could modify map and returns by reference:
    expect<std::string&>("world",    m1["hello"]);

    // Works on rvalue map
    expect<std::string>("world",     std::move(m1).get("hello", "everybody"));
    expect<std::string>("everybody", std::move(m1).get("goodbye","everybody"));

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
    expect<NotDefaultConstructible>( 5, M2.get(0, 99));
    expect<NotDefaultConstructible>(99, M2.get(1, 99));
  }
}

void test_get_ref()
{
  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string& world = m3["hello"]; const std::string& WORLD = world;
    std::string  dummy("dummy");      const std::string& DUMMY = dummy;

    // Test basic `get_ref` functionality
    expect<      std::string&>("world", m3.get_ref("hello",   dummy));
    expect<      std::string&>("dummy", m3.get_ref("goodbye", dummy));
    expect<const std::string&>("world", M3.get_ref("hello",   dummy));
    expect<const std::string&>("dummy", M3.get_ref("goodbye", dummy));
    expect<const std::string&>("world", m3.get_ref("hello",   DUMMY));
    expect<const std::string&>("dummy", m3.get_ref("goodbye", DUMMY));
    expect<const std::string&>("world", M3.get_ref("hello",   DUMMY));
    expect<const std::string&>("dummy", M3.get_ref("goodbye", DUMMY));

    // Verify that the address of the return value is correct
    expect<      std::string*>(&WORLD, &m3.get_ref("hello",   dummy));
    expect<      std::string*>(&DUMMY, &m3.get_ref("goodbye", dummy));
    expect<const std::string*>(&WORLD, &M3.get_ref("hello",   dummy));
    expect<const std::string*>(&DUMMY, &M3.get_ref("goodbye", dummy));
    expect<const std::string*>(&WORLD, &m3.get_ref("hello",   DUMMY));
    expect<const std::string*>(&DUMMY, &m3.get_ref("goodbye", DUMMY));
    expect<const std::string*>(&WORLD, &M3.get_ref("hello",   DUMMY));
    expect<const std::string*>(&DUMMY, &M3.get_ref("goodbye", DUMMY));

    // The following are unsafe and deliberately yield a warning or error:
    // (void) m3.get_ref("goodbye", "dummy");
    // (void) m3.get_ref("goodbye", std::string("dummy"));
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , m4.get_ref(3, zero));
    expect<      NonCopyable&>(ZERO, m4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, zero));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , m4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, m4.get_ref(1, ZERO));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &m4.get_ref(3, zero));
    expect<      NonCopyable*>(&ZERO, &m4.get_ref(1, zero));

    assert(33 == m4.get_ref(3, zero).value());
    assert(0  == m4.get_ref(1, zero).value());
  }

  // Repeat with std::less<void> to test `is_transparent` metaprogramming
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string  dummy("dummy");

    // Test basic `get_ref` functionality
    expect<      std::string&>("world", m3.get_ref("hello",   dummy));
    expect<      std::string&>("dummy", m3.get_ref("goodbye", dummy));
    expect<const std::string&>("world", M3.get_ref("hello",   dummy));
    expect<const std::string&>("dummy", M3.get_ref("goodbye", dummy));
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyable, std::less<>>        m4;
    xmap<int, NonCopyable, std::less<>> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , m4.get_ref(3, zero));
    expect<      NonCopyable&>(ZERO, m4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , m4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, zero));
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

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , m4.get_ref(3, zero));
    expect<      NonCopyable&>(ZERO, m4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, zero));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , m4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, m4.get_ref(1, ZERO));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &m4.get_ref(3, zero));
    expect<      NonCopyable*>(&ZERO, &m4.get_ref(1, zero));

    assert(33 == m4.get_ref(3, zero).value());
    assert(0  == m4.get_ref(1, zero).value());
  }

  // Use derived as mapped-to type
  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyableDerived>        m4;
    xmap<int, NonCopyableDerived> const& M4 = m4;

    const NonCopyable& E3 = m4.emplace(3, 33).first->second;

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable&>(E3  , m4.get_ref(3, zero));
    expect<      NonCopyable&>(ZERO, m4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, zero));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, zero));
    expect<const NonCopyable&>(E3  , m4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, m4.get_ref(1, ZERO));
    expect<const NonCopyable&>(E3  , M4.get_ref(3, ZERO));
    expect<const NonCopyable&>(ZERO, M4.get_ref(1, ZERO));

    // Check addresses
    expect<      NonCopyable*>(&E3  , &m4.get_ref(3, zero));
    expect<      NonCopyable*>(&ZERO, &m4.get_ref(1, zero));

    assert(33 == m4.get_ref(3, zero).value());
    assert(0  == m4.get_ref(1, zero).value());
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

    assert("world" == m3.get_as<std::string_view>("hello", "dummy"));
    assert("dummy" == m3.get_as<std::string_view>("goodbye", "dummy"));
    assert("dummy" == m3.get_as<std::string_view>("goodbye", dummy));
  }

  // Repeat with std::less<void>
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string_view dummy("dummy");

    assert("world" == m3.get_as<std::string_view>("hello", "dummy"));
    assert("dummy" == m3.get_as<std::string_view>("goodbye", "dummy"));
    assert("dummy" == m3.get_as<std::string_view>("goodbye", dummy));
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

    expect<      string*>(&WORLD, &m3.get_as<string&>("hello", dummy));
    expect<      string*>(&DUMMY, &m3.get_as<string&>("goodbye", dummy));

    expect<      string&>(WORLD, m3.get_as<string&>("hello", dummy));
    expect<      string&>(DUMMY, m3.get_as<string&>("goodbye", dummy));
    expect<const string&>(WORLD, m3.get_as<const string&>("hello", dummy));
    expect<const string&>(DUMMY, m3.get_as<const string&>("goodbye", dummy));
    expect<const string&>(WORLD, M3.get_as<const string&>("hello", dummy));
    expect<const string&>(DUMMY, M3.get_as<const string&>("goodbye", dummy));
    expect<const string&>(WORLD, m3.get_as<const string&>("hello", DUMMY));
    expect<const string&>(DUMMY, m3.get_as<const string&>("goodbye", DUMMY));
    expect<const string&>(WORLD, M3.get_as<const string&>("hello", DUMMY));
    expect<const string&>(DUMMY, M3.get_as<const string&>("goodbye", DUMMY));

    // Shouldn't compile (const mismatch)
    // (void) m3.get_as<string&>("hello", DUMMY);
    // (void) M3.get_as<string&>("hello", dummy);
    // (void) M3.get_as<string&>("hello", DUMMY);

    // The following are unsafe and would yield a warning or deliberate error:
    // (void) m3.get_as<const string&>("goodbye", "dummy");
    // (void) m3.get_as<const string&>("goodbye", string("dummy"));
  }

  {
    NonCopyableDerived zero{0}; const NonCopyableDerived& ZERO = zero;

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyable& e3 = M4.at(3);

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable*>(&e3  , &m4.get_as<      NonCopyable&>(3, zero));
    expect<      NonCopyable*>(&zero, &m4.get_as<      NonCopyable&>(1, zero));
    expect<const NonCopyable&>(e3  , m4.get_as<const NonCopyable&>(3, ZERO));
    expect<const NonCopyable&>(zero, m4.get_as<const NonCopyable&>(1, ZERO));

    // Shouldn't compile
    // (void) m4.get_as<      NonCopyable&>(3, ZERO);
    // (void) M4.get_as<      NonCopyable&>(3, zero);
    // (void) m4.get_as<NonCopyableDerived&>(3, zero);

    assert(33 == m4.get_as<NonCopyable&>(3, zero).value());
    assert(0  == m4.get_as<NonCopyable&>(1, zero).value());
  }

  {
    NonCopyable zero{0}; const NonCopyable& ZERO = zero;

    xmap<int, NonCopyableDerived, std::less<>>        m4;
    xmap<int, NonCopyableDerived, std::less<>> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyableDerived& e3 = M4.at(3);

    // (void) m4.get(3, zero);      // shouldn't compile
    expect<      NonCopyable*>(&e3  , &m4.get_as<      NonCopyable&>(3, zero));
    expect<      NonCopyable*>(&zero, &m4.get_as<      NonCopyable&>(1, zero));
    expect<const NonCopyable&>(e3  , m4.get_as<const NonCopyable&>(3, ZERO));
    expect<const NonCopyable&>(zero, m4.get_as<const NonCopyable&>(1, ZERO));

    // Shouldn't compile
    // (void) m4.get_as<      NonCopyable&>(3, ZERO);
    // (void) M4.get_as<      NonCopyable&>(3, zero);
    // (void) m4.get_as<NonCopyableDerived&>(3, zero);
  }
}

int main()
{
  test_get();
  test_get_ref();
  test_get_ref_derived();
  test_get_as();
  test_get_as_ref();
}

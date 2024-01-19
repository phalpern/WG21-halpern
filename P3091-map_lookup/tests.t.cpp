/* use_cases.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Use cases/tests for PXXXX Non-mutating lookups for `map` and `unordered_map`
 */

#include <xmap.h>
#include <string>
#include <iostream>
#include <cassert>

template <class Key, class T, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using xmap = std::experimental::map<Key, T, Compare, Allocator>;

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

  friend bool operator==(NonCopyable a, int b)
    { return a.value() == b; }
};

void test_get()
{
  {
    xmap<std::string, std::string>        m1;
    xmap<std::string, std::string> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    assert("world"     == M1.get("hello", "everybody"));
    assert("everybody" == M1.get("goodbye", "everybody"));
    // assert("world" == M1["hello"]);  // Does not compile

    // Works on nonconst map
    assert("world"     == m1.get("hello", "everybody"));
    assert("everybody" == m1.get("goodbye", "everybody"));
    assert("world"     == m1["hello"]);  // equivalent, but could modify map

    // Works on rvalue map
    assert("world"     == std::move(m1).get("hello", "everybody"));
    assert("everybody" == std::move(m1).get("goodbye", "everybody"));

    // Does not modify map, even when value was not found
    assert(1 == M1.size());
    assert("hello" == M1.begin()->first);
    assert(""      == M1.begin()->second);  // Was moved from
  }

  {
    xmap<int, NotDefaultConstructible>        m2;
    xmap<int, NotDefaultConstructible> const &M2 = m2;
    m2.try_emplace(0, 5);
    assert(1 == M2.size());

    // assert(5 == m2[0]);       // Does not compile
    // assert(5 == m2.get(0));   // Does not compile
    assert( 5 == M2.get(0, 99));
    assert(99 == M2.get(1, 99));
  }

  // Repeat all of the above tests with std::less<void>
  {
    xmap<std::string, std::string, std::less<>>        m1;
    xmap<std::string, std::string, std::less<>> const& M1 = m1;
    m1.emplace("hello", "world");
    assert(1 == M1.size());

    // Works on const map
    assert("world"     == M1.get("hello", "everybody"));
    assert("everybody" == M1.get("goodbye", "everybody"));
    // assert("world" == M1["hello"]);  // Does not compile

    // Works on nonconst map
    assert("world"     == m1.get("hello", "everybody"));
    assert("everybody" == m1.get("goodbye", "everybody"));
    assert("world"     == m1["hello"]);  // equivalent, but could modify map

    // Works on rvalue map
    assert("world"     == std::move(m1).get("hello", "everybody"));
    assert("everybody" == std::move(m1).get("goodbye", "everybody"));

    // Does not modify map, even when value was not found
    assert(1 == M1.size());
    assert("hello" == M1.begin()->first);
    assert(""      == M1.begin()->second);  // Was moved from
  }

  {
    xmap<int, NotDefaultConstructible, std::less<>>        m2;
    xmap<int, NotDefaultConstructible, std::less<>> const &M2 = m2;
    m2.try_emplace(0, 5);
    assert(1 == M2.size());

    // assert(5 == m2[0]);       // Does not compile
    // assert(5 == m2.get(0));   // Does not compile
    assert( 5 == M2.get(0, 99));
    assert(99 == M2.get(1, 99));
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
  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string& world = m3["hello"];
    std::string  dummy("dummy");

    assert(&world == &m3.get_as<std::string&>("hello", dummy));
    assert(&dummy == &m3.get_as<std::string&>("goodbye", dummy));
    // The following are unsafe and would yield a warning or deliberate error:
    // (void) m3.get_as<const std::string&>("goodbye", "dummy");
    // (void) m3.get_as<const std::string&>("goodbye", std::string("dummy"));
  }

  {
    NonCopyable zero{0};

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyable& e3 = M4.at(3);

    // (void) m4.get(3, zero);      // Won't compile
    assert(&e3   == &m4.get_as<NonCopyable&>(3, zero));
    assert(&zero == &m4.get_as<NonCopyable&>(1, zero));

    assert(33 == m4.get_as<NonCopyable&>(3, zero).value());
    assert(0  == m4.get_as<NonCopyable&>(1, zero).value());
  }

  // Repeat with std::less<void>
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    std::string& world = m3["hello"];
    std::string  dummy("dummy");

    assert(&world == &m3.get_as<std::string&>("hello", dummy));
    assert(&dummy == &m3.get_as<std::string&>("goodbye", dummy));
    // The following are unsafe and would yield a warning or deliberate error:
    (void) m3.get_as<std::string&>("goodbye", "dummy");
    (void) m3.get_as<std::string&>("goodbye", std::string("dummy"));
  }
}

void test_get_as_const_ref()
{
  {
    xmap<std::string, std::string>        m3;
    xmap<std::string, std::string> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    const std::string& world = m3["hello"];
    const std::string  dummy("dummy");

    assert(&world == &M3.get_as<const std::string&>("hello", dummy));
    assert(&dummy == &M3.get_as<const std::string&>("goodbye", dummy));
    // The following are unsafe and would yield a warning or deliberate error:
    // (void) M3.get_as<const std::string&>("goodbye", "dummy");
    // (void) M3.get_as<const std::string&>("goodbye", std::string("dummy"));
  }

  {
    const NonCopyable zero{0};

    xmap<int, NonCopyable>        m4;
    xmap<int, NonCopyable> const& M4 = m4;
    m4.emplace(3, 33);
    const NonCopyable& e3 = M4.at(3);

    // (void) m4.get(3, zero);      // Won't compile
    assert(&e3   == &M4.get_as<const NonCopyable&>(3, zero));
    assert(&zero == &M4.get_as<const NonCopyable&>(1, zero));

    assert(33 == M4.get_as<const NonCopyable&>(3, zero).value());
    assert(0  == M4.get_as<const NonCopyable&>(1, zero).value());
  }

  // Repeat with std::less<void>
  {
    xmap<std::string, std::string, std::less<>>        m3;
    xmap<std::string, std::string, std::less<>> const& M3 = m3;
    m3.emplace("hello", "world");
    assert(1 == M3.size());

    const std::string& world = m3["hello"];
    const std::string  dummy("dummy");

    assert(&world == &M3.get_as<const std::string&>("hello", dummy));
    assert(&dummy == &M3.get_as<const std::string&>("goodbye", dummy));
    // The following are unsafe and would yield a warning or deliberate error:
    // (void) M3.get_as<const std::string&>("goodbye", "dummy");
    // (void) M3.get_as<const std::string&>("goodbye", std::string("dummy"));
  }
}

int main()
{
  test_get();
  test_get_as();
  test_get_as_ref();
  test_get_as_const_ref();
}

/* use_cases.t.cpp                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Use cases for P3091 Better lookups for `map` and `unordered_map`
 */

#include <xmap.h>
#include <string>
#include <iostream>
#include <utility>
#include <cassert>
#include <limits>
#include <vector>
#include <string_view>
#include <span>
#include <optional>

namespace xstd = std::experimental;

class NotDefaultConstructible
{
  // This class does not have a default ctor
  int m_value;

public:
  explicit NotDefaultConstructible(int v) : m_value(v) { }

  int value() const { return m_value; }

  friend std::strong_ordering operator<=>(NotDefaultConstructible a,
                                          NotDefaultConstructible b) = default;
};

bool operator==(NotDefaultConstructible a, int b) { return a.value() == b; }

void nonZero()
{
  xstd::map<int, unsigned> data = {
    { 0, 10 },
    { 4, 8 },
    { 5, 2 },
    { 8, 6 },
    { 11, 9 }
  };

  assert(data.size() == 5);

  // Find the minimum of value for odd-numbered keys
  unsigned smallest = 100;
  for (int i = 1; i < 15; i += 2)
    smallest = std::min(smallest, value_or(data.get(i), 100));

  assert(data.size() == 5);  // No new items added
  assert(2 == smallest);     // Correctly computed smallest value
}

void constMap()
{
  const xstd::map<const char*, int> m{ { "one", 1 }, { "two", 2}, { "three", 3 }};

//  int v = m["two"];  // Won't compile
  int v = value_or(m.get("two"));  // OK

  assert(2 == v);
}

void noDefaultCtor()
{
  xstd::map<std::string, NotDefaultConstructible> m;

  m.try_emplace("hello", 5);

//  auto e = m["hello"];          // Won't compile
  auto e = value_or<NotDefaultConstructible>(m.get("hello"), 99);  // OK

  assert(5 == e);
}

struct Person
{
  std::string first_name;
  std::string last_name;
  std::string address;

  friend bool operator==(const Person&, const Person&) = default;
};

void bigValue()
{
  const Person nobody{};

  xstd::map<unsigned, Person> id_to_person;
  unsigned id = 0;

  Person        p1 = value_or(id_to_person.get(id));    // Copies element
  Person const& p2 = value_or<Person const&>(id_to_person.get(id), nobody);  // No copies made

  assert(p1.first_name.empty());
  assert(&p1 != &nobody);
  assert(p2.first_name.empty());
  assert(&p2 == &nobody);

  // Person const& p3 = value_or(id_to_person.get(id), Person{});  // Won't compile
}

void convertedValue()
{
  const xstd::map<int, std::string> m{
    { 1, "one one one one one one one one one one one one one" },
    { 2, "two two two two two two two two two two two two two" },
    { 3, "three three three three three three three three three" }
  };

  std::string_view sv1 = value_or(m.get(1), "none");  // Dangling reference
  std::string_view sv2 = value_or<std::string_view>(m.get(2), "none");  // OK
  std::string_view sv3 = value_or<std::string_view>(m.get(5), "nonex", 4);  // OK

  (void) sv1;

  // assert("one"  == sv1.substr(0, 3));  // Fails (as expected) in gcc
  assert("two"  == sv2.substr(0, 3));
  assert("none" == sv3);
}

class NotCopyAssignable
{
  int m_value;

public:
  NotCopyAssignable(int i = 0) : m_value(i) { }

  NotCopyAssignable& operator=(const NotCopyAssignable&) = delete;

  friend std::strong_ordering operator<=>(const NotCopyAssignable&,
                                          const NotCopyAssignable&) = default;

  int value() const { return m_value; }
};

struct Base { };
struct Derived : Base { };

// Examples in the paper (to test compilation and correctness)
void fromPaper()
{
  {
    std::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 1; i <= 100; ++i)
      largest = std::max(largest, theMap[i]);
    assert(0.0 == largest);  // Surprising result.  Would prefer -20.0.
    assert(101 == theMap.size());  // Lots of growth
  }

  {
    std::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 1; i <= 100; ++i)
    {
      try {
        largest = std::max(largest, theMap.at(i));
      } catch (const std::out_of_range&) { }
    }
    assert(-20.0 == largest);    // Expected result.
    assert(3 == theMap.size());  // No growth
  }

  {
    std::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 1; i <= 100; ++i)
      if (auto iter = theMap.find(i); iter != theMap.end())
        largest = std::max(largest, iter->second);

    assert(-20.0 == largest);    // Expected result.
    assert(3 == theMap.size());  // No growth
  }

  {
    std::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 1; i <= 100; ++i)
    {
      auto iter = theMap.find(i);
      if (iter != theMap.end())
        largest = std::max(largest, iter->second);
    }
    assert(-20.0 == largest);    // Expected result.
    assert(3 == theMap.size());  // No growth
  }

  {
    xstd::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 1; i <= 100; ++i)
      largest = std::max(largest, value_or(theMap.get(i), largest));
    assert(-20.0 == largest);    // Expected result.
    assert(3 == theMap.size());  // No growth
  }

  {
    xstd::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    constexpr double inf = std::numeric_limits<double>::infinity();
    double largest = -inf;
    for (int i = 1; i <= 100; ++i)
      largest = std::max(largest, value_or(theMap.get(i), -inf));
    assert(-20.0 == largest);    // Expected result.
    assert(3 == theMap.size());  // No growth
  }

  {
    xstd::map<int, double> theMap = { { 3, -20.0 }, { 90, -90.0 }, { 110, 4.0 } };
    xstd::optional<double&> largest;
    for (int i = 1; i <= 100; ++i)
      largest = std::max(largest, theMap.get(i));
    assert(-20.0 == largest.value());    // Expected result.
    assert(3 == theMap.size());          // No growth
  }

  {
    using Value = NotCopyAssignable;

    std::map<int, Value> aMap{ { 1, 2 }, { 3, 4 } };
    {
      int k = 1;
      Value obj = aMap.count(k) ? aMap.at(k) : Value(99);
      assert(obj.value() == 2);
    }
    {
      int k = 2;
      Value obj = aMap.count(k) ? aMap.at(k) : Value(99);
      assert(obj.value() == 99);
    }
    {
      int k = 1;
      auto iter = aMap.find(k);
      Value obj = iter != aMap.end() ? iter->second : Value(99);
      assert(obj.value() == 2);
    }
    {
      int k = 2;
      auto iter = aMap.find(k);
      Value obj = iter != aMap.end() ? iter->second : Value(99);
      assert(obj.value() == 99);
    }
  }

  {
    xstd::map<std::string, int> theMap = { { "hello", 2 } };
    std::vector<std::string> names = { "goodbye", "hello" };
    // ...
    // Increment the entries matching `names`, but only if they are already in `theMap`.
    for (const auto& name : names) {
      int temp = 0;  // Value is irrelevant
      ++value_or<int&>(theMap.get(name), temp);  // increment through reference
      // Possibly-modified value of `temp` is discarded here.
    }
    assert(3 == theMap.at("hello"));
    assert(0 == theMap.count("goodbye"));
  }

#if 0 // Test compilation error
  {
    const xstd::map<int, std::string> theMap{};
    const std::string& ref = value_or(theMap.get(0), "zero");  // ERROR: temporary `std::string("zero")`
  }
#endif
  {
    int key = 0;
    xstd::map<int, int> m{};
    const xstd::map<int, int>& theMap = m;

    // ...
    int alt = 0;
    auto& ref = value_or<const int&>(theMap.get(key), alt);  // `ref` has type `const int&`
    static_assert(std::is_same_v<const int&, decltype((ref))>);
  }

  {
    int key = 0;
    xstd::map<int, Derived> theMap;
    // ...
    Base alt{  };
    auto& ref = value_or<Base&>(theMap.get(key), alt);  // `ref` has type `Base&`
    static_assert(std::is_same_v<Base&, decltype((ref))>);
  }

  {
    int key = 0;
    xstd::map<int, std::string> theMap;
    // ...
    std::string_view sv = value_or<std::string_view>(theMap.get(key), "none");
    assert("none" == sv);

    // Dangling reference: convert returned temporary `string` to `string_view`
    std::string_view sv2 = value_or(theMap.get(key), "none");
    (void) sv2;

    // ERROR: cannot bind temporary `string` to `string&` parameter
    // std::string_view sv3 = value_or(theMap.get(key), "none");
  }
  {
    xstd::map<int, int> theMap;
    const int zero = 0;

    // auto& v1 = value_or<int&>(theMap.get(0), zero);       // ERROR: `const` mismatch
    auto& v2 = value_or<const int&>(theMap.get(0), zero); // OK
    (void) v2;
  }

  {
    xstd::map<int, int> theMap;
    int k = 0;

    if (auto opt = theMap.get(k); opt) {
      auto& ref = *opt;
      // ...
      (void) ref;
    }
  }

  using K = int;
  using T = int;
  using U = int;
  int k = 0;
  constexpr std::size_t N = 3;
  xstd::map<int, int> m;

  {
    auto iter = m.find(k);
    T x = iter == m.end() ? T{} : iter->second;
    (void) x;
  }
  {
    T x = value_or(m.get(k));
    (void) x;
  }

  {
    int a1 = 1;
    auto iter = m.find(k);
    T x = iter == m.end() ? T{a1} : iter->second;
    (void) x;
  }
  {
    int a1 = 1;
    T x = value_or(m.get(k), a1);
    (void) x;
  }

  {
    T v;
    auto iter = m.find(k);
    T& x = iter == m.end() ? v : iter->second;
    (void) x;
  }
  {
    T v;
    T& x = value_or<T&>(m.get(k), v);
    (void) x;
  }

  {
    xstd::map<K, std::vector<U>> m{ };
    auto iter = m.find(k);
    std::span<U> x = iter == m.end() ? std::span<U>{} : iter->second;
    (void) x;
  }
  {
    xstd::map<K, std::vector<U>> m{ };
    std::span<U> x = value_or<std::span<U>>(m.get(k));
    (void) x;
  }

  {
    xstd::map<K, std::vector<U>> m{ };
    const std::array<U, N> preset{ 1, 2, 3 };
    auto iter = m.find(k);
    std::span<const U> x = iter == m.end() ? std::span<const U>{preset} : iter->second;
    (void) x;
  }
  {
    xstd::map<K, std::vector<U>> m{ };
    const std::array<U, N> preset{ 1, 2, 3 };
    std::span<const U> x = value_or<std::span<const U>>(m.get(k), preset);
    (void) x;
  }

  {
    xstd::map<K, U*> m{  };
    auto iter = m.find(k);
    if (iter != m.end()) {
      U* p = iter->second;
      // ...
      (void) p;
    }
  }

  {
    xstd::map<K, U*> m{  };
    U* p = value_or(m.get(k), nullptr);
    if (p) {
      // ...
    }
  }

  {
    auto iter = m.find(k);
    if (iter != m.end()) {
      T& r = iter->second;
      // ...
      (void) r;
    }
  }
  {
    T not_found;
    T& r = value_or<T&>(m.get(k), not_found);
    if (&r != &not_found) {
      // ...
    }
  }

  {
    {
      int product = 9;
      xstd::map<int, int> theMap;
      product *= value_or(theMap.get(k), 1);
      assert(9 == product);
    }
  }
}

int main()
{
  nonZero();
  constMap();
  noDefaultCtor();
  bigValue();
  convertedValue();
  fromPaper();
}

// Local Variables:
// c-basic-offset: 2
// End:

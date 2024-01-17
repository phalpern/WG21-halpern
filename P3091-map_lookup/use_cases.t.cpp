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
#include <utility>
#include <cassert>

template <class Key, class T, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using xmap = std::experimental::map<Key, T, Compare, Allocator>;

using std::experimental::map;

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

void nonZero()
{
  map<int, unsigned> data = {
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
    smallest = std::min(smallest, data.get(i, 100));

  assert(data.size() == 5);  // No new items added
  assert(2 == smallest);     // Correctly computed smallest value
}

void constMap()
{
  const map<const char*, int> m{ { "one", 1 }, { "two", 2}, { "three", 3 }};

//  int v = m["two"];  // Won't compile
  int v = m.get("two");  // OK

  assert(2 == v);
}

void noDefaultCtor()
{
  map<std::string, NotDefaultConstructible> m;

  m.try_emplace("hello", 5);

//  auto e = m["hello"];          // Won't compile
  auto e = m.get("hello", 99);  // OK

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

  map<unsigned, Person> id_to_person;
  unsigned id = 0;

  Person        p1 = id_to_person.get(id);              // Copies element
  Person const& p2 = id_to_person.get_ref(id, nobody);  // No copies made

  assert(p1.first_name.empty());
  assert(&p1 != &nobody);
  assert(p2.first_name.empty());
  assert(&p2 == &nobody);

//  Person const& p3 = id_to_person.get_ref(id, Person{});  // Won't compile
}

void replaceEfficiently()
{
  map<std::string, Person, std::less<>> regions;

  Person js{ "John", "Smith", "15 Birch St." };
  Person jd{ "Jane", "Doe", "9 Whisteria Ln." };
  Person bc{ "Betty", "Crocker", "44 Baker Rd." };
  Person fr{ "Fred", "Rogers", "Neighborly Ct" };
  Person mw{ "Marth", "Washington", "Virginia Ave" };

  regions.try_emplace("Eastern", js);
  assert(js == regions["Eastern"]);

  // Doesn't replace the value
  regions.try_emplace("Eastern", jd);
  assert(js == regions["Eastern"]);

  regions["Eastern"] = jd;  // OK
  assert(jd == regions["Eastern"]);

  // OK, but default-constructs Person only to overwrite it
  regions["Western"] = bc;
  assert(bc == regions["Western"]);

  // Same effiency as `try_emplace`.  More efficient than indexing.
  regions.replace("Central", fr);
  assert(fr == regions["Central"]);

  // Same efficiency as indexing.
  regions.replace("Eastern", mw);
  assert(mw == regions["Eastern"]);
}

void replaceNoDefaultCtor()
{
  map<std::string, NotDefaultConstructible> m;

  m.try_emplace("hello", 5);

//  m["hello"] = 6;         // Won't compile
  m.replace("hello", 6);  // OK

  assert(6 == m.at("hello"));
}

int main()
{
  nonZero();
  constMap();
  noDefaultCtor();
  bigValue();
  replaceEfficiently();
  replaceNoDefaultCtor();
}

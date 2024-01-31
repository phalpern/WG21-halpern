/* tests.t.cpp                                                        -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P3091: Better lookups for `map` and `unordered_map`
 */

#include <move_with_alloc.h>
#include <iostream>
#include <memory_resource>
#include <cassert>

std::pmr::unsynchronized_pool_resource rsrc1, rsrc2;
std::pmr::polymorphic_allocator<> dfltAlloc{};
std::pmr::polymorphic_allocator<> alloc1(&rsrc1);
std::pmr::polymorphic_allocator<> alloc2(&rsrc2);

class TestClass
{
  int                               m_value;
  std::pmr::polymorphic_allocator<> m_alloc;

public:
  using allocator_type = std::pmr::polymorphic_allocator<>;

  TestClass() : m_value(0) { }
  explicit TestClass(const allocator_type& a) : m_value(), m_alloc(a) { }
  explicit TestClass(int v, const allocator_type& a = { }) : m_value(v), m_alloc(a) { }
  TestClass(const TestClass& other, const allocator_type& a = { })
    : m_value(other.value()), m_alloc(a) { std::cout << "copy\n"; }
  TestClass(TestClass&& other) : m_value(other.value()), m_alloc(other.m_alloc)
    { std::cout << "move\n"; }
  TestClass(TestClass&& other, const allocator_type& a)
    : TestClass(std::experimental::move_construct_with_allocator(other, a)) { }

  int value() const { return m_value; }
  allocator_type get_allocator() const { return m_alloc; }
};

int main()
{
  TestClass x1(99);
  assert(99        == x1.value());
  assert(dfltAlloc == x1.get_allocator());

  // Copy
  TestClass x2(x1, alloc1);
  assert(99     == x2.value());
  assert(alloc1 == x2.get_allocator());

  // Move
  TestClass x3(std::move(x2));
  assert(99     == x3.value());
  assert(alloc1 == x3.get_allocator());

  // Copy
  TestClass x4(std::move(x3), alloc2);
  assert(99     == x4.value());
  assert(alloc2 == x4.get_allocator());

  // Move
  TestClass x5(std::move(x4), alloc2);
  assert(99     == x5.value());
  assert(alloc2 == x5.get_allocator());
}

// Local Variables:
// c-basic-offset: 2
// End:

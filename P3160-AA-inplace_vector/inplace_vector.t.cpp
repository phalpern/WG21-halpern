#include <inplace_vector.h>
#include <memory_resource>
#include <cassert>

namespace xstd = std::experimental;

struct Endless
{
  Endless* next;
  int      val;
};

Endless endlessList;
Endless *endlessPtr = &endlessList;

void resetVal()
{
  endlessPtr->next = &endlessList;
  endlessPtr->val  = 1;
}

int nextVal()
{
  endlessPtr = endlessPtr->next;
  return ++endlessPtr->val;
}

template <class Tp>
void test()
{
  resetVal();

  // Construct with 10 elements
  xstd::inplace_vector<Tp, 30> iv1(10);
  assert(10 == iv1.size());

  // Push-back 10 elements
  for (int i = 0; i < 10; ++i)
    iv1.push_back(nextVal());

  assert(20 == iv1.size());

  // Copy construct
  auto iv2 = iv1;
  assert(20 == iv2.size());

  // Equality operator
  bool is_eq = (iv1 == iv2);
  assert(is_eq);

  // Test `back` and `pop_back`
  for (int i = 0; i < 5; ++i)
    if (iv2.back() != 0) iv2.pop_back();

  assert(15 == iv2.size());
}

// Non-allocator-aware test type
template <int N>
struct TestTypeNA
{
  int   m_value;

public:
  TestTypeNA() : m_value(0) { }
  TestTypeNA(int v) : m_value(v) { }  // Implicit

  TestTypeNA(const TestTypeNA& rhs) : m_value(rhs.m_value) { }
  TestTypeNA(TestTypeNA&& rhs) : m_value(rhs.m_value) { }

  TestTypeNA& operator=(const TestTypeNA& rhs)
    { m_value = rhs.m_value; return *this; }

  int value() const { return m_value; }

  friend bool operator==(const TestTypeNA& a, const TestTypeNA& b)
    { return a.m_value == b.m_value; }
};


// Allocator-aware test type
template <int N, class Alloc = std::allocator<std::byte>>
struct TestTypeA
{
  int   m_value;
  Alloc m_allocator;

public:
  using allocator_type = Alloc;

  explicit TestTypeA(const allocator_type& a = {})
    : m_value(0), m_allocator(a) { }
  TestTypeA(int v, const allocator_type& a = {})  // Implicit
    : m_value(v), m_allocator(a) { }

  TestTypeA(const TestTypeA& rhs, const allocator_type& a = {})
    : m_value(rhs.m_value), m_allocator(a) { }

  TestTypeA(TestTypeA&& rhs) : m_value(rhs.m_value), m_allocator() { }

  TestTypeA(TestTypeA&& rhs, const allocator_type& a)
    : m_value(rhs.m_value), m_allocator(a) { }

  TestTypeA& operator=(const TestTypeA& rhs)
    { m_value = rhs.m_value; return *this; }

  allocator_type get_allocator() const { return m_allocator; }
  int value() const { return m_value; }

  friend bool operator==(const TestTypeA& a, const TestTypeA& b)
    { return a.m_value == b.m_value; }
};


int main()
{
#ifndef AA_ONLY
  test<short>();     test<unsigned short>();
  test<int>();       test<unsigned int>();
  test<long>();      test<unsigned long>();
  test<long long>(); test<unsigned long long>();
  test<float>();
  test<double>();
  test<long double>();

  test<TestTypeNA<1>>();
  test<TestTypeNA<2>>();
  test<TestTypeNA<3>>();
  test<TestTypeNA<4>>();
  test<TestTypeNA<5>>();
  test<TestTypeNA<6>>();
  test<TestTypeNA<7>>();
  test<TestTypeNA<8>>();
  test<TestTypeNA<9>>();
  test<TestTypeNA<10>>();
#endif // ! AA_ONLY

#ifndef NON_AA_ONLY
  test<TestTypeA<1>>();
  test<TestTypeA<1, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<2>>();
  test<TestTypeA<2, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<3>>();
  test<TestTypeA<3, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<4>>();
  test<TestTypeA<4, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<5>>();
  test<TestTypeA<5, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<6>>();
  test<TestTypeA<6, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<7>>();
  test<TestTypeA<7, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<8>>();
  test<TestTypeA<8, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<9>>();
  test<TestTypeA<9, std::pmr::polymorphic_allocator<>>>();
  test<TestTypeA<10>>();
  test<TestTypeA<10, std::pmr::polymorphic_allocator<>>>();
#endif // ! NON_AA_ONLY
}

// Local Variables:
// c-basic-offset: 2
// End:

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

  xstd::inplace_vector<Tp, 30> iv1(10);
  assert(10 == iv1.size());

  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());
  iv1.push_back(nextVal());

  assert(20 == iv1.size());

  auto iv2 = iv1;
  assert(20 == iv2.size());
  assert(iv1 == iv2);

  if (iv2.back() != 0) iv2.pop_back();
  if (iv2.back() != 0) iv2.pop_back();
  if (iv2.back() != 0) iv2.pop_back();
  if (iv2.back() != 0) iv2.pop_back();
  if (iv2.back() != 0) iv2.pop_back();

  assert(15 == iv2.size());
}

template <int N, class Alloc = std::allocator<std::byte>>
struct TestType
{
  int   m_value;
  Alloc m_allocator;

public:
  using allocator_type = Alloc;

  explicit TestType(const allocator_type& a = {})
    : m_value(0), m_allocator(a) { }
  TestType(int v, const allocator_type& a = {})
    : m_value(v), m_allocator(a) { }

  TestType(const TestType& rhs, const allocator_type& a = {})
    : m_value(rhs.m_value), m_allocator(a) { }

  TestType(const TestType&& rhs)
    : m_value(rhs.m_value), m_allocator() { }

  TestType(const TestType&& rhs, const allocator_type& a)
    : m_value(rhs.m_value), m_allocator(a) { }

  TestType& operator=(const TestType& rhs)
    { m_value = rhs.m_value; return *this; }

  allocator_type get_allocator() const { return m_allocator; }
  int value() const { return m_value; }

  friend bool operator==(const TestType& a, const TestType& b)
    { return a.m_value == b.m_value; }
};


int main()
{
  test<short>();     test<unsigned short>();
  test<int>();       test<unsigned int>();
  test<long>();      test<unsigned long>();
  test<long long>(); test<unsigned long long>();
  test<float>();
  test<double>();
  test<long double>();

  test<TestType<1>>();
  test<TestType<2, std::allocator<std::byte>>>();
  test<TestType<3, std::pmr::polymorphic_allocator<>>>();
}

// Local Variables:
// c-basic-offset: 2
// End:

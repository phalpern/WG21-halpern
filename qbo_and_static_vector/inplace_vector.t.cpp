#include <inplace_vector.h>
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

  if (iv2.back() > 0) iv2.pop_back();
  if (iv2.back() > 0) iv2.pop_back();
  if (iv2.back() > 0) iv2.pop_back();
  if (iv2.back() > 0) iv2.pop_back();
  if (iv2.back() > 0) iv2.pop_back();

  assert(15 == iv2.size());
}

int main()
{
  test<short>();     test<unsigned short>();
  test<int>();       test<unsigned int>();
  test<long>();      test<unsigned long>();
  test<long long>(); test<unsigned long long>();
  test<float>();
  test<double>();
  test<long double>();
}

// Local Variables:
// c-basic-offset: 2
// End:

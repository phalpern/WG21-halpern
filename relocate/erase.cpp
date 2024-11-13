/* erase.cpp                                                          -*-C++-*-
 *
 * Implementation and test of `vector::erase` using relocation and
 * replaceability.
 */

#include <utility>
#include <algorithm>
#include <type_traits>
#include <memory>
#include <iostream>
#include <cstring>
#include <cstddef>

// Experimental stuff is in namespace `xstd`:
namespace xstd
{
using namespace std;

// Rough default implementations of `is_trivially_relocatable` and
// `is_replaceable`.  Since we don't have keyword support yet, these traits
// would need to be specialized for types that aren't trivially copy
// constructible or trivially assignable.
template <class T>
struct is_trivially_relocatable : is_trivially_copy_constructible<T> { };

template <class T>
struct is_replaceable : is_trivially_move_assignable<T> { };

template <class T>
constexpr inline bool is_trivially_relocatable_v =
  is_trivially_relocatable<T>::value;

template <class T>
constexpr inline bool is_replaceable_v = is_replaceable<T>::value;

// Almost-correct implementation of magic `trivially_relocate` function, but
// relies on UB.
template <class T>
T* trivially_relocate(T* begin, T* end, T* new_location)
{
  static_assert(is_trivially_relocatable_v<T> && ! is_const_v<T>);

  std::memmove(new_location, begin, (end - begin) * sizeof(T));
  return new_location + (end - begin);
}

template <class T>
constexpr T* relocate(T* begin, T* end, T* new_location)
{
  static_assert(is_nothrow_move_constructible_v<T> && ! is_const_v<T>);

  if consteval {
    for ( ; begin != end; ++begin, ++new_location) {
      std::construct_at(new_location, std::move(*begin));
      begin->~T();
    }
  }
  else {
    if constexpr (is_trivially_relocatable_v<T> && ! is_const_v<T>) {
      new_location = trivially_relocate(begin, end, new_location);
    }
    else {
      for ( ; begin != end; ++begin, ++new_location) {
        std::construct_at(new_location, std::move(*begin));
        begin->~T();
      }
    }
  }

  return new_location;
}

// Minimal subset of `vector` implementation needed to test `erase`.
template <class T, class Allocator = std::allocator<T>>
class vector
{
  Allocator   m_alloc    = Allocator{};
  T*          m_begin    = nullptr;
  T*          m_end      = nullptr;
  std::size_t m_capacity = 0;

public:
  using iterator       = T*;
  using const_iterator = const T*;

  constexpr iterator       begin()       { return m_begin; }
  constexpr const_iterator begin() const { return m_begin; }
  constexpr iterator       end()         { return m_end; }
  constexpr const_iterator end() const   { return m_end; }

  template <class FwdIter>
  constexpr vector(FwdIter first, FwdIter last,
                   const Allocator& alloc = Allocator());
  constexpr vector(initializer_list<T> il,
                   const Allocator& alloc = Allocator());
  constexpr ~vector();

  constexpr iterator erase(const_iterator position);
};

template <class T, class A>
template <class FwdIter>
constexpr vector<T,A>::vector(FwdIter first, FwdIter last, const A& alloc)
  : m_alloc(alloc), m_capacity(distance(first, last))
{
  m_begin = allocator_traits<A>::allocate(m_alloc, m_capacity);
  m_end   = m_begin;
  for ( ; first != last; ++first)
    allocator_traits<A>::construct(m_alloc, m_end++, *first);
}

template <class T, class A>
constexpr vector<T,A>::vector(initializer_list<T> il, const A& alloc)
  : vector(il.begin(), il.end(), alloc)
{
}

template <class T, class A>
constexpr vector<T,A>::~vector()
{
  for (auto& v : *this)
    allocator_traits<A>::destroy(m_alloc, &v);
  allocator_traits<A>::deallocate(m_alloc, m_begin, m_capacity);
}

// Here's the implemenation of `erase`, which uses trivial relocation only if
// `T` is both trivially relocatable and replaceable.
template <class T, class A>
constexpr auto vector<T,A>::erase(const_iterator position) -> iterator
{
  T* elem_addr = m_begin + (position - begin());

  if constexpr (is_trivially_relocatable_v<T> && is_replaceable_v<T>) {
    allocator_traits<A>::destroy(m_alloc, elem_addr);
    m_end = relocate(elem_addr + 1, m_end, elem_addr);
  }
  else {
    m_end = std::shift_left(elem_addr, m_end, 1);
    allocator_traits<A>::destroy(m_alloc, m_end);
  }

  return iterator(elem_addr);
}

}

// `X` is a trivially relocatable type that is not replaceable.  A message is
// printed each time it's assigned.
class X
{
  int m_v;
public:
  X(int v) : m_v(v) { }
  X(const X&) = default;
  ~X() { std::cout << "~X(" << m_v << ")\n"; }
  X& operator=(const X& rhs) {
    m_v = rhs.m_v;
    std::cout << "assign from " << rhs << '\n';
    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, const X& obj)
    { return os << "X(" << obj.m_v << ')'; }
};

// `Y` is a trivially relocatable type that *is* replaceable. A message is
// printed each time it's assigned (non-trivial assignment) but the
// `is_replaceable` trait (defined below) allows assignment to be bypassed
// if destroy+relocate is preferred.
class Y
{
  int m_v;

public:
  Y(int v) : m_v(v) { }
  Y(const Y&) = default;
  ~Y() { std::cout << "~Y(" << m_v << ")\n"; }
  Y& operator=(const Y& rhs) {
    m_v = rhs.m_v;
    std::cout << "assign from " << rhs << '\n';
    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, const Y& obj)
    { return os << "Y(" << obj.m_v << ')'; }
};

namespace xstd {
// Since we don't have the `trivally_relocatable` and `replaceable` keywords,
// we specialize the traits, instead.
template <> struct is_trivially_relocatable<X> : true_type { };
template <> struct is_trivially_relocatable<Y> : true_type { };
template <> struct is_replaceable<Y> : true_type { };
}

// Print the contents of a container.  This would be simpler using
// `std::print`, but I'm too lazy to learn how to do it.
template <class C>
void dump(const C& c)
{
  for (const auto& v : c)
    std::cout << ' ' << v;
  std::cout << std::endl;
}

// Test that `vector::erase` (and therefore `relocate`) can be evaluated in a
// `constexpr` context:
consteval int two()
{
  xstd::vector<int> r{1, 2, 3, 4};
  r.erase(r.begin());
  return *r.begin();
}

static_assert(2 == two());

int main()
{
  xstd::vector<int> vi{ 1, 2, 3, 4 };
  xstd::vector<X> vx(vi.begin(), vi.end());
  xstd::vector<Y> vy(vi.begin(), vi.end());

  // Breathing test: erase() works for `int`.
  std::cout << "\nvector<int>\n";
  std::cout << "Before erase():"; dump(vi);
  vi.erase(vi.begin() + 1);
  std::cout << "After erase(): "; dump(vi);

  // Test with non-replaceable type (uses assignment).
  std::cout << "\nvector<X>\n";
  std::cout << "Before erase():"; dump(vx);
  vx.erase(vx.begin() + 1);
  std::cout << "After erase(): "; dump(vx);

  // Test with replaceable type (skips assignment).
  std::cout << "\nvector<Y>\n";
  std::cout << "Before erase():"; dump(vy);
  vy.erase(vy.begin() + 1);
  std::cout << "After erase(): "; dump(vy);

  std::cout << "\nDestroy vectors:\n";
}

// Local Variables:
// c-basic-offset: 2
// End:

#include <iostream>
#include <utility>
#include <type_traits>
#include <memory>
#include <cstring>
#include <cassert>

#ifndef __cpp_lib_constexpr_dynamic_alloc
namespace std {

template<class T, class... Args>
constexpr T* construct_at(T* location, Args&&... args)
{
  return ::new (static_cast<void*>(location)) T(std::forward<Args>(args)...);
}

}
#endif

namespace xstd {

template <class T> class Relocator;
template <class T> class MoveRelocator;
template <class T> class TrivialRelocator { };

template <class T>
struct is_explicitly_relocatable : std::is_convertible<Relocator<T>, T> {};

template <class T>
inline constexpr bool is_explicitly_relocatable_v =
  is_explicitly_relocatable<T>::value;

template <class T>
struct is_trivially_relocatable
  : std::bool_constant<std::is_trivially_move_constructible_v<T> ||
                       std::is_convertible_v<TrivialRelocator<T>, T>>
{};

template <class T>
inline constexpr bool is_trivially_relocatable_v =
  is_trivially_relocatable<T>::value;

template <class T>
requires is_explicitly_relocatable_v<T>
inline constexpr
Relocator<T> relocate(T& v) noexcept { return Relocator<T>(v); }

template <class T>
requires std::is_trivially_move_constructible_v<T>
inline constexpr T&& relocate(T& v) noexcept { return std::move(v); }

template <class T>
inline constexpr
MoveRelocator<T> relocate(T& v) noexcept { return MoveRelocator<T>(v); }

enum class RelocateState { engaged, exploded, released };

template <class T>
class Relocator
{
  T&            m_source;
  RelocateState m_state;

public:
  constexpr explicit Relocator(T& r) noexcept
    : m_source(r), m_state(RelocateState::engaged) { }

  Relocator(const Relocator&& dr) = delete; // move-only

  Relocator(Relocator&& dr) noexcept // move-only
    : m_source(dr.m_source)
    , m_state(dr.m_state) {
    dr.m_state = RelocateState::released;
  }

  constexpr ~Relocator()
    { if (m_state == RelocateState::engaged) m_source.~T(); }

  Relocator& operator=(const Relocator&) = delete;
  Relocator& operator=(Relocator&&) = delete;

  constexpr RelocateState state() const { return m_state; }
  constexpr T& explode() {
    assert(m_state != RelocateState::released);
    m_state = RelocateState::exploded;
    return m_source;
  }


  // HIDDEN FRIENDS
  friend T& reloc_get(const Relocator& dr) {
    assert(dr.m_state != RelocateState::released);
    return dr.m_source;
  }

  friend T& release(Relocator& dr) {
    assert(dr.m_state != RelocateState::exploded);
    dr.m_state = RelocateState::released;
    return dr.m_source;
  }

  friend T& release(Relocator&& dr) { return release(dr); }

  friend constexpr Relocator relocate(Relocator& r) noexcept
    { return std::move(r); }
};

template <class T>
struct MoveRelocator : Relocator<T>
{
  using Relocator<T>::Relocator;

  operator T&&() const { return std::move(reloc_get(*this)); }

  friend T& reloc_get(const MoveRelocator& dr)
    { return reloc_get(static_cast<const Relocator<T>&>(dr)); }
  friend T& release(MoveRelocator& dr)
    { return release(static_cast<Relocator<T>&>(dr)); }

  friend constexpr MoveRelocator relocate(MoveRelocator& r) noexcept
    { return std::move(r); }
};

template <class T>
requires (! std::is_trivially_move_constructible_v<T>)
inline constexpr T& reloc_get(T& r) { return r; }

template <class T>
requires (! is_explicitly_relocatable_v<T>)
inline constexpr const T& release(const T& r) { return r; }

#define RELOC_EXPLODE(R, MEMB_NAME) xstd::relocate(R.explode().MEMB_NAME)

template <class T>
class Relocatable
{
  union {
    char m_unused;
    T    m_value;
  };

  decltype(relocate(m_value)) m_relocator;

public:
  Relocatable() : m_relocator(relocate(m_value))
    { std::construct_at(&m_value); }

  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  Relocatable(Arg&& arg) : m_relocator(relocate(m_value)) {
    std::construct_at(&m_value, std::forward<Arg>(arg));
  }

  template <class... Arg>
  explicit Relocatable(Arg&&... arg) : m_relocator(relocate(m_value)) {
    if constexpr (std::is_aggregate_v<T>)
        std::construct_at(&m_value, T{std::forward<Arg>(arg)...});
    else
      std::construct_at(&m_value, std::forward<Arg>(arg)...);
  }

  ~Relocatable() { }

  operator       T&()         { return reloc_get(m_value); }
  operator const T&()   const { return reloc_get(m_value); }
  T&       operator*()        { return reloc_get(m_value); }
  T const& operator*()  const { return reloc_get(m_value); }
  T*       operator->()       { return &reloc_get(m_value); }
  T const* operator->() const { return &reloc_get(m_value); }

  constexpr decltype(auto) do_relocate() noexcept
    { return relocate(m_relocator); }
};

template <class T>
requires std::is_trivially_move_constructible_v<T>
class Relocatable<T>
{
  T m_value;

public:
  Relocatable() = default;

  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  Relocatable(Arg&& arg) : m_value(std::forward<Arg>(arg)) { }

  template <class... Arg>
  explicit Relocatable(Arg&&... arg) : m_value{std::forward<Arg>(arg)...} { }

  operator       T&()         { return m_value; }
  operator const T&()   const { return m_value; }
  T&       operator*()        { return m_value; }
  T const& operator*()  const { return m_value; }
  T*       operator->()       { return &m_value; }
  T const* operator->() const { return &m_value; }

  constexpr T&& do_relocate() noexcept { return std::move(m_value); }
};

template <class T>
constexpr decltype(auto) relocate(Relocatable<T>& r) noexcept
{
  return r.do_relocate();
}

template <class T>
void uninitialized_relocate(T* src, T* dest, std::size_t n = 1)
  noexcept(noexcept(T(xstd::relocate(*src))))
{
  if constexpr (is_trivially_relocatable_v<T>)
    std::memcpy(dest, src, n * sizeof(*src));
  else {
    for ( ; n > 0; --n)
      std::construct_at(dest++, xstd::relocate(*src++));
  }
}

template <class T>
void uninitialized_relocate(Relocatable<T>* src, T* dest, std::size_t n = 1)
  noexcept(noexcept(T(xstd::relocate(*src))))
{
  if constexpr (std::is_trivially_move_constructible_v<T>)
    std::memcpy(dest, src, n * sizeof(*src));
  else {
    for ( ; n > 0; --n)
      if constexpr (is_trivially_relocatable_v<T>) {
        std::memcpy(dest++, &*src, sizeof(T));
        xstd::relocate(*src++);
      }
      else
        std::construct_at(dest++, xstd::relocate(*src++));
  }
}

}  // Close namespace xstd.

// Class without relocating constructor
class W
{
  W*  m_self;
  int m_data;
  // ...

public:
  explicit W(int v = 0) : m_self(this), m_data(v) {
    std::cout << "Constructing W with this = " << this
              << " and data = " << m_data << std::endl;
  }

  W(const W& other) noexcept : m_self(this), m_data(other.m_data) {
    std::cout << "Copy constructing W with this = " << this
              << " and data = " << m_data << std::endl;
  }

  W(W&& other) noexcept : m_self(this), m_data(other.m_data) {
    other.m_data = 0;
    std::cout << "Move constructing W with this = " << this
              << " and data = " << m_data << std::endl;
  }

  ~W() {
    std::cout << "Destroying W with this = " << this
              << " and data = " << m_data << std::endl;
    assert(this == m_self);
  }

  int value() const { return m_data; }
};

static_assert(! xstd::is_explicitly_relocatable_v<W>);
static_assert(! xstd::is_trivially_relocatable_v<W>);

// Class with explicit relocating constructor
class X
{
  W m_data1;
  W m_data2;
  // ...

public:
  explicit X(int v1 = 0, int v2 = 0) : m_data1(v1), m_data2(v2) {
    std::cout << "Constructing X with this = " << this
              << " and data = (" << v1 << ", " << v2 << ')' << std::endl;
  }

  X(const X& other) noexcept : m_data1(other.m_data1), m_data2(other.m_data2) {
    std::cout << "Copy constructing X with this = " << this
              << " and data = (" << m_data1.value() << ", "
              << m_data2.value() << ')' << std::endl;
  }

  X(X&& other) noexcept
    : m_data1(std::move(other.m_data1))
    , m_data2(std::move(other.m_data2)) {
    std::cout << "Move constructing X with this = " << this
              << " and data = (" << m_data1.value() << ", "
              << m_data2.value() << ')' << std::endl;
  }

  X(xstd::Relocator<X> other) noexcept
    : m_data1(RELOC_EXPLODE(other, m_data1))
    , m_data2(RELOC_EXPLODE(other, m_data2)) {
    std::cout << "Reloc constructing X with this = " << this
              << " and data = (" << m_data1.value() << ", "
              << m_data2.value() << ')' << std::endl;
  }

  ~X() {
    std::cout << "Destroying X with this = " << this
              << " and data = (" << m_data1.value() << ", "
              << m_data2.value() << ')' << std::endl;
  }
};

static_assert(  xstd::is_explicitly_relocatable_v<X>);
static_assert(! xstd::is_trivially_relocatable_v<X>);

// Trivially relocatable class
class Y
{
  int m_data1;
  int m_data2;
  // ...

public:
  explicit Y(int v1 = 0, int v2 = 0) : m_data1(v1), m_data2(v2) {
    std::cout << "Constructing Y with this = " << this
              << " and data = (" << v1 << ", " << v2 << ')' << std::endl;
  }

  Y(const Y& other) noexcept : m_data1(other.m_data1), m_data2(other.m_data2) {
    std::cout << "Copy constructing Y with this = " << this
              << " and data = (" << m_data1 << ", "
              << m_data2 << ')' << std::endl;
  }

  Y(Y&& other) noexcept : m_data1(other.m_data1), m_data2(other.m_data2) {
    other.m_data1 = other.m_data2 = 0;
    std::cout << "Move constructing Y with this = " << this
              << " and data = (" << m_data1 << ", "
              << m_data2 << ')' << std::endl;
  }

  Y(xstd::TrivialRelocator<Y> other) noexcept;

  ~Y() {
    std::cout << "Destroying Y with this = " << this
              << " and data = (" << m_data1 << ", "
              << m_data2 << ')' << std::endl;
  }

  constexpr int data1() const { return m_data1; }
  constexpr int data2() const { return m_data2; }
};

static_assert(! xstd::is_explicitly_relocatable_v<Y>);
static_assert(  xstd::is_trivially_relocatable_v<Y>);

int main()
{
  // Test scenarios:

  // Trivially moveable types
  std::cout << "Trivially movable types\n";
  {
    int x = 5;
    int y = xstd::relocate(x);
    std::cout << "y = " << y << std::endl;

    struct Aggregate { int first; double second; };
    Aggregate p1{ 8, 9.2 };
    Aggregate p2 = xstd::relocate(p1);
    std::cout << "p2 = (" << p2.first << ", " << p2.second << ")\n";

    xstd::Relocatable<Aggregate> p3{ 4, 4.4 };
    Aggregate p4 = xstd::relocate(p3);
    std::cout << "p4 = (" << p4.first << ", " << p4.second << ")\n";
  }

  // Movable type
  std::cout << std::endl << "Movable type\n";
  {
    W *pw1 = new W(1);
    W w2 = xstd::relocate(*pw1);
    ::operator delete(pw1);
  }
  {
    xstd::Relocatable<W> w3(3);
    W w4 = xstd::relocate(w3);
    std::cout << "w4 exists\n";
  }

  // Explicitly relocatable type
  std::cout << std::endl << "Explicitly relocatable type\n";
  {
    X *px1 = new X(2, 3);
    X x2 = xstd::relocate(*px1);
    ::operator delete(px1);
  }
  {
    xstd::Relocatable<X> x3(4, 5);
    X x4 = xstd::relocate(x3);
  }

  // Trivially relocatable type
  std::cout << std::endl << "Trivially relocatable type\n";
  {
    Y *py1 = new Y(2, 3);
    Y y2 = xstd::relocate(*py1);
    ::operator delete(py1);
  }
  {
    xstd::Relocatable<Y> y3(4, 5);
    Y y4 = xstd::relocate(y3);
  }
  {
    Y *py3 = new Y(4, 5);
    Y *py4 = static_cast<Y*>(::operator new(sizeof(Y)));
    xstd::uninitialized_relocate(py3, py4);
    std::cout << "Relocated Y at " << py4 << " with data = ("
              << py4->data1() << ", " << py4->data2() << ")\n";
    ::operator delete(py3);
    delete py4;
  }
  {
    xstd::Relocatable<Y> y5(6, 7);
    Y *py6 = static_cast<Y*>(::operator new(sizeof(Y)));
    xstd::uninitialized_relocate(&y5, py6);
    std::cout << "Relocated Y at " << py6 << " with data = ("
              << py6->data1() << ", " << py6->data2() << ")\n";
    delete py6;
  }
}

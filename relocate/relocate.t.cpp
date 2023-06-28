// relocate.t.cpp                                                     -*-C++-*-

#include <relocate.h>

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
    : m_data1(RELOCATE_MEMBER(other, m_data1))
    , m_data2(RELOCATE_MEMBER(other, m_data2)) {
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

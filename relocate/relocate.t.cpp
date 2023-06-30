// relocate.t.cpp                                                     -*-C++-*-

#include <relocate.h>

inline const char* typeName(const int&) { return "int"; }

template <class T>
inline const char* typeName()
{
  union A {
    char m_unused;
    T    m_obj;

    A() { }
    ~A() { }
  };

  return typeName(A{}.m_obj);
}

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

  friend const char* typeName(const W&) { return "W"; }
};

std::ostream& operator<<(std::ostream& os, const W& w)
{
  return os << w.value();
}

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

  constexpr int data1() const { return m_data1.value(); }
  constexpr int data2() const { return m_data2.value(); }

  friend const char* typeName(const X&) { return "X"; }
};

std::ostream& operator<<(std::ostream& os, const X& x)
{
  return os << '(' << x.data1() << ", " << x.data2() << ')';
}

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

  Y(xstd::TrivialRelocator<Y> original) noexcept
  {
    original.relocateTo(this); // Uses `memcpy`
  }

  ~Y() {
    std::cout << "Destroying Y with this = " << this
              << " and data = (" << m_data1 << ", "
              << m_data2 << ')' << std::endl;
  }

  constexpr int data1() const { return m_data1; }
  constexpr int data2() const { return m_data2; }

  friend const char* typeName(const Y&) { return "Y"; }
};

std::ostream& operator<<(std::ostream& os, const Y& y)
{
  return os << '(' << y.data1() << ", " << y.data2() << ')';
}

static_assert(! xstd::is_explicitly_relocatable_v<Y>);
static_assert(  xstd::is_trivially_relocatable_v<Y>);

struct Aggregate { int first; double second; };
const char* typeName(const Aggregate&) { return "Aggregate"; }
std::ostream& operator<<(std::ostream& os, const Aggregate& agg)
{
  return os << '(' << agg.first << ", " << agg.second << ')';
}

static_assert(! xstd::is_explicitly_relocatable_v<Aggregate>);
static_assert(  xstd::is_trivially_relocatable_v<Aggregate>);

template <class T, class... CtorArgs>
void TestRelocateCtor(CtorArgs&&... args)
{
  std::cout << "Testing " << typeName<T>() << " relocate ctor\n";
  T* src_p = new T{std::forward<CtorArgs>(args)...};
  std::cout << "Relocating value: " << *src_p << '\n';
  T dest = xstd::relocate(*src_p);
  std::cout << "Relocated value: " << dest << std::endl;
}

template <class T, class... CtorArgs>
void TestRelocatable(CtorArgs&&... args)
{
  std::cout << "Testing Relocatable<" << typeName<T>() << ">\n";
  xstd::Relocatable<T> src(std::forward<CtorArgs>(args)...);
  std::cout << "Relocating value: " << *src << '\n';
  T dest = xstd::relocate(src);
  std::cout << "Relocated value: " << dest << std::endl;
}

template <class T, class... CtorArgs>
void TestUninitializedRelocate(CtorArgs&&... args)
{
  std::cout << "Testing `uninitialized_relocate(" << typeName<T>()
            << "*, ...)`\n";
  T* src_p = new T{std::forward<CtorArgs>(args)...};
  std::cout << "Relocating value: " << *src_p << '\n';
  T* dest_p = static_cast<T*>(::operator new(sizeof(T)));
  xstd::uninitialized_relocate(src_p, dest_p);
  std::cout << "Relocated value: " << *dest_p << std::endl;
  ::operator delete(src_p);
  delete dest_p;

  std::cout << "Testing `uninitialized_relocate(Relocatable<" << typeName<T>()
            << ">*, ...)`\n";
  xstd::Relocatable<T> src(std::forward<CtorArgs>(args)...);
  std::cout << "Relocating value: " << *src << '\n';
  dest_p = static_cast<T*>(::operator new(sizeof(T)));
  xstd::uninitialized_relocate(&src, dest_p);
  std::cout << "Relocated value: " << *dest_p << std::endl;
  delete dest_p;
}

int main()
{
  // Test scenarios:

  // Trivially moveable types
  std::cout << "# Trivially movable types\n";
  TestRelocateCtor<int>(1);
  TestRelocatable<int>(2);
  TestUninitializedRelocate<int>(3);
  TestRelocateCtor<Aggregate>(4, 4.4);
  TestRelocatable<Aggregate>(5, 5.5);
  TestUninitializedRelocate<Aggregate>(6, 6.6);

  // Movable type
  std::cout << std::endl << "# Movable type\n";
  TestRelocateCtor<W>(7);
  TestRelocatable<W>(8);
  TestUninitializedRelocate<W>(9);

  // Explicitly relocatable type
  std::cout << std::endl << "# Explicitly relocatable type\n";
  TestRelocateCtor<X>(10, 11);
  TestRelocatable<X>(12, 13);
  TestUninitializedRelocate<X>(14, 15);

  // Trivially relocatable type
  std::cout << std::endl << "# Trivially relocatable type\n";
  TestRelocateCtor<Y>(16, 17);
  TestRelocatable<Y>(18, 19);
  TestUninitializedRelocate<Y>(20, 21);
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * End:
 */

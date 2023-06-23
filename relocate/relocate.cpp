#include <iostream>
#include <utility>
#include <type_traits>
#include <cassert>

template <class T> class DestructiveRefImp;

enum class RelocateState { engaged, exploded, released };

template <class T>
class DestructiveRefImp
{
    T&            m_source;
    RelocateState m_state;

  public:
    explicit DestructiveRefImp(T& r) noexcept
        : m_source(r), m_state(RelocateState::engaged) { }

    DestructiveRefImp(const DestructiveRefImp&& dr) = delete; // move-only

    DestructiveRefImp(DestructiveRefImp&& dr) noexcept // move-only
        : m_source(dr.source)
        , m_state(dr.m_state)
    {
        dr.m_state = RelocateState::released;
    }

    ~DestructiveRefImp()
        { if (m_state == RelocateState::engaged) m_source.~T(); }

    DestructiveRefImp& operator=(const DestructiveRefImp&) = delete;
    DestructiveRefImp& operator=(DestructiveRefImp&&) = delete;

    operator T&&() && noexcept { return std::move(release(*this)); }

    // HIDDEN FRIENDS
    friend T& get(const DestructiveRefImp& dr)
    {
        assert(dr.m_state != RelocateState::released);
        return dr.m_source;
    }

    friend T& release(DestructiveRefImp& dr)
    {
        assert(dr.m_state != RelocateState::exploded);
        dr.m_state = RelocateState::released;
        return dr.m_source;
    }

    friend T& release(DestructiveRefImp&& dr) { return release(dr); }

    template <class M>
    friend auto explode(DestructiveRefImp& v, M T::*mp) noexcept
    {
        assert(v.m_state != RelocateState::released);
        v.m_state = RelocateState::exploded;
        return relocate(get(v).*mp);
    }
};

template <class T>
using DestructiveRef = std::conditional_t<std::is_trivially_copyable_v<T>,
                                          const T&, DestructiveRefImp<T> >;

template <class T>
requires std::is_trivially_copyable_v<T>
inline const T& get(const T& r) { return r; }

template <class T>
requires std::is_trivially_copyable_v<T>
inline const T& release(const T& r) { return r; }

template <class T>
inline DestructiveRef<T> relocate(T& v) noexcept {return DestructiveRef<T>(v);}

template <class T>
inline DestructiveRefImp<T> relocate(DestructiveRefImp<T> v) noexcept
{
    return std::move(v);
}

#define RELOC_EXPLODE(DR, MEMB_NAME) \
    explode(DR, &std::decay_t<decltype(get(DR))>::MEMB_NAME)

// Class with no relocating constructor
class W
{
    W*  m_self;
    int m_data;
    // ...

  public:
    explicit W(int v = 0) : m_self(this), m_data(v)
    {
        std::cout << "Constructing W with this = " << this
                  << " and data = " << m_data << std::endl;
    }

    W(const W& other) : m_self(this), m_data(other.m_data) { /* ... */ }

    ~W()
    {
        std::cout << "Destroying W with this = " << this
                  << " and data = " << m_data << std::endl;
    }

    int value() const { return m_data; }
};


// Class with explicit relocating constructor
class X
{
    W m_data1;
    W m_data2;
    // ...

  public:
    explicit X(int v1 = 0, int v2 = 0) : m_data1(v1), m_data2(v2)
    {
        std::cout << "Constructing X with this = " << this
                  << " and data = (" << v1 << ", " << v2 << ')' << std::endl;
    }

    X(const X& other) : m_data1(other.m_data1), m_data2(other.m_data2)
    {
        std::cout << "Copy constructing X with this = " << this
                  << " and data = (" << m_data1.value() << ", "
                  << m_data2.value() << ')' << std::endl;
    }

    X(X&& other)
        : m_data1(std::move(other.m_data1)),
          m_data2(std::move(other.m_data2))
    {
        std::cout << "Move constructing X with this = " << this
                  << " and data = (" << m_data1.value() << ", "
                  << m_data2.value() << ')' << std::endl;
    }

    X(DestructiveRefImp<X>&& other)
        : m_data1(RELOC_EXPLODE(other, m_data1))
        , m_data2(RELOC_EXPLODE(other, m_data2))
    {
        std::cout << "Reloc constructing X with this = " << this
                  << " and data = (" << m_data1.value() << ", "
                  << m_data2.value() << ')' << std::endl;
    }

    ~X()
    {
        std::cout << "Destroying X with this = " << this
                  << " and data = (" << m_data1.value() << ", "
                  << m_data2.value() << ')' << std::endl;
    }
};

int main()
{
    // Test scenarios:
    {
        int x = 5;
        int y = relocate(x);
        std::cout << "y = " << y << std::endl;
    }

    {
        W *pw1 = new W(1);
        W w2 = relocate(*pw1);
        ::operator delete(pw1);
    }

    {
        X *px1 = new X(2, 3);
        X x2 = relocate(*px1);
        ::operator delete(px1);
    }
}

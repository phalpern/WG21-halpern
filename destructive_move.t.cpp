// destructive_move.t.cpp                  -*-C++-*-

#include "destructive_move.h"
#include <iostream>
#include <utility>

namespace exp = std::experimental;

static int status = 0;

#define TEST_ASSERT(cond)                                               \
    do { if (!(cond)) {                                                 \
             std::cout << __FILE__ ":" << __LINE__                      \
                       << ": Error: Assert failed: " #cond << std::endl; \
             status = 1;                                                \
    }} while (false)

struct Pod
{
    int a;
    double b;
};

enum throwyness { trivial, unthrowy, throwy };

template <throwyness E>
class MyClass
{
    int m_val;

    static int s_population;

public:
    MyClass(int v = 0) noexcept(E < throwy)
    {
        if (E >= throwy && 0xdead == v) throw "dead";
        m_val = v;
        ++s_population;
    }

    MyClass(const MyClass& other) noexcept(E < throwy)
    {
        if (E >= throwy && 0xbeaf == other.m_val) throw "beaf";
        m_val = other.m_val;
        ++s_population;
    }

    MyClass(MyClass&& other) noexcept(E < throwy)
    {
        if (E >= throwy && 0xcafe == other.m_val) throw "cafe";
        m_val = other.m_val;
        other.m_val = 0;
        ++s_population;
    }

    ~MyClass() { TEST_ASSERT(0xffff != m_val); m_val = 0xffff; --s_population; }

    void setVal(int v) { m_val = v; }
    int val() const { return m_val; }

    static int population() { return s_population; }
};

template <throwyness E>
int MyClass<E>::s_population = 0;

namespace std {
namespace experimental {
    template <> struct is_trivially_destructive_movable<MyClass<trivial>>
    : true_type { };
}
}

template <throwyness E>
void testMoveMyClass()
{
    using exp::destructive_move;

    typedef MyClass<E> Obj;

    Obj *a = (Obj*) ::operator new(sizeof(Obj));  // Uninitialized
    TEST_ASSERT(0 == Obj::population());
    a->setVal(0xffff);
    Obj *b = new Obj(99);                  // Initialized
    TEST_ASSERT(1 == Obj::population());

    destructive_move(a, b);
    TEST_ASSERT(1 == Obj::population());
    TEST_ASSERT(99 == a->val());
    if (E == trivial)
        TEST_ASSERT(99 == b->val());
    else
        TEST_ASSERT(0xffff == b->val());

    delete a;
    ::operator delete(b);
    TEST_ASSERT(0 == Obj::population());

    if (E < throwy)
        return;

    a = (Obj*) ::operator new(sizeof(Obj));  // Uninitialized
    TEST_ASSERT(0 == Obj::population());
    a->setVal(0xffff);
    b = new Obj(0xcafe);              // Initialized
    TEST_ASSERT(1 == Obj::population());

    try {
        destructive_move(a, b);
        TEST_ASSERT(0);  // Shouldn't get here
    }
    catch (const char* e) {
        TEST_ASSERT(0 == std::strcmp("cafe", e));
    }
    TEST_ASSERT(1 == Obj::population());
    TEST_ASSERT(0xffff == a->val()); // Unchanged
    TEST_ASSERT(0xcafe == b->val()); // Unchanged

    ::operator delete(a);
    delete b;
    TEST_ASSERT(0 == Obj::population());
}

int main()
{
    TEST_ASSERT( exp::is_trivially_destructive_movable<int>::value);
    TEST_ASSERT( exp::is_nothrow_destructive_movable<int>::value);
    TEST_ASSERT( exp::is_trivially_destructive_movable<Pod>::value);
    TEST_ASSERT( exp::is_nothrow_destructive_movable<Pod>::value);
    TEST_ASSERT( exp::is_trivially_destructive_movable<MyClass<trivial>>());
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<trivial>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<unthrowy>>());
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<unthrowy>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<throwy>>());
    TEST_ASSERT(!exp::is_nothrow_destructive_movable<MyClass<throwy>>());

    int a = 4, b = 5;
    exp::destructive_move(&a, &b);
    TEST_ASSERT(5 == a && 5 == b);

    testMoveMyClass<trivial>();
    testMoveMyClass<unthrowy>();
    testMoveMyClass<throwy>();
}

// destructive_move.t.cpp                  -*-C++-*-

#include "destructive_move.h"
#include "simple_vec.h"
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

enum throwyness {
    trivial,     // Trivially destructive movable
    unthrowy,    // Non-throwing move constructor
    throwy,      // Throwing move constructor, but specialized destructive_move
    veryThrowy   // Throwing move constructor & destructive_move
};

template <throwyness E>
class MyClass
{
    int m_val;

    static int s_population;
    static int s_move_ctor_calls;

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
        ++s_move_ctor_calls;
        if (E >= throwy && 0xcafe == other.m_val) throw "cafe";
        m_val = other.m_val;
        other.m_val = 0;
        ++s_population;
    }

    ~MyClass() { TEST_ASSERT(0xffff != m_val); m_val = 0xffff; --s_population; }

    void setVal(int v) { m_val = v; }
    int val() const { return m_val; }

    static int population() { return s_population; }
    static int move_ctor_calls() { return s_move_ctor_calls; }
};

template <throwyness E>
int MyClass<E>::s_population = 0;

template <throwyness E>
int MyClass<E>::s_move_ctor_calls = 0;

// Specialization that doesn't call (throwing) move constructor
void destructive_move(MyClass<throwy> *to, MyClass<throwy> *from) noexcept
{
    to->setVal(from->val());
    from->setVal(0xffff);
}

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

    // If 'E' is 'unthrowy' or 'veryThrowy', then the move constructor is
    // involved in the destructive move operation; otherwise (if 'E' is
    // 'trivial' or 'throwy') the move constructor is not involved.
    int newMoveCtorCalls = (E == unthrowy || E == veryThrowy) ? 1 : 0;

    typedef MyClass<E> Obj;

    Obj *a = (Obj*) ::operator new(sizeof(Obj));  // Uninitialized
    TEST_ASSERT(0 == Obj::population());
    a->setVal(0xffff);
    Obj *b = new Obj(99);                  // Initialized
    TEST_ASSERT(1 == Obj::population());
    int moveCtorCallsBefore = Obj::move_ctor_calls();

    destructive_move(a, b);

    TEST_ASSERT(Obj::move_ctor_calls() == moveCtorCallsBefore+newMoveCtorCalls);
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

    moveCtorCallsBefore = Obj::move_ctor_calls();
    try {
        destructive_move(a, b);
        TEST_ASSERT(E == throwy); // Specialized noexcept destructive_move
        TEST_ASSERT(1 == Obj::population());
        TEST_ASSERT(0xcafe == a->val());
        TEST_ASSERT(0xffff == b->val());
        delete a;
        ::operator delete(b);
    }
    catch (const char* e) {
        TEST_ASSERT(E == veryThrowy); // throw on move
        TEST_ASSERT(0 == std::strcmp("cafe", e));
        TEST_ASSERT(1 == Obj::population());
        TEST_ASSERT(0xffff == a->val()); // Unchanged
        TEST_ASSERT(0xcafe == b->val()); // Unchanged
        ::operator delete(a);
        delete b;
    }

    TEST_ASSERT(Obj::move_ctor_calls() == moveCtorCallsBefore+newMoveCtorCalls);
    TEST_ASSERT(0 == Obj::population());
}

template <throwyness E>
void testSimpleVec()
{
    // If 'E' is 'unthrowy' the (noexcept) move constructor is involved in the
    // 'destructive_move_array' call; otherwise the move constructor is not
    // involved.
    int newMoveCtorCalls = E == unthrowy ? 1 : 0;

    typedef MyClass<E>           Elem;
    typedef my::simple_vec<Elem> Obj;

    int data[] = {
        1,
        2,
        0xcafe, // Throws on move if 'E >= throwy'
        4,
        5
    };

    {
        Obj vec;
        // Insert 4 elements
        const int *data_p;
        for (data_p = data; data_p != data + 4; ++data_p)
            vec.push_back(Elem(*data_p));

        TEST_ASSERT(vec.size() == 4);
        TEST_ASSERT(vec.capacity() == 4);
        TEST_ASSERT(Elem::population() == 4);

        // The fifth insertion will cause capacity to expand to 8, forcing a
        // move or copy of all elements, including those that might throw on
        // move.  It will always succeed because no throwing operations are
        // invoked (a throwing move is replaced by a copy).
        int moveCtorCalls = Elem::move_ctor_calls();
        vec.push_back(Elem(*data_p));
        TEST_ASSERT(vec.size() == 5);
        TEST_ASSERT(vec.capacity() == 8);
        TEST_ASSERT(Elem::move_ctor_calls() ==
                    moveCtorCalls + 4 * newMoveCtorCalls);
        TEST_ASSERT(Elem::population() == 5);

        // Verify results
        data_p = data;
        for (typename Obj::iterator i = vec.begin(); i != vec.end(); ++i)
            TEST_ASSERT(*data_p++ == i->val());
    }
    TEST_ASSERT(Elem::population() == 0);

    Obj vec;
    // Insert 4 elements
    const int *data_p;
    for (data_p = data; data_p != data + 4; ++data_p)
        vec.push_back(Elem(*data_p));
    vec.back().setVal(0xbeaf);  // Throw on copy construction
    data[3] = 0xbeaf;

    TEST_ASSERT(vec.size() == 4);
    TEST_ASSERT(vec.capacity() == 4);
    TEST_ASSERT(Elem::population() == 4);

    // The fifth insertion will cause capacity to expand to 8, forcing a move
    // or copy of all elements, including those that might throw on move or
    // copy. Only the case of 'veryThrowy' will fail because it is the only
    // case where a copy constructor is invoked.
    std::size_t moveCtorCalls = Elem::move_ctor_calls();
    try {
        vec.push_back(Elem(*data_p));
        TEST_ASSERT(E != veryThrowy);
        TEST_ASSERT(vec.size() == 5);
        TEST_ASSERT(vec.capacity() == 8);
        TEST_ASSERT(Elem::move_ctor_calls() ==
                    moveCtorCalls + 4 * newMoveCtorCalls);
        TEST_ASSERT(Elem::population() == 5);
    }
    catch (const char* e) {
        TEST_ASSERT(E == veryThrowy);
        TEST_ASSERT(0 == std::strcmp("beaf", e));
        TEST_ASSERT(Elem::move_ctor_calls() == moveCtorCalls);
        // Strong guarantee holds
        TEST_ASSERT(vec.size() == 4);
        TEST_ASSERT(vec.capacity() == 4);
        TEST_ASSERT(Elem::population() == 4);
    }

    // Verify results
    data_p = data;
    typename Obj::iterator i = vec.begin();
    for (std::size_t c = 0; c < vec.size(); ++c)
        TEST_ASSERT(*data_p++ == (i++)->val());
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
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<throwy>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<veryThrowy>>());
    TEST_ASSERT(!exp::is_nothrow_destructive_movable<MyClass<veryThrowy>>());

    int a = 4, b = 5;
    exp::destructive_move(&a, &b);
    TEST_ASSERT(5 == a && 5 == b);

    testMoveMyClass<trivial>();
    testMoveMyClass<unthrowy>();
    testMoveMyClass<throwy>();
    testMoveMyClass<veryThrowy>();

    testSimpleVec<trivial>();
    testSimpleVec<unthrowy>();
    testSimpleVec<throwy>();
    testSimpleVec<veryThrowy>();
}

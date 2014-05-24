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

enum moveType {
    trivialMove,   // Trivially destructive movable
    nothrowMove,   // Non-throwing move constructor
    specialMove,   // Throwing move ctor, but specialized destructive_move
    throwingMove   // Throwing move constructor & destructive_move
};

template <moveType E>
class MyClass
{
    int m_val;

    static int s_population;
    static int s_copy_ctor_calls;
    static int s_move_ctor_calls;

public:
    MyClass(int v = 0) noexcept(E < specialMove)
    {
        if (E >= specialMove && 0xdead == v) throw "dead";
        m_val = v;
        ++s_population;
    }

    MyClass(const MyClass& other) noexcept(E < specialMove)
    {
        ++s_copy_ctor_calls;
        if (E >= specialMove && 0xbeaf == other.m_val) throw "beaf";
        m_val = other.m_val;
        ++s_population;
    }

    MyClass(MyClass&& other) noexcept(E < specialMove)
    {
        ++s_move_ctor_calls;
        if (E >= specialMove && 0xcafe == other.m_val) throw "cafe";
        m_val = other.m_val;
        other.m_val = 0;
        ++s_population;
    }

    ~MyClass() { TEST_ASSERT(0xffff != m_val); m_val = 0xffff; --s_population; }

    void setVal(int v) { m_val = v; }
    int val() const { return m_val; }

    static int population() { return s_population; }
    static int copy_ctor_calls() { return s_copy_ctor_calls; }
    static int move_ctor_calls() { return s_move_ctor_calls; }
};

template <moveType E>
int MyClass<E>::s_population = 0;

template <moveType E>
int MyClass<E>::s_copy_ctor_calls = 0;

template <moveType E>
int MyClass<E>::s_move_ctor_calls = 0;

// Specialization that doesn't call (throwing) move constructor
void destructive_move(MyClass<specialMove> *to, MyClass<specialMove> *from) noexcept
{
    to->setVal(from->val());
    from->setVal(0xffff);
}

namespace std {
namespace experimental {
    template <> struct is_trivially_destructive_movable<MyClass<trivialMove>>
    : true_type { };
}
}

template <moveType E>
void testMoveMyClass()
{
    // Test operation of 'destructive_move'.  Whether or not
    // 'destructive_move' can throw and whether or not it calls the move
    // constructor for 'MyClass' depends on 'E' as follows:
    //
    // E            Can throw? Mechanism
    // ----------   ---------- ---------
    // trivialMove  No         memcpy instead of move ctor.
    // nothrowMove  No         (nothrow) move ctor
    // specialMove  No         specialized nothrow 'destructive_move'
    // throwingMove Yes        throwing move ctor

    using exp::destructive_move;

    // the move constructor is called only for 'nothrowMove' or 'throwingMove':
    int callMoveCtor = (E == nothrowMove || E == throwingMove) ? 1 : 0;

    typedef MyClass<E> Obj;

    Obj *a = (Obj*) ::operator new(sizeof(Obj));  // Uninitialized
    TEST_ASSERT(0 == Obj::population());
    a->setVal(0xffff);
    Obj *b = new Obj(99);                  // Initialized
    TEST_ASSERT(1 == Obj::population());
    int moveCtorCallsBefore = Obj::move_ctor_calls();

    destructive_move(a, b);

    TEST_ASSERT(Obj::move_ctor_calls() == moveCtorCallsBefore+callMoveCtor);
    TEST_ASSERT(1 == Obj::population());
    TEST_ASSERT(99 == a->val());
    if (E == trivialMove)
        TEST_ASSERT(99 == b->val());
    else
        TEST_ASSERT(0xffff == b->val());

    delete a;
    ::operator delete(b);
    TEST_ASSERT(0 == Obj::population());

    if (E < specialMove)
        return;

    a = (Obj*) ::operator new(sizeof(Obj));  // Uninitialized
    TEST_ASSERT(0 == Obj::population());
    a->setVal(0xffff);
    b = new Obj(0xcafe);              // Initialized
    TEST_ASSERT(1 == Obj::population());

    moveCtorCallsBefore = Obj::move_ctor_calls();
    try {
        destructive_move(a, b);
        TEST_ASSERT(E == specialMove); // Specialized noexcept destructive_move
        TEST_ASSERT(1 == Obj::population());
        TEST_ASSERT(0xcafe == a->val());
        TEST_ASSERT(0xffff == b->val());
        delete a;
        ::operator delete(b);
    }
    catch (const char* e) {
        TEST_ASSERT(E == throwingMove); // throw on move
        TEST_ASSERT(0 == std::strcmp("cafe", e));
        TEST_ASSERT(1 == Obj::population());
        TEST_ASSERT(0xffff == a->val()); // Unchanged
        TEST_ASSERT(0xcafe == b->val()); // Unchanged
        ::operator delete(a);
        delete b;
    }

    TEST_ASSERT(Obj::move_ctor_calls() == moveCtorCallsBefore+callMoveCtor);
    TEST_ASSERT(0 == Obj::population());
}

template <moveType E>
void testSimpleVec()
{
    // Test operation of 'simple_vec', which uses 'destructive_move_array'.

#ifdef USE_DESTRUCTIVE_MOVE
    // Whether or not 'destructive_move_array' can throw and whether or not it
    // calls the move constructor for 'MyClass' depends on 'E' as follows:
    //
    // E            Can throw? Mechanism
    // ----------   ---------- ---------
    // trivialMove  No         memcpy instead of move ctor.
    // nothrowMove  No         nothrow move ctor
    // specialMove  No         specialized nothrow 'destructive_move'
    // throwingMove Yes        throwing copy ctor (not move ctor)
    const int callMoveCtor = E == nothrowMove  ? 1 : 0;
    const int callCopyCtor = E == throwingMove ? 1 : 0;
#else
    // If 'USE_DESTRUCTIVE_MOVE' is not defined, then the move constructor and
    // copy constructor are used more frequently and exceptions are thrown in
    // more circumstances:
    //
    // E            Can throw? Mechanism
    // ----------   ---------- ---------
    // trivialMove  No         nothrow move ctor
    // nothrowMove  No         nothrow move ctor
    // specialMove  Yes        throwing copy ctor (not move ctor)
    // throwingMove Yes        throwing copy ctor (not move ctor)
    const int callMoveCtor = E <= nothrowMove ? 1 : 0;
    const int callCopyCtor = E >= specialMove ? 1 : 0;
#endif

    typedef MyClass<E>           Elem;
    typedef my::simple_vec<Elem> Obj;

    int data[] = {
        1,
        2,
        0xcafe, // Throws on move if 'E >= specialMove'
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
        int copyCtorCalls = Elem::copy_ctor_calls();
        vec.push_back(Elem(*data_p));
        TEST_ASSERT(vec.size() == 5);
        TEST_ASSERT(vec.capacity() == 8);
        TEST_ASSERT(Elem::move_ctor_calls() ==
                    moveCtorCalls + 4 * callMoveCtor);
        TEST_ASSERT(Elem::copy_ctor_calls() ==
                    copyCtorCalls + 4 * callCopyCtor + 1);
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
    // copy. Only the case of 'throwingMove' will fail because it is the only
    // case where a copy constructor is invoked.
    std::size_t moveCtorCalls = Elem::move_ctor_calls();
    std::size_t copyCtorCalls = Elem::copy_ctor_calls();
    try {
        vec.push_back(Elem(*data_p));
        TEST_ASSERT(E != throwingMove);
        TEST_ASSERT(vec.size() == 5);
        TEST_ASSERT(vec.capacity() == 8);
        TEST_ASSERT(Elem::move_ctor_calls() ==
                    moveCtorCalls + 4 * callMoveCtor);
        TEST_ASSERT(Elem::copy_ctor_calls() == copyCtorCalls + 1);
        TEST_ASSERT(Elem::population() == 5);
    }
    catch (const char* e) {
#if USE_DESTRUCTIVE_MOVE
        TEST_ASSERT(E == throwingMove);
#else
        TEST_ASSERT(E == specialMove || E == throwingMove);
#endif
        TEST_ASSERT(0 == std::strcmp("beaf", e));
        TEST_ASSERT(Elem::move_ctor_calls() == moveCtorCalls);
        TEST_ASSERT(Elem::copy_ctor_calls() ==
                    copyCtorCalls + 4 * callCopyCtor);
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
    TEST_ASSERT( exp::is_trivially_destructive_movable<MyClass<trivialMove>>());
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<trivialMove>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<nothrowMove>>());
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<nothrowMove>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<specialMove>>());
    TEST_ASSERT( exp::is_nothrow_destructive_movable<MyClass<specialMove>>());
    TEST_ASSERT(!exp::is_trivially_destructive_movable<MyClass<throwingMove>>());
    TEST_ASSERT(!exp::is_nothrow_destructive_movable<MyClass<throwingMove>>());

    int a = 4, b = 5;
    exp::destructive_move(&a, &b);
    TEST_ASSERT(5 == a && 5 == b);

    testMoveMyClass<trivialMove>();
    testMoveMyClass<nothrowMove>();
    testMoveMyClass<specialMove>();
    testMoveMyClass<throwingMove>();

    testSimpleVec<trivialMove>();
    testSimpleVec<nothrowMove>();
    testSimpleVec<specialMove>();
    testSimpleVec<throwingMove>();

    std::cout << (status ? "TEST FAILED" : "TEST PASSED") << std::endl;

    return status;
}

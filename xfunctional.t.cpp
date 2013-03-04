// xfunctional.t.cpp                  -*-C++-*-

#include "xfunctional.h"

#include <iostream>
#include <cstdlib>

using namespace std;


//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//
// Test allocator propgation in XSTD::function with each of the following
// orthogonal concerns:
//
// 1. No allocator, SimpleAllocator, polymorphic_allocator, allocator_resource
// 2. No function, function ptr, stateless functor, stateful functor
// 3. Each of the (1) cases combined with copy construction, move
//    construction, copy assignment, and move-assignment.
//
// Also test copy construction, move construction, copy assignment and move
// assignment between XSTD::function and std::function to ensure binary
// compatibility.

//-----------------------------------------------------------------------------

//==========================================================================
//                  ASSERT TEST MACRO
//--------------------------------------------------------------------------

namespace {

int testStatus = 0;
bool verbose = false;
bool veryVerbose = false;

void aSsErT(int c, const char *s, int i) {
    if (c) {
        std::cout << __FILE__ << ":" << i << ": error: " << s
                  << "    (failed)" << std::endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

} // Close unnamed namespace

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }
//--------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
    if (!(X)) { std::cout << #I << ": " << I << "\n"; \
                aSsErT(1, #X, __LINE__); } }

#define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " \
                          << J << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J \
                         << "\t" << #K << ": " << K << "\n";           \
                aSsErT(1, #X, __LINE__); } }

#define LOOP4_ASSERT(I,J,K,L,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J \
                         << "\t" << #K << ": " << K << "\t" << #L << ": " \
                         << L << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP5_ASSERT(I,J,K,L,M,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J    \
                         << "\t" << #K << ": " << K << "\t" << #L << ": " \
                         << L << "\t" << #M << ": " << M << "\n";         \
               aSsErT(1, #X, __LINE__); } }

#define LOOP6_ASSERT(I,J,K,L,M,N,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J     \
                         << "\t" << #K << ": " << K << "\t" << #L << ": "  \
                         << L << "\t" << #M << ": " << M << "\t" << #N     \
                         << ": " << N << "\n"; aSsErT(1, #X, __LINE__); } }

// Allow compilation of individual test-cases (for test drivers that take a
// very long time to compile).  Specify '-DSINGLE_TEST=<testcase>' to compile
// only the '<testcase>' test case.
#define TEST_IS_ENABLED(num) (! defined(SINGLE_TEST) || SINGLE_TEST == (num))


//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) std::cout << #X " = " << (X) << std::endl; // Print identifier and value.
#define Q(X) std::cout << "<| " #X " |>" << std::endl;  // Quote identifier literally.
#define P_(X) std::cout << #X " = " << (X) << ", " << flush; // P(X) without '\n'
#define L_ __LINE__                           // current Line number
#define T_ std::cout << "\t" << flush;             // Print a tab (w/o newline)

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

enum { VERBOSE_ARG_NUM = 2, VERY_VERBOSE_ARG_NUM, VERY_VERY_VERBOSE_ARG_NUM };

#define UNPAREN(...) __VA_ARGS__

//=============================================================================
//                  CLASSES FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

class AllocCounters
{
    int m_num_allocs;
    int m_num_deallocs;
    int m_bytes_allocated;
    int m_bytes_deallocated;

  public:
    AllocCounters()
	: m_num_allocs(0)
	, m_num_deallocs(0)
	, m_bytes_allocated(0)
	, m_bytes_deallocated(0)
	{ }

    void allocate(std::size_t nbytes) {
        ++m_num_allocs;
        m_bytes_allocated += nbytes;
    }

    void deallocate(std::size_t nbytes) {
        ++m_num_deallocs;
        m_bytes_deallocated += nbytes;
    }

    int blocks_outstanding() const { return m_num_allocs - m_num_deallocs; }
    int bytes_outstanding() const
        { return m_bytes_allocated - m_bytes_deallocated; }

    void dump(std::ostream& os, const char* msg) {
	os << msg << ":\n";
        os << "  num allocs         = " << m_num_allocs << '\n';
        os << "  num deallocs       = " << m_num_deallocs << '\n';
        os << "  outstanding allocs = " << blocks_outstanding() << '\n';
        os << "  bytes allocated    = " << m_bytes_allocated << '\n';
        os << "  bytes deallocated  = " << m_bytes_deallocated << '\n';
        os << "  outstanding bytes  = " << bytes_outstanding() << '\n';
        os << std::endl;
    }

    void clear() {
	m_num_allocs = 0;
	m_num_deallocs = 0;
	m_bytes_allocated = 0;
	m_bytes_deallocated = 0;
    }
};

union CountedAllocHeader {
    void*       m_align;
    std::size_t m_size;
};

void *countedAllocate(std::size_t nbytes, AllocCounters *counters)
{
    static const std::size_t headerSize = sizeof(CountedAllocHeader);
    std::size_t blocksize = (nbytes + 2 * headerSize - 1) & ~(headerSize - 1);
    CountedAllocHeader* ret =
        static_cast<CountedAllocHeader*>(std::malloc(blocksize));
    ret->m_size = nbytes;
    ++ret;
    if (counters)
        counters->allocate(nbytes);
    return ret;
}

void countedDeallocate(void *p, std::size_t nbytes, AllocCounters *counters)
{
    CountedAllocHeader* h = static_cast<CountedAllocHeader*>(p) - 1;
    ASSERT(nbytes == h->m_size);
    h->m_size = 0xdeadbeaf;
    std::free(h);
    if (counters)
        counters->deallocate(nbytes);
}

void countedDeallocate(void *p, AllocCounters *counters)
{
    CountedAllocHeader* h = static_cast<CountedAllocHeader*>(p) - 1;
    std::size_t nbytes = h->m_size;
    h->m_size = 0xdeadbeaf;
    std::free(h);
    if (counters)
        counters->deallocate(nbytes);
}

// Abbreviations for names in 'XSTD::polyalloc' namespace.
using XSTD::polyalloc::allocator_resource;
using XSTD::polyalloc::new_delete_resource;
using XSTD::polyalloc::new_delete_resource_singleton;

allocator_resource *newDelRsrc = new_delete_resource_singleton();
AllocCounters newDelCounters;

// Replace global new and delete
void* operator new(std::size_t nbytes)
{
    return countedAllocate(nbytes, &newDelCounters);
}

void operator delete(void *p)
{
    countedDeallocate(p, &newDelCounters);
}

class TestResource : public allocator_resource
{
    AllocCounters m_counters;

    // Not copyable
    TestResource(const TestResource&);
    TestResource operator=(const TestResource&);

public:
    TestResource() { }

    virtual ~TestResource();
    virtual void* allocate(size_t bytes, size_t alignment = 0);
    virtual void  deallocate(void *p, size_t bytes, size_t alignment = 0);
    virtual bool is_equal(const allocator_resource& other) const;

    AllocCounters      & counters()       { return m_counters; }
    AllocCounters const& counters() const { return m_counters; }

    void clear() { m_counters.clear(); }
};

TestResource::~TestResource()
{
    ASSERT(0 == m_counters.blocks_outstanding());
}

void* TestResource::allocate(size_t bytes, size_t alignment)
{
    return countedAllocate(bytes, &m_counters);
}

void  TestResource::deallocate(void *p, size_t bytes, size_t alignment)
{
    countedDeallocate(p, bytes, &m_counters);
}

bool TestResource::is_equal(const allocator_resource& other) const
{
    // Two TestResource objects are equal only if they are the same object
    return this == &other;
}

TestResource dfltTstRsrc;
AllocCounters& dfltTstCounters = dfltTstRsrc.counters();

AllocCounters dfltSimpleCounters;

template <typename Tp>
class SimpleAllocator
{
    AllocCounters *m_counters;

    template <typename T2> friend class SimpleAllocator;

  public:
    typedef Tp              value_type;

    // C++11 allocators usually don't need a rebind, but the gcc 4.6.3
    // implementation of shared_ptr still depends on the C++03 allocator
    // definition.
    template <class T2>
    struct rebind {
        typedef SimpleAllocator<T2> other;
    };

    SimpleAllocator(AllocCounters* c = &dfltSimpleCounters) : m_counters(c) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : m_counters(other.m_counters) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(countedAllocate(n*sizeof(Tp), m_counters)); }

    void deallocate(Tp* p, std::size_t n)
        { countedDeallocate(p, n*sizeof(Tp), m_counters); }

    const AllocCounters& counters() const { return *m_counters; }
};

template <typename Tp1, typename Tp2>
bool operator==(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return &a.counters() == &b.counters();
}

template <typename Tp1, typename Tp2>
bool operator!=(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return ! (a == b);
}

class Functor
{
public:
    typedef XSTD::polyalloc::polymorphic_allocator<int> allocator_type;

private:
    allocator_type  m_allocator;
    int            *m_offset_p;

    typedef XSTD::allocator_traits<allocator_type> ATraits;

public:
    Functor(int offset, const allocator_type& alloc) : m_allocator(alloc) {
        m_offset_p = ATraits::allocate(m_allocator, 1);
        *m_offset_p = offset;
    }

    Functor(const Functor& other)
        : m_allocator(ATraits::select_on_container_copy_construction(
                          other.m_allocator)) {
        m_offset_p = ATraits::allocate(m_allocator, 1);
        *m_offset_p = *other.m_offset_p;
    }

    Functor(const Functor& other, const allocator_type& alloc)
        : m_allocator(alloc) {
        m_offset_p = ATraits::allocate(m_allocator, 1);
        *m_offset_p = *other.m_offset_p;
    }

    ~Functor() { ATraits::deallocate(m_allocator, m_offset_p, 1); }

    int operator()(const char* s) {
        return *m_offset_p + std::atoi(s);
    }
};

// Return the address of the 'AllocCounters' object managed by the specified
// 'alloc'.  All (possibly rebound) copies of 'alloc' will use the same
// counters.
template <class T>
const AllocCounters *getCounters(const SimpleAllocator<T>& alloc)
{
    return &alloc.counters();
}

// If 'alloc' is a 'std::allocator' instance, then return the address of
// 'newDelCounters'.
template <class T>
const AllocCounters *getCounters(const std::allocator<T>& alloc)
{
    return &newDelCounters;
}

// If the dynamic type of 'alloc' is 'TestResource', return the address of the
// 'AllocCounters' object managed by 'alloc'. Otherwise, if the dynamic type
// of 'alloc' is a 'resource_adaptor<SimpleAllocator<T>>', return the adddress
// of the 'AllocCounters' object managed by the 'SimpleAllocator' within the
// adaptor. Otherwise, return NULL.  Note that if a 'function' is constructed
// with an 'allocator_resource*', it should store the exact pointer, not a
// pointer to a copy of the resource.
const AllocCounters *
getCounters(allocator_resource *const &alloc)
{
    typedef POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char> ) SmplAdaptor;

    const TestResource *tstRsrc = dynamic_cast<const TestResource*>(alloc);
    if (tstRsrc)
        return &tstRsrc->counters();
    const SmplAdaptor *aRsrc = dynamic_cast<const SmplAdaptor*>(alloc);
    if (aRsrc)
        return getCounters(aRsrc->get_allocator());
    const new_delete_resource *stdRsrc =
        dynamic_cast<const new_delete_resource*>(alloc);
    if (stdRsrc)
        return &newDelCounters;
    return NULL;
}

// If 'alloc' is a wrapper around a 'TestResource', then return the address of
// the 'AllocCounters' object managed by the 'TestResource', otherwise return
// NULL.  Note that if a 'function' is constructed with an
// 'polymorphic_allocator', it should store the exact pointer returned by
// 'alloc.resource(), not a pointer to a copy of that resource.
template <class T>
const AllocCounters *
getCounters(const XSTD::polyalloc::polymorphic_allocator<T>& alloc)
{
    return getCounters(alloc.resource());
}

// Compute the expected block count given the allocation requirements for the
// function object, 'rawCount' as well as flags indicating whether the
// function is invocable (i.e., not empty/null), whether allocator is the
// new/delete allocator, and whether the allocator is shared with another
// function object.
int calcBlocks(int rawCount, bool isInvocable, bool isNewDelRsrc,
               bool sharedAlloc = false)
{
    // If the allocator is not the new/delete allocator, then we cannot use the
    // small-object optimization and the block count must be at least one.
    // This minimum allocation does not apply to non-invocable (i.e.,
    // empty/null) function objects.
    if (isInvocable && ! isNewDelRsrc)
        rawCount = std::max(rawCount, 1);

    // The use of a non-new-delete allocator requires the creation of a shared
    // pointer, which increases the block count by one.
    if (! isNewDelRsrc && ! sharedAlloc)
        ++rawCount;

    return rawCount;
}

// Given a list of 0 to 3 XSTD::function constructor arguments, return the
// allocator argument, if any, or the default allocator resource if none.
allocator_resource* allocatorFromCtorArgs()
{
    return allocator_resource::default_resource();
}

template <class F>
allocator_resource* allocatorFromCtorArgs(F&&)
{
    return allocator_resource::default_resource();
}

template <class A>
const A& allocatorFromCtorArgs(allocator_arg_t, const A& a)
{
    return a;
}

template <class A, class F>
const A& allocatorFromCtorArgs(allocator_arg_t, const A& a, F&&)
{
    return a;
}

// Given a list of 0 to 3 XSTD::function constructor arguments, return the
// functor argument, if any, or nullptr if none.
nullptr_t functorFromCtorArgs()
{
    return nullptr;
}

template <class F>
F&& functorFromCtorArgs(F&& f)
{
    return std::forward<F>(f);
}

template <class A>
nullptr_t functorFromCtorArgs(allocator_arg_t, const A& a)
{
    return nullptr;
}

template <class A, class F>
F&& functorFromCtorArgs(allocator_arg_t, const A&, F&& f)
{
    return std::forward<F>(f);
}

#define POLYALLOC XSTD::polyalloc::polymorphic_allocator

inline bool defaultIsNewDel()
{
    return (allocator_resource::default_resource() ==
            new_delete_resource_singleton());
}

// Return a string indicating whether the default allocator resource is equal
// to the new/delete allocator resource.
inline
const char* defaultIsNewDelStr()
{
    if (defaultIsNewDel())
        return "[default == new/del]";
    else
        return "[default != new/del]";
}

// Simple stack that requires no dynamic memory allocation
template <typename T, std::size_t MAX_SIZE>
class SimpleStack
{
    T           m_entries[MAX_SIZE];
    std::size_t m_index;

public:
    void push(const T& v) {
        ASSERT(m_index < MAX_SIZE);
        m_entries[m_index++] = v;
    }

    T pop() {
        ASSERT(m_index > 0);
        return m_entries[--m_index];
    }
};

SimpleStack<allocator_resource*, 10> allocResourceStack;

void pushDefaultResource(allocator_resource *r)
{
    allocResourceStack.push(allocator_resource::default_resource());
    allocator_resource::set_default_resource(r);
}

void popDefaultResource()
{
    allocator_resource::set_default_resource(allocResourceStack.pop());
}

// Test constructor invocation.
//
// Concerns:
// - Constructed function uses correct allocator
// - On construction, expected number of blocks are allocated from the
//   specified allocator. 
// - On destruction, expected number of blocks are deallocated from the
//   specified allocator. 
// - No memory is allocated or deallocated from the default allocator resource
//   unless the specified allocator uses the default allocator resource.
// - No memory is allocated or deallocated using global operator new/delete
//   unless the specified allocator uses the new_delete_resource.
// - When the functor is invoked, the expected result is produced.
// - If the functor is invocable, then cast to bool returns true
//   and 'operator!' returns false.
// - If the functor is supposed to be empty, then cast to bool returns false
//   and 'operator!' returns true.
template <class F, class A, class... CtorArgs>
void testCtor(const A& alloc, int expRet,
              int expAllocBlocks, int expDeallocBlocks,
              CtorArgs&&... ctorArgs)
{
    typedef XSTD::function<F> Obj;

    const AllocCounters *const counters = getCounters(alloc);
    const int startDfltBlocks = dfltTstCounters.blocks_outstanding();
    const int startNewDelBlocks = newDelCounters.blocks_outstanding();
    int expBlocks = counters->blocks_outstanding();

    {
        Obj f(std::forward<CtorArgs>(ctorArgs)...);

        expBlocks += expAllocBlocks;
        ASSERT(getCounters(f.get_allocator_resource()) == counters);
        ASSERT(counters->blocks_outstanding() == expBlocks);
        if (counters != &dfltTstCounters)
            ASSERT(dfltTstCounters.blocks_outstanding() == startDfltBlocks);
        if (counters != &newDelCounters)
            ASSERT(newDelCounters.blocks_outstanding() == startNewDelBlocks);
        if (expRet)
        {
            ASSERT(static_cast<bool>(f));
            ASSERT(false == ! f);
            ASSERT(f);
            ASSERT(expRet == f("74"));
        }
        else
        {
            ASSERT(! static_cast<bool>(f));
            ASSERT(! f);
        }
    }
    expBlocks -= expDeallocBlocks;
    ASSERT(counters->blocks_outstanding() == expBlocks);
    if (counters != &dfltTstCounters)
        ASSERT(dfltTstCounters.blocks_outstanding() == startDfltBlocks);
    if (counters != &newDelCounters)
        ASSERT(newDelCounters.blocks_outstanding() == startNewDelBlocks);
}    

template <class F, class A, class Rhs, class... LhsCtorArgs>
void testAssignment(const A& alloc, Rhs&& rhs, int expRet,
                    int expAllocBlocks, int expDeallocBlocks,
                    LhsCtorArgs&&... lhsCtorArgs)
{
    typedef XSTD::function<F> Obj;

    const AllocCounters *const counters = getCounters(alloc);
    int expBlocks = counters->blocks_outstanding();
    const int startDfltBlocks = dfltTstCounters.blocks_outstanding();
    const int startNewDelBlocks = newDelCounters.blocks_outstanding();

    {
        // Construct lhs, then assign to it.  Memory used by the constructor
        // should be returned to the heap, so only the memory from the copy
        // should remain outstanding.
        Obj lhs(std::forward<LhsCtorArgs>(lhsCtorArgs)...);
        lhs = std::forward<Rhs>(rhs);

        expBlocks += expAllocBlocks;
        ASSERT(getCounters(lhs.get_allocator_resource()) == counters);
        ASSERT(counters->blocks_outstanding() == expBlocks);
        if (counters != &dfltTstCounters)
            ASSERT(dfltTstCounters.blocks_outstanding() == startDfltBlocks);
        if (counters != &newDelCounters)
            ASSERT(newDelCounters.blocks_outstanding() == startNewDelBlocks);
        if (expRet)
        {
            ASSERT(static_cast<bool>(lhs));
            ASSERT(false == ! lhs);
            ASSERT(lhs);
            ASSERT(expRet == lhs("74"));
        }
        else
        {
            ASSERT(! static_cast<bool>(lhs));
            ASSERT(! lhs);
        }
    }
    expBlocks -= expDeallocBlocks;
    ASSERT(counters->blocks_outstanding() == expBlocks);
    if (counters != &dfltTstCounters)
        ASSERT(dfltTstCounters.blocks_outstanding() == startDfltBlocks);
    if (counters != &newDelCounters)
        ASSERT(newDelCounters.blocks_outstanding() == startNewDelBlocks);
}

// Test copy assignment of XSTD::function
template <class F, class A>
void testCopyAssign(const A& alloc, const XSTD::function<F>& rhs,
                    int expRet, int expRawBlocks)
{
    typedef XSTD::function<F> Obj;

    // Stateless lambda
    auto lambda = [](const char* s) { return std::atoi(s); };

    // Stateful functor that uses an allocator
    TestResource tmpRsrc;  // Ignore this allocator resource
    Functor functor(1, &tmpRsrc);

    // Null function pointer
    int (*const nullFuncPtr)(const char*) = nullptr;

    const AllocCounters *const counters = getCounters(alloc);
    bool isNewDelRsrc = (counters == &newDelCounters);
    bool sharedAlloc = (counters == getCounters(rhs.get_allocator_resource()));

    int expCopyBlocks = calcBlocks(expRawBlocks, expRet != 0,
                                   isNewDelRsrc, sharedAlloc);

    // Test assignment with initial lhs having each possible functor type.
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc);
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc, nullptr);
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc, nullFuncPtr);
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc, &std::atoi);
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc, lambda);
    testAssignment<F>(alloc, rhs, expRet, expCopyBlocks, expCopyBlocks,
                      allocator_arg, alloc, functor);
}

// Test move assignment of XSTD::function
template <class F, class A>
void testMoveAssign(const A& alloc, const XSTD::function<F>& rhs,
                    int expRet, int expRawBlocks)
{
    typedef XSTD::function<F> Obj;

    // Stateless lambda
    auto lambda = [](const char* s) { return std::atoi(s); };

    // Stateful functor that uses an allocator
    TestResource tmpRsrc;  // Ignore this allocator resource
    Functor functor(1, &tmpRsrc);

    // Null function pointer
    int (*const nullFuncPtr)(const char*) = nullptr;

    const AllocCounters *const counters = getCounters(alloc);
    bool isNewDelRsrc = (counters == &newDelCounters);

    int expAllocBlocks;
    int expDeallocBlocks = calcBlocks(expRawBlocks, expRet != 0, isNewDelRsrc);

    if (counters == getCounters(rhs.get_allocator_resource())) {
        expAllocBlocks = 0;
        expDeallocBlocks -= (isNewDelRsrc ? 0 : 1);
    }
    else
        expAllocBlocks = expDeallocBlocks;

// rvalue copy of 'rhs':
#define RHS_RV Obj(allocator_arg, rhs.get_allocator_resource(), rhs)

    // Test assignment with initial lhs having each possible functor type.
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc);
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc, nullptr);
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc, nullFuncPtr);
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc, &std::atoi);
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc, lambda);
    testAssignment<F>(alloc, RHS_RV, expRet, expAllocBlocks, expDeallocBlocks,
                      allocator_arg, alloc, functor);

#undef RHS_RV
}

// Test heterogeneous assignment into XSTD::function
template <class F, class A, class Rhs>
void testHeteroAssign(const A& alloc, Rhs&& rhs, int expRet, int expRawBlocks)
{
    // Stateless lambda
    auto lambda = [](const char* s) { return std::atoi(s); };

    // Stateful functor that uses an allocator
    TestResource tmpRsrc;  // Ignore this allocator resource
    Functor functor(1, &tmpRsrc);

    // Null function pointer
    int (*const nullFuncPtr)(const char*) = nullptr;

    const AllocCounters *const counters = getCounters(alloc);
    bool isNewDelRsrc = (counters == &newDelCounters);

    int expBlocks = calcBlocks(expRawBlocks, expRet != 0, isNewDelRsrc);

    // Test assignment with initial lhs having each possible functor type.
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc);
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, nullptr);
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, nullFuncPtr);
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, &std::atoi);
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, lambda);
    testAssignment<F>(alloc, rhs, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, functor);
}

// Test ABI compatibility with pre-allocator implementation of gnu
// std::function.
template <class F, class A, class... CtorArgs>
void testABICompatibility(const A& alloc, int expRet, CtorArgs&&... ctorArgs)
{
    // Known fails:
    // - Copying from legacy to new function will ignore new allocator.
    // - Constructing a new *empty* function with type-erased allocator and
    //   destructing it in legacy code will result in a small memory leak.
    //   Similarly, moving an empty function with allocator to within legacy
    //   code and destroying it in legacy code will result in a small memory
    //   leak.

    typedef XSTD::function<F> Obj;
    typedef std::function<F>  LegacyObj;

    static_assert(sizeof(Obj) == sizeof(LegacyObj),
                  "ABI incompatibility: size of std::function has changed");

    const AllocCounters *const counters = getCounters(alloc);
    const int startBlocks = counters->blocks_outstanding();
    const int startNewDelBlocks = newDelCounters.blocks_outstanding();

    bool isNewDelAlloc = (counters == &newDelCounters);

    {
        // Construct new function object and test invocability through legacy
        // view.
        Obj fnew(std::forward<CtorArgs>(ctorArgs)...);
        LegacyObj& fleg = reinterpret_cast<LegacyObj&>(fnew);

        // Test invocability
        if (expRet)
        {
            ASSERT(static_cast<bool>(fleg));
            ASSERT(false == ! fleg);
            ASSERT(fleg);
            ASSERT(expRet == fleg("74"));
        }
        else
        {
            ASSERT(! static_cast<bool>(fleg));
            ASSERT(! fleg);
        }

        // Test copy construction
        int preCopyBlocks = counters->blocks_outstanding();
        LegacyObj fleg2(fleg);
        if (! isNewDelAlloc)
            ASSERT(counters->blocks_outstanding() == preCopyBlocks);

        // Test invocability of copy
        if (expRet)
        {
            ASSERT(static_cast<bool>(fleg2));
            ASSERT(false == ! fleg2);
            ASSERT(fleg2);
            ASSERT(expRet == fleg2("74"));
        }
        else
        {
            ASSERT(! static_cast<bool>(fleg2));
            ASSERT(! fleg2);
        }

        // Test move construction
        LegacyObj fleg3(std::move(fleg));
        if (! isNewDelAlloc)
            ASSERT(counters->blocks_outstanding() == preCopyBlocks);

        // Test invocability of move
        if (expRet)
        {
            ASSERT(static_cast<bool>(fleg3));
            ASSERT(false == ! fleg3);
            ASSERT(fleg3);
            ASSERT(expRet == fleg3("74"));
        }
        else
        {
            ASSERT(! static_cast<bool>(fleg3));
            ASSERT(! fleg3);
        }

        if (! expRet && ! isNewDelAlloc)
        {
            // Moving an empty function that uses a non-new/delete allocator
            // into a legacy function causes a known memory leak.  Fix it
            // by destroying the legacy function using the new destructor then
            // rebuilding an empty legacy function.
            reinterpret_cast<Obj&>(fleg3).~Obj();
            new (&fleg3) LegacyObj;
        }
    }

    // Assert that all memory was freed
    ASSERT(counters->blocks_outstanding() == startBlocks);
    ASSERT(newDelCounters.blocks_outstanding() == startNewDelBlocks);
}

// Test construction, copy construction, and move construction of
// XSTD::function
template <class F, class... CtorArgs>
void testFunction(int expRet, int expRawBlocks, bool isNewDelRsrc,
                  CtorArgs&&... ctorArgs)
{
    typedef XSTD::function<F> Obj;

    // NOTE: Cannot use std::forward<CtorArgs>(ctorArgs)... because the
    // arguments are being passed multiple times.  A function must forward its
    // arguments only once or else rvalues will get destroyed.
    auto alloc = allocatorFromCtorArgs(ctorArgs...);
    auto rhsFunc = functorFromCtorArgs(ctorArgs...);

    // Compute expected blocks needed to construct function.
    int expObjBlocks = calcBlocks(expRawBlocks, expRet != 0, isNewDelRsrc);

    // Test function constructor
    testCtor<F>(alloc, expRet, expObjBlocks, expObjBlocks, ctorArgs...);

    if (veryVerbose)
        std::cout << "        ABI compatibility check" << std::endl;
    testABICompatibility<F>(alloc, expRet, ctorArgs...);

    TestResource testRsrc;
    typedef POLYALLOC<char> PolyAllocTp;
    PolyAllocTp polyAlloc(&testRsrc);
    AllocCounters smplAllocCounters;
    SimpleAllocator<char>  smplAlloc(&smplAllocCounters);
    POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>)
        smplAllocAdaptor(smplAlloc);
    std::allocator<int> stdAlloc;

    typedef XSTD::function<F> Obj;

// FRSRC is the allocator resource used by function object 'f'
#define FRSRC (f.get_allocator_resource())

// This macro tests the copy constructor and extended copy constructor.
// It first computes the space requirements for the copy.  If the copy uses
// the same allocator resource as the original, then the space requirements
// are one block less because the space needs of the shared allocator pointer
// are shared between the original and the copy (hence, expCopyBlocks is
// decremented in this case).  Once the space requirements are computed, the
// constructor test is performed to test the copy.
#define COPY_CTOR_TEST(cArgs, cAlloc, cIsNewDelRsrc) do {                     \
            if (veryVerbose)                                                  \
                std::cout << "        copy function" #cArgs " "               \
                          << defaultIsNewDelStr() << std::endl;               \
            bool cSharedAlloc = (getCounters(cAlloc) == getCounters(FRSRC));  \
            int expCopyBlocks = calcBlocks(expRawBlocks, expRet != 0,         \
                                           cIsNewDelRsrc, cSharedAlloc);      \
            testCtor<F>(cAlloc, expRet, expCopyBlocks, expCopyBlocks,         \
                        UNPAREN cArgs);                                       \
        } while (false)

    allocator_resource *saveRsrc = allocator_resource::default_resource();

    {
    Obj f(ctorArgs...);

    // fNDR is true if alloc uses the new_delete_resource
    const bool fNDR = (FRSRC == new_delete_resource_singleton());

    // Test variations on copy construction.

    // NDR  = Use new_delete_resource for copy (true/false)
    //
    //             Copy ctor args                     Allocator     NDR
    //             =================================  ============  ===
    pushDefaultResource(nullptr);
    COPY_CTOR_TEST((f)                              , newDelRsrc  , 1  );
    COPY_CTOR_TEST((allocator_arg, PolyAllocTp(), f), newDelRsrc  , 1  );
    COPY_CTOR_TEST((allocator_arg, stdAlloc, f)     , newDelRsrc  , 1  );
    popDefaultResource();
    pushDefaultResource(&dfltTstRsrc);
    COPY_CTOR_TEST((f)                              , &dfltTstRsrc, 0  );
    COPY_CTOR_TEST((allocator_arg, PolyAllocTp(), f), &dfltTstRsrc, 0  );
    COPY_CTOR_TEST((allocator_arg, stdAlloc, f)     , newDelRsrc  , 1  );
    COPY_CTOR_TEST((allocator_arg, smplAlloc, f)    , smplAlloc   , 0  );
    COPY_CTOR_TEST((allocator_arg, &testRsrc, f)    , &testRsrc   , 0  );
    COPY_CTOR_TEST((allocator_arg, polyAlloc, f)    , polyAlloc   , 0  );
    COPY_CTOR_TEST((allocator_arg, FRSRC, f)        , FRSRC       , fNDR);
    COPY_CTOR_TEST((allocator_arg, alloc, f)        , FRSRC       , fNDR);
    popDefaultResource();
    }
#undef COPY_CTOR_TEST

// This macro tests the move constructor and extended move constructor.  It
// first constructs a new function object, 'f'.  If the allocator specified
// for the move is the same as the allocator used by 'f', then a true move
// will occur and no new space is expected to be allocated, otherwise, a copy
// will occur and enough space is expected be allocated to hold a copy of f.
// If a true move occurs, then the space being deallocated will be the space
// originally used by 'f', except that the block used by the shared pointer is
// shared by 'f' and the move-constructed function, and will not be
// deallocated when the move-constructed function is destroyed (which is why 1
// is subtracted from expDeallocBlocks for non-new-delete allocators).
// Otherwise, if a copy occurs, then destruction the copy will deallocate
// exactly as much space as was allocated.
#define MOVE_CTOR_TEST(mArgs, mAlloc, mIsNewDelRsrc) do {                     \
        if (veryVerbose)                                                      \
            std::cout << "        move function" #mArgs " "                   \
                      << defaultIsNewDelStr() << std::endl;                   \
        pushDefaultResource(saveRsrc);                                        \
        Obj f(ctorArgs...);                           \
        popDefaultResource();                                                 \
        int expAllocBlocks, expDeallocBlocks;                                 \
        if (getCounters(alloc) == getCounters(mAlloc)) {                      \
            expAllocBlocks = 0;                                               \
            expDeallocBlocks = expObjBlocks - (isNewDelRsrc ? 0 : 1);         \
        } else                                                                \
            expAllocBlocks = expDeallocBlocks =                               \
                calcBlocks(expRawBlocks, expRet != 0, mIsNewDelRsrc);         \
        testCtor<F>(mAlloc, expRet, expAllocBlocks, expDeallocBlocks,         \
                    UNPAREN mArgs);                                           \
    } while (false)

#define MF std::move(f)

// fNDR is true if alloc uses the new_delete_resource
#define fNDR (FRSRC == new_delete_resource_singleton())

    // NDR  = Use new_delete_resource for move (true/false)
    //
    //             Copy ctor args                      Allocator     NDR
    //             ==================================  ============  ===
    pushDefaultResource(nullptr);
    MOVE_CTOR_TEST((MF)                              , FRSRC       , fNDR);
    MOVE_CTOR_TEST((allocator_arg, stdAlloc, MF)     , newDelRsrc  , 1   );
    MOVE_CTOR_TEST((allocator_arg, PolyAllocTp(), MF), newDelRsrc  , 1   );
    popDefaultResource();
    pushDefaultResource(&dfltTstRsrc);
    MOVE_CTOR_TEST((MF)                              , FRSRC       , fNDR);
    MOVE_CTOR_TEST((allocator_arg, stdAlloc, MF)     , newDelRsrc  , 1   );
    MOVE_CTOR_TEST((allocator_arg, PolyAllocTp(), MF), &dfltTstRsrc, 0   );
    MOVE_CTOR_TEST((allocator_arg, smplAlloc, MF)    , smplAlloc   , 0   );
    MOVE_CTOR_TEST((allocator_arg, &testRsrc, MF)    , &testRsrc   , 0   );
    MOVE_CTOR_TEST((allocator_arg, polyAlloc, MF)    , polyAlloc   , 0   );
    MOVE_CTOR_TEST((allocator_arg, FRSRC, MF)        , FRSRC       , fNDR);
    MOVE_CTOR_TEST((allocator_arg, alloc, MF)        , FRSRC       , fNDR);
    popDefaultResource();

#undef MF
#undef MOVE_CTOR_TEST

#define ASSIGN_TEST(cpOrMv, aAlloc) do {                                      \
        if (veryVerbose)                                                      \
            std::cout << "        " #cpOrMv " assign w/ " #aAlloc <<std::endl;\
        test##cpOrMv##Assign(aAlloc, f, expRet, expRawBlocks);                \
    } while (false)

    {
    Obj f(ctorArgs...);

    ASSIGN_TEST(Copy, &dfltTstRsrc );
    ASSIGN_TEST(Copy, stdAlloc     );
    ASSIGN_TEST(Copy, PolyAllocTp());
    ASSIGN_TEST(Copy, smplAlloc    );
    ASSIGN_TEST(Copy, &testRsrc    );
    ASSIGN_TEST(Copy, polyAlloc    );
    ASSIGN_TEST(Copy, FRSRC        );
    ASSIGN_TEST(Copy, alloc        );

    ASSIGN_TEST(Move, &dfltTstRsrc );
    ASSIGN_TEST(Move, stdAlloc     );
    ASSIGN_TEST(Move, PolyAllocTp());
    ASSIGN_TEST(Move, smplAlloc    );
    ASSIGN_TEST(Move, &testRsrc    );
    ASSIGN_TEST(Move, polyAlloc    );
    ASSIGN_TEST(Move, FRSRC        );
    ASSIGN_TEST(Move, alloc        );

    }
#undef ASSIGN_TEST
#undef FRSRC

    if (veryVerbose)
        std::cout << "        hetero assign " <<std::endl;
    testHeteroAssign<F>(alloc, rhsFunc, expRet, expRawBlocks);
}

enum no_args_t { NO_ARGS };

// Special case of no constructor arguments
template <class F, class... CtorArgs>
inline
void testFunction(int expRet, int expRawBlocks, bool isNewDelRsrc, no_args_t)
{
    testFunction<F>(expRet, expRawBlocks, isNewDelRsrc);
}


//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    verbose = argc > 2;
    veryVerbose = argc > 3;
    // veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

    typedef int Sig(const char*);
    typedef XSTD::function<Sig> Obj;

    TestResource testRsrc;
    typedef POLYALLOC<char> PolyAllocTp;
    PolyAllocTp polyAlloc(&testRsrc);
    AllocCounters smplAllocCounters;
    SimpleAllocator<char>  smplAlloc(&smplAllocCounters);
    POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>)
        smplAllocAdaptor(smplAlloc);
    std::allocator<int> stdAlloc;

    // Stateless lambda
    auto lambda = [](const char* s) { return std::atoi(s); };

    // Stateful functor that uses an allocator
    TestResource tmpRsrc;  // Ignore this allocator resource
    Functor functor(1, &tmpRsrc);

    // Null function pointer
    int (*const nullFuncPtr)(const char*) = nullptr;

    // Prime the pump by making sure all singletons are allocated.
    // This step makes future allocation counts predictable.
    (void) XSTD::function<void()>().get_allocator_resource();
    newDelCounters.clear();

    switch (test) {
      case 0: // Do all cases for test-case 0
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING/USAGE TEST
        //
        // Concerns:
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nBREATHING TEST"
                  << "\n==============" << std::endl;

        if (verbose)
            std::cout << "    construct with no allocator" << std::endl;
        {
            XSTD::function<int(const char*)> f([](const char* s) {
                    return std::atoi(s); });
            ASSERT(newDelRsrc == f.get_allocator_resource());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == newDelCounters.blocks_outstanding());

        if (verbose)
            std::cout << "    construct with default allocator" << std::endl;
        pushDefaultResource(&dfltTstRsrc);
        allocator_resource::set_default_resource(&dfltTstRsrc);
        {
            XSTD::function<int(const char*)> f([](const char* s) {
                    return std::atoi(s); });
            ASSERT(&dfltTstRsrc == f.get_allocator_resource());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == dfltTstRsrc.counters().blocks_outstanding());
        popDefaultResource();

        if (verbose)
            std::cout << "    construct with SimpleAllocator" << std::endl;
        {
            XSTD::function<int(const char*)> f(allocator_arg, smplAlloc,
                                               [](const char* s) {
                                                   return std::atoi(s); });
            ASSERT(smplAllocAdaptor == *f.get_allocator_resource());
            // 2 blocks allocated, 1 for a copy of the functor, 1 for a copy
            // of the allocator:
            ASSERT(2 == smplAllocCounters.blocks_outstanding());
            ASSERT(0 == dfltTstRsrc.counters().blocks_outstanding());
            ASSERT(6 == f("6"));
        }
        ASSERT(0 == smplAllocCounters.blocks_outstanding());
        ASSERT(0 == dfltTstRsrc.counters().blocks_outstanding());

        if (verbose)
            std::cout << "    construct with allocator resource" << std::endl;
        {
            XSTD::function<int(const char*)> f(allocator_arg, &testRsrc,
                                               [](const char* s) {
                                                   return std::atoi(s); });
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(2 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == dfltTstRsrc.counters().blocks_outstanding());
            ASSERT(7 == f("7"));
        }

      } if (test != 0) break;

      case 2: {
        // --------------------------------------------------------------------
        // NO ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nNO ALLOCATOR"
                  << "\n============" << std::endl;

#define TEST(ctorArgs, isNewDelRsrc, expRet, expAlloc) do {                  \
        if (verbose) std::cout << "    function" #ctorArgs " "               \
                               << defaultIsNewDelStr() << std::endl;         \
        testFunction<Sig>(expRet, expAlloc, isNewDelRsrc, UNPAREN ctorArgs); \
    } while (false)

        // NDR  = new_delete_resource is the default allocator_resource
        //
        //   Ctor args      NDR Ret blocks
        //   =============  === === ======
        TEST((NO_ARGS)    ,  1,  0, 0     );
        TEST((nullptr)    ,  1,  0, 0     );
        TEST((nullFuncPtr),  1,  0, 0     );
        TEST((&std::atoi) ,  1, 74, 0     );
        TEST((lambda)     ,  1, 74, 1     );
        TEST((functor)    ,  1, 75, 2     );
        pushDefaultResource(&dfltTstRsrc);
        TEST((NO_ARGS)    ,  0,  0, 0     );
        TEST((nullptr)    ,  0,  0, 0     );
        TEST((nullFuncPtr),  0,  0, 0     );
        TEST((&std::atoi) ,  0, 74, 0     );
        TEST((lambda)     ,  0, 74, 1     );
        TEST((functor)    ,  0, 75, 2     );
        popDefaultResource();

#undef TEST

      } if (test != 0) break;

      case 3: {
        // --------------------------------------------------------------------
        // STANDARD ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nSTANDARD ALLOCATOR"
                  << "\n==================" << std::endl;

#define TEST(ctorArgs, expRet, expRawBlocks) do {                            \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            testFunction<Sig>(expRet, expRawBlocks, true, UNPAREN ctorArgs); \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                               Raw   
        //   Ctor args                              Ret Blocks 
        //   =====================================  === ====== 
        TEST((allocator_arg, stdAlloc)             ,  0, 0    );
        TEST((allocator_arg, stdAlloc, nullptr)    ,  0, 0    );
        TEST((allocator_arg, stdAlloc, nullFuncPtr),  0, 0    );
        TEST((allocator_arg, stdAlloc, &std::atoi) , 74, 0    );
        TEST((allocator_arg, stdAlloc, lambda)     , 74, 1    );
        TEST((allocator_arg, stdAlloc, functor)    , 75, 2    );

#undef TEST

      } if (test != 0) break;

      case 4: {
        // --------------------------------------------------------------------
        // TYPE-ERASED ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nTYPE-ERASED ALLOCATOR"
                  << "\n=====================" << std::endl;

#define TEST(ctorArgs, expRet, expRawBlocks) do {                             \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl;  \
            testFunction<Sig>(expRet, expRawBlocks, false, UNPAREN ctorArgs); \
        } while (false)

        // Always allocate one block for functor and one block for type-erased
        // allocator.

        //                                                Raw   
        //   Ctor args                               Ret Blocks 
        //   ======================================  === ====== 
        TEST((allocator_arg, smplAlloc)             ,  0, 0    );
        TEST((allocator_arg, smplAlloc, nullptr)    ,  0, 0    );
        TEST((allocator_arg, smplAlloc, nullFuncPtr),  0, 0    );
        TEST((allocator_arg, smplAlloc, &std::atoi) , 74, 0    );
        TEST((allocator_arg, smplAlloc, lambda)     , 74, 1    );
        TEST((allocator_arg, smplAlloc, functor)    , 75, 2    );

#undef TEST

      } if (test != 0) break;

      case 5: {
        // --------------------------------------------------------------------
        // ALLOCATOR_RESOURCE
        // --------------------------------------------------------------------
          
        std::cout << "\nALLOCATOR_RESOURCE"
                  << "\n==================" << std::endl;

#define TEST(ctorArgs, rsrc, expRet, expRawBlocks) do {                      \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            testFunction<Sig>(expRet, expRawBlocks,                          \
                              (newDelRsrc == &(rsrc)), UNPAREN ctorArgs);    \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                                               Raw
        //   Ctor args                                   Resource    Ret Blocks
        //   ==========================================  =========== === ======
        TEST((allocator_arg, &dfltTstRsrc)             , dfltTstRsrc,  0, 0 );
        TEST((allocator_arg, &dfltTstRsrc, nullptr)    , dfltTstRsrc,  0, 0 );
        TEST((allocator_arg, &dfltTstRsrc, nullFuncPtr), dfltTstRsrc,  0, 0 );
        TEST((allocator_arg, &dfltTstRsrc, &std::atoi) , dfltTstRsrc, 74, 0 );
        TEST((allocator_arg, &dfltTstRsrc, lambda)     , dfltTstRsrc, 74, 1 );
        TEST((allocator_arg, &dfltTstRsrc, functor)    , dfltTstRsrc, 75, 2 );

        TEST((allocator_arg, &testRsrc)                , testRsrc   ,  0, 0 );
        TEST((allocator_arg, &testRsrc, nullptr)       , testRsrc   ,  0, 0 );
        TEST((allocator_arg, &testRsrc, nullFuncPtr)   , testRsrc   ,  0, 0 );
        TEST((allocator_arg, &testRsrc, &std::atoi)    , testRsrc   , 74, 0 );
        TEST((allocator_arg, &testRsrc, lambda)        , testRsrc   , 74, 1 );
        TEST((allocator_arg, &testRsrc, functor)       , testRsrc   , 75, 2 );

#undef TEST

      } if (test != 0) break;

      case 6: {
        // --------------------------------------------------------------------
        // POLYMORPHIC_ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nPOLYMORPHIC_ALLOCATOR"
                  << "\n=====================" << std::endl;

#define TEST(ctorArgs, isDfltRsrc, expRet, expRawBlocks) do {                \
            if (verbose) std::cout << "    function" #ctorArgs " "           \
                                   << defaultIsNewDelStr() << std::endl;     \
            testFunction<Sig>(expRet, expRawBlocks, isDfltRsrc,              \
                              UNPAREN ctorArgs);                             \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                               Dflt      Raw
        //   Ctor args                                   Rsrc Ret Blocks
        //   =========================================== ==== === ======
        TEST((allocator_arg, PolyAllocTp())             ,   1,  0, 0    );
        TEST((allocator_arg, PolyAllocTp(), nullptr)    ,   1,  0, 0    );
        TEST((allocator_arg, PolyAllocTp(), nullFuncPtr),   1,  0, 0    );
        TEST((allocator_arg, PolyAllocTp(), &std::atoi) ,   1, 74, 0    );
        TEST((allocator_arg, PolyAllocTp(), lambda)     ,   1, 74, 1    );
        TEST((allocator_arg, PolyAllocTp(), functor)    ,   1, 75, 2    );

        TEST((allocator_arg, polyAlloc)                 ,   0,  0, 0    );
        TEST((allocator_arg, polyAlloc, nullptr)        ,   0,  0, 0    );
        TEST((allocator_arg, polyAlloc, nullFuncPtr)    ,   0,  0, 0    );
        TEST((allocator_arg, polyAlloc, &std::atoi)     ,   0, 74, 0    );
        TEST((allocator_arg, polyAlloc, lambda)         ,   0, 74, 1    );
        TEST((allocator_arg, polyAlloc, functor)        ,   0, 75, 2    );

#undef TEST

      } if (test != 0) break;

      break;

      default: {
        std::cerr << "ERROR: CASE " << test << " NOT FOUND." << std::endl;
        ++testStatus;
      }
    } // End switch

    if (testStatus > 0) {
        std::cerr << "Error, non-zero test status = " << testStatus
                  << std::endl;
    }

    return testStatus;
}

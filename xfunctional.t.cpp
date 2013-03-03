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

    union Header {
        void*       m_align;
        std::size_t m_size;
    };

  public:
    AllocCounters()
	: m_num_allocs(0)
	, m_num_deallocs(0)
	, m_bytes_allocated(0)
	, m_bytes_deallocated(0)
	{ }

    ~AllocCounters() {
        ASSERT(0 == blocks_outstanding());
        ASSERT(0 == bytes_outstanding());
    }

    void* allocate(std::size_t nbytes) {
	ASSERT(this != nullptr);
        std::size_t blocksize =
            (nbytes + 2*sizeof(Header) - 1) & ~(sizeof(Header)-1);
        Header* ret = static_cast<Header*>(operator new(blocksize));
        ret->m_size = nbytes;
        ++ret;
        ++m_num_allocs;
        m_bytes_allocated += nbytes;
        return ret;
    }

    void deallocate(void* p, std::size_t nbytes) {
        Header* h = static_cast<Header*>(p) - 1;
        ASSERT(nbytes == h->m_size);
	h->m_size = 0xdeadbeaf;
        operator delete((void*)h);
        ++m_num_deallocs;
        m_bytes_deallocated += nbytes;
    }

    int blocks_outstanding() const { return m_num_allocs - m_num_deallocs; }
    int bytes_outstanding() const
        { return m_bytes_allocated - m_bytes_deallocated; }

    void dump(std::ostream& os, const char* msg) {
	os << msg << ":\n";
        os << "  num allocs = " << m_num_allocs << '\n';
        os << "  num deallocs = " << m_num_deallocs << '\n';
        os << "  outstanding allocs = " << blocks_outstanding() << '\n';
        os << "  bytes allocated = " << m_bytes_allocated << '\n';
        os << "  bytes deallocated = " << m_bytes_deallocated << '\n';
        os << "  outstanding bytes = " << bytes_outstanding() << '\n';
        os << std::endl;
    }

    void clear() {
	m_num_allocs = 0;
	m_num_deallocs = 0;
	m_bytes_allocated = 0;
	m_bytes_deallocated = 0;
    }

    // // Merge 'other' into '*this' and clear 'other'.
    // void merge(AllocCounters& other) {
    //     m_num_allocs        += other.m_num_allocs;
    //     m_num_deallocs      += other.m_num_deallocs;
    //     m_bytes_allocated   += other.m_bytes_allocated;
    //     m_bytes_deallocated += other.m_bytes_deallocated;
    //     other.clear();
    // }
};

// Abbreviation for 'XSTD::polyalloc::allocator_resource'.
typedef XSTD::polyalloc::allocator_resource allocator_resource;

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

    const AllocCounters& counters() const { return m_counters; }
    void clear() { m_counters.clear(); }
};

TestResource::~TestResource()
{
    ASSERT(0 == m_counters.blocks_outstanding());
}

void* TestResource::allocate(size_t bytes, size_t alignment)
{
    return m_counters.allocate(bytes);
}

void  TestResource::deallocate(void *p, size_t bytes, size_t alignment)
{
    m_counters.deallocate(p, bytes);
}

bool TestResource::is_equal(const allocator_resource& other) const
{
    // Two TestResource objects are equal only if they are the same object
    return this == &other;
}

TestResource dfltTstRsrc;
AllocCounters& globalCounters =
    const_cast<AllocCounters&>(dfltTstRsrc.counters());

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

    SimpleAllocator(AllocCounters* c = &globalCounters) : m_counters(c) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : m_counters(other.m_counters) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(m_counters->allocate(n*sizeof(Tp))); }

    void deallocate(Tp* p, std::size_t n)
        { m_counters->deallocate(p, n*sizeof(Tp)); }

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

// Return the address of the global counters.
template <class T>
const AllocCounters *getCounters(const std::allocator<T>& alloc)
{
    return &globalCounters;
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
    typedef POLYALLOC_RESOURCE_ADAPTOR(std::allocator<char> ) StdAdaptor;

    const TestResource *tstRsrc = dynamic_cast<const TestResource*>(alloc);
    if (tstRsrc)
        return &tstRsrc->counters();
    const SmplAdaptor *aRsrc = dynamic_cast<const SmplAdaptor*>(alloc);
    if (aRsrc)
        return getCounters(aRsrc->get_allocator());
    const StdAdaptor *stdRsrc = dynamic_cast<const StdAdaptor*>(alloc);
    if (stdRsrc)
        return &globalCounters;
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
// default allocator, and whether the allocator is shared with another
// function object.
int calcBlocks(int rawCount, bool isInvocable, bool isDefaultAlloc,
               bool sharedAlloc = false)
{
    // If the allocator is not the default allocator, then we cannot use the
    // small-object optimization and the block count must be at least one.
    // This minimum allocation does not apply to non-invocable (i.e.,
    // empty/null) function objects.
    if (isInvocable && ! isDefaultAlloc)
        rawCount = std::max(rawCount, 1);

    // The use of a non-default allocator requires the creation of a shared
    // pointer, which increases the block count by one.
    if (! isDefaultAlloc && ! sharedAlloc)
        ++rawCount;

    return rawCount;
}

// Given a list of 0 to 3 XSTD::function constructor arguments, return the
// allocator argument, if any, or the default allocator if none.
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
    const int initialGlobalBlocks = globalCounters.blocks_outstanding();
    int expBlocks = counters->blocks_outstanding();

    {
        Obj f(std::forward<CtorArgs>(ctorArgs)...);

        expBlocks += expAllocBlocks;
        ASSERT(getCounters(f.get_allocator_resource()) == counters);
        ASSERT(counters->blocks_outstanding() == expBlocks);
        if (counters != &globalCounters)
            ASSERT(globalCounters.blocks_outstanding() == initialGlobalBlocks);
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
    if (counters != &globalCounters)
        ASSERT(globalCounters.blocks_outstanding() == initialGlobalBlocks);
}    

template <class F, class A, class Rhs, class... LhsCtorArgs>
void testAssignment(const A& alloc, Rhs&& rhs, int expRet,
                    int expAllocBlocks, int expDeallocBlocks,
                    LhsCtorArgs&&... lhsCtorArgs)
{
    typedef XSTD::function<F> Obj;

    const AllocCounters *const counters = getCounters(alloc);
    int expBlocks = counters->blocks_outstanding();
    const int initialGlobalBlocks = globalCounters.blocks_outstanding();

    {
        // Construct lhs, then assign to it.  Memory used by the constructor
        // should be returned to the heap, so only the memory from the copy
        // should remain outstanding.
        Obj lhs(std::forward<LhsCtorArgs>(lhsCtorArgs)...);
        lhs = std::forward<Rhs>(rhs);

        expBlocks += expAllocBlocks;
        ASSERT(getCounters(lhs.get_allocator_resource()) == counters);
        ASSERT(counters->blocks_outstanding() == expBlocks);
        if (counters != &globalCounters)
            ASSERT(globalCounters.blocks_outstanding() == initialGlobalBlocks);
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
    if (counters != &globalCounters)
        ASSERT(globalCounters.blocks_outstanding() == initialGlobalBlocks);
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
    bool isDefaultAlloc =
        (counters == getCounters(allocator_resource::default_resource()));
    bool sharedAlloc = (counters == getCounters(rhs.get_allocator_resource()));

    int expCopyBlocks = calcBlocks(expRawBlocks, expRet != 0,
                                   isDefaultAlloc, sharedAlloc);

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
    bool isDefaultAlloc =
        (counters == getCounters(allocator_resource::default_resource()));

    int expAllocBlocks;
    int expDeallocBlocks = calcBlocks(expRawBlocks, expRet != 0,
                                      isDefaultAlloc);

    if (counters == getCounters(rhs.get_allocator_resource())) {
        expAllocBlocks = 0;
        expDeallocBlocks -= (isDefaultAlloc ? 0 : 1);
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
    bool isDefaultAlloc =
        (counters == getCounters(allocator_resource::default_resource()));

    int expBlocks = calcBlocks(expRawBlocks, expRet != 0, isDefaultAlloc);

#define FWD_RHS std::forward<Rhs>(rhs)

    // Test assignment with initial lhs having each possible functor type.
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc);
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, nullptr);
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, nullFuncPtr);
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, &std::atoi);
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, lambda);
    testAssignment<F>(alloc, FWD_RHS, expRet, expBlocks, expBlocks,
                      allocator_arg, alloc, functor);

#undef FWD_RHS
}

// Test construction, copy construction, and move construction of
// XSTD::function
template <class F, class... CtorArgs>
void testFunction(int expRet,
                  int expRawBlocks, bool isDefaultAlloc,
                  CtorArgs&&... ctorArgs)
{
    typedef XSTD::function<F> Obj;

    auto alloc = allocatorFromCtorArgs(std::forward<CtorArgs>(ctorArgs)...);
    auto rhsFunc = functorFromCtorArgs(std::forward<CtorArgs>(ctorArgs)...);

    // Compute expected blocks needed to construct function.
    int expObjBlocks = calcBlocks(expRawBlocks, expRet != 0, isDefaultAlloc);

    // Test function constructor
    testCtor<F>(alloc, expRet, expObjBlocks, expObjBlocks,
                std::forward<CtorArgs>(ctorArgs)...);

    TestResource testRsrc;
    POLYALLOC<char> polyAlloc(&testRsrc);
    POLYALLOC<char> dfltPolyAlloc(&dfltTstRsrc);
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
#define COPY_CTOR_TEST(cArgs, cAlloc, cIsDefaultAlloc) do {                   \
            if (veryVerbose)                                                  \
                std::cout << "        copy function" #cArgs << std::endl;     \
            bool cSharedAlloc = (getCounters(cAlloc) == getCounters(FRSRC));  \
            int expCopyBlocks = calcBlocks(expRawBlocks, expRet != 0,         \
                                           cIsDefaultAlloc, cSharedAlloc);    \
            testCtor<F>(cAlloc, expRet, expCopyBlocks, expCopyBlocks,         \
                        UNPAREN cArgs);                                       \
        } while (false)

    {
    Obj f(std::forward<CtorArgs>(ctorArgs)...);

    // fDA is true if alloc uses the default allocator resource
    const bool fDA = (FRSRC == allocator_resource::default_resource());

    // Test variations on copy construction.

    // DA  = Use default allocator resource for copy (true/false)
    //
    //             Copy ctor args                     Allocator     DA
    //             =================================  ============  ==
    COPY_CTOR_TEST((f)                              , &dfltTstRsrc, 1  );
//  COPY_CTOR_TEST((allocator_arg, stdAlloc, f)     , &dfltTstRsrc, 1  );
    COPY_CTOR_TEST((allocator_arg, dfltPolyAlloc, f), &dfltTstRsrc, 1  );
    COPY_CTOR_TEST((allocator_arg, smplAlloc, f)    , smplAlloc   , 0  );
    COPY_CTOR_TEST((allocator_arg, &testRsrc, f)    , &testRsrc   , 0  );
    COPY_CTOR_TEST((allocator_arg, polyAlloc, f)    , polyAlloc   , 0  );
    COPY_CTOR_TEST((allocator_arg, FRSRC, f)        , FRSRC       , fDA);
    COPY_CTOR_TEST((allocator_arg, alloc, f)        , FRSRC       , fDA);
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
// is subtracted from expDeallocBlocks for non-default allocators).
// Otherwise, if a copy occurs, then destruction the copy will deallocate
// exactly as much space as was allocated.
#define MOVE_CTOR_TEST(mArgs, mAlloc, mIsDefaultAlloc) do {                   \
        if (veryVerbose)                                                      \
            std::cout << "        move function" #mArgs << std::endl;         \
        Obj f(std::forward<CtorArgs>(ctorArgs)...);                           \
        int expAllocBlocks, expDeallocBlocks;                                 \
        if (getCounters(alloc) == getCounters(mAlloc)) {                      \
            expAllocBlocks = 0;                                               \
            expDeallocBlocks = expObjBlocks - (isDefaultAlloc ? 0 : 1);       \
        } else                                                                \
            expAllocBlocks = expDeallocBlocks =                               \
                calcBlocks(expRawBlocks, expRet != 0, mIsDefaultAlloc);       \
        testCtor<F>(mAlloc, expRet, expAllocBlocks, expDeallocBlocks,         \
                    UNPAREN mArgs);                                           \
    } while (false)

    const bool NA = false;  // NA == not-applicable/don't care

#define MF std::move(f)

    // DA  = Use default allocator resource for move (true/false)
    //
    //             Copy ctor args                      Allocator     DA
    //             ==================================  ============  ==
    MOVE_CTOR_TEST((MF)                              , FRSRC       , 1 );
//  MOVE_CTOR_TEST((allocator_arg, stdAlloc, MF)     , &dfltTstRsrc, 1 );
    MOVE_CTOR_TEST((allocator_arg, dfltPolyAlloc, MF), &dfltTstRsrc, 1 );
    MOVE_CTOR_TEST((allocator_arg, smplAlloc, MF)    , smplAlloc   , 0 );
    MOVE_CTOR_TEST((allocator_arg, &testRsrc, MF)    , &testRsrc   , 0 );
    MOVE_CTOR_TEST((allocator_arg, polyAlloc, MF)    , polyAlloc   , 0 );
    MOVE_CTOR_TEST((allocator_arg, FRSRC, MF)        , FRSRC       , NA);
    MOVE_CTOR_TEST((allocator_arg, alloc, MF)        , FRSRC       , NA);

#undef MF
#undef MOVE_CTOR_TEST

#define ASSIGN_TEST(cpOrMv, aAlloc) do {                                      \
        if (veryVerbose)                                                      \
            std::cout << "        " #cpOrMv " assign w/ " #aAlloc <<std::endl;\
        test##cpOrMv##Assign(aAlloc, f, expRet, expRawBlocks);                \
    } while (false)

    {
    Obj f(std::forward<CtorArgs>(ctorArgs)...);

    ASSIGN_TEST(Copy, &dfltTstRsrc );
//  ASSIGN_TEST(Copy, stdAlloc     );
    ASSIGN_TEST(Copy, dfltPolyAlloc);
    ASSIGN_TEST(Copy, smplAlloc    );
    ASSIGN_TEST(Copy, &testRsrc    );
    ASSIGN_TEST(Copy, polyAlloc    );
    ASSIGN_TEST(Copy, FRSRC        );
    ASSIGN_TEST(Copy, alloc        );

    ASSIGN_TEST(Move, &dfltTstRsrc );
//  ASSIGN_TEST(Move, stdAlloc     );
    ASSIGN_TEST(Move, dfltPolyAlloc);
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
void testFunction(int expRet, int expRawBlocks,
                  bool isDefaultAlloc, no_args_t)
{
    testFunction<F>(expRet, expRawBlocks, isDefaultAlloc);
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

    allocator_resource::set_default_resource(&dfltTstRsrc);

    TestResource testRsrc;
    POLYALLOC<char> polyAlloc(&testRsrc);
    POLYALLOC<char> dfltPolyAlloc(&dfltTstRsrc);
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
            ASSERT(&dfltTstRsrc == f.get_allocator_resource());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == dfltTstRsrc.counters().blocks_outstanding());

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

#define TEST(ctorArgs, expRet, expAlloc) do {                                \
        if (verbose) std::cout << "    function" #ctorArgs << std::endl;     \
        testFunction<Sig>(expRet, expAlloc, true, UNPAREN ctorArgs);         \
    } while (false)

        //   Ctor args      Ret blocks
        //   =============  === ======
        TEST((NO_ARGS)    ,  0, 0     );
        TEST((nullptr)    ,  0, 0     );
        TEST((nullFuncPtr),  0, 0     );
        TEST((&std::atoi) , 74, 0     );
        TEST((lambda)     , 74, 1     );
        TEST((functor)    , 75, 2     );

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

#if 0
        //                                               Raw   
        //   Ctor args                              Ret Blocks 
        //   =====================================  === ====== 
        TEST((allocator_arg, stdAlloc)             ,  0, 0    );
        TEST((allocator_arg, stdAlloc, nullptr)    ,  0, 0    );
        TEST((allocator_arg, stdAlloc, nullFuncPtr),  0, 0    );
        TEST((allocator_arg, stdAlloc, &std::atoi) , 74, 0    );
        TEST((allocator_arg, stdAlloc, lambda)     , 74, 1    );
        TEST((allocator_arg, stdAlloc, functor)    , 75, 2    );
#endif

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
                              (&dfltTstRsrc == &(rsrc)), UNPAREN ctorArgs);  \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                                                Raw
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
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            testFunction<Sig>(expRet, expRawBlocks, isDfltRsrc,              \
                              UNPAREN ctorArgs);                             \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                               Dflt      Raw
        //   Ctor args                                   Rsrc Ret Blocks
        //   =========================================== ==== === ======
        TEST((allocator_arg, dfltPolyAlloc)             ,   1,  0, 0    );
        TEST((allocator_arg, dfltPolyAlloc, nullptr)    ,   1,  0, 0    );
        TEST((allocator_arg, dfltPolyAlloc, nullFuncPtr),   1,  0, 0    );
        TEST((allocator_arg, dfltPolyAlloc, &std::atoi) ,   1, 74, 0    );
        TEST((allocator_arg, dfltPolyAlloc, lambda)     ,   1, 74, 1    );
        TEST((allocator_arg, dfltPolyAlloc, functor)    ,   1, 75, 2    );

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

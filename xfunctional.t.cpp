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
};

class TestResource : public XSTD::polyalloc::allocator_resource
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

TestResource dfltTestRsrc;
AllocCounters& globalCounters =
    const_cast<AllocCounters&>(dfltTestRsrc.counters());

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

// If the dynamic type of 'alloc' is 'TestResource', return the address of the
// 'AllocCounters' object managed by 'alloc'. Otherwise, if the dynamic type
// of 'alloc' is a 'resource_adaptor<SimpleAllocator<T>>', return the adddress
// of the 'AllocCounters' object managed by the 'SimpleAllocator' within the
// adaptor. Otherwise, return NULL.  Note that if a 'function' is constructed
// with an 'allocator_resource*', it should store the exact pointer, not a
// pointer to a copy of the resource.
const AllocCounters *
getCounters(XSTD::polyalloc::allocator_resource *const &alloc)
{
    typedef POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char> ) RsrcAdaptor;
    const TestResource *tstRsrc = dynamic_cast<const TestResource*>(alloc);
    if (tstRsrc)
        return &tstRsrc->counters();
    const RsrcAdaptor *aRsrc = dynamic_cast<const RsrcAdaptor*>(alloc);
    if (aRsrc)
        return getCounters(aRsrc->get_allocator());
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

#define POLYALLOC XSTD::polyalloc::polymorphic_allocator

template <class F, class A>
void testFunction(XSTD::function<F>& f, const A& alloc, int expRet,
                  int expBlocks)
{
    const AllocCounters *const expCounters = getCounters(alloc);
    ASSERT(expCounters == getCounters(f.get_allocator_resource()));
    ASSERT(expBlocks == expCounters->blocks_outstanding());
    if (expCounters != &globalCounters)
        ASSERT(0 == globalCounters.blocks_outstanding());
    if (expRet)
    {
        ASSERT(f);
        ASSERT(expRet == f("74"));
    }
    // TBD: This test fails for empty functions with allocators
    // else
    //     ASSERT(! f);
}    

template <class F>
void
testCopyAndMove(XSTD::function<F>& f, int expRet, int expRaw, int expAlloc)
{
    typedef XSTD::function<F> Obj;

    TestResource testRsrc;
    POLYALLOC<char> polyAlloc(&testRsrc);
    AllocCounters sAllocCounters;
    SimpleAllocator<char>  sAlloc(&sAllocCounters);
    POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>) sAllocAdaptor(sAlloc);

#define COPY_TEST(ctorArgs, alloc, ignore) do {                               \
        if (veryVerbose)                                                      \
            std::cout << "        copy function" #ctorArgs << std::endl;      \
        const AllocCounters* const counters = getCounters(alloc);             \
        int expCount = expRaw;                                                \
        if (counters != &globalCounters) {                                    \
            ++expCount;                                                       \
            if (0 == expRaw) ++expCount;                                      \
        }                                                                     \
        int startBlocks = counters->blocks_outstanding();                     \
        {                                                                     \
            Obj f2 ctorArgs;                                                  \
            testFunction(f2, (alloc), (expRet), startBlocks + expCount);      \
        }                                                                     \
        ASSERT(startBlocks == counters->blocks_outstanding());                \
    } while (false)

    COPY_TEST((f)                          , &dfltTestRsrc, expRaw);
    COPY_TEST((allocator_arg, sAlloc, f)   , sAlloc       , expRaw);
    COPY_TEST((allocator_arg, &testRsrc, f), &testRsrc    , expAlloc);
    COPY_TEST((allocator_arg, polyAlloc, f), polyAlloc    , expAlloc);
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

//     int verbose = argc > 2;
//     int veryVerbose = argc > 3;
//     int veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

    XSTD::polyalloc::allocator_resource::set_default_resource(&dfltTestRsrc);

    TestResource testRsrc;
    POLYALLOC<char> polyAlloc(&testRsrc);
    AllocCounters sAllocCounters;
    SimpleAllocator<char>  sAlloc(&sAllocCounters);
    POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>) sAllocAdaptor(sAlloc);

    // Stateless lambda
    auto lambda = [](const char* s) { return std::atoi(s); };

    // Stateful functor that uses an allocator
    TestResource tmpRsrc;  // Ignore this allocator resource
    Functor functor(1, &tmpRsrc);

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
            ASSERT(&dfltTestRsrc == f.get_allocator_resource());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());

        if (verbose)
            std::cout << "    construct with SimplAllocator" << std::endl;
        {
            XSTD::function<int(const char*)> f(allocator_arg, sAlloc,
                                               [](const char* s) {
                                                   return std::atoi(s); });
            ASSERT(sAllocAdaptor == *f.get_allocator_resource());
            // 2 blocks allocated, 1 for a copy of the functor, 1 for a copy
            // of the allocator:
            ASSERT(2 == sAllocCounters.blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(6 == f("6"));
        }
        ASSERT(0 == sAllocCounters.blocks_outstanding());
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());

        if (verbose)
            std::cout << "    construct with allocator resource" << std::endl;
        {
            XSTD::function<int(const char*)> f(allocator_arg, &testRsrc,
                                               [](const char* s) {
                                                   return std::atoi(s); });
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(7 == f("7"));
        }

      } if (test != 0) break;

      case 2: {
        // --------------------------------------------------------------------
        // NO ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nNO ALLOCATOR"
                  << "\n============" << std::endl;

        typedef XSTD::function<int(const char*)> Obj;

#define TEST(ctorArgs, expRet, expAlloc) do {                                \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            {                                                                \
                Obj f ctorArgs;                                              \
                testFunction(f, &dfltTestRsrc, expRet, expAlloc);            \
                testCopyAndMove(f, expRet, expAlloc, expAlloc);              \
            }                                                                \
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());       \
            dfltTestRsrc.clear();                                            \
        } while (false)

        //   Ctor args     Ret blocks
        //   ============  === ======
        TEST(            ,  0, 0     );
        TEST((nullptr)   ,  0, 0     );
        TEST((&std::atoi), 74, 0     );
        TEST((lambda)    , 74, 1     );
        TEST((functor)   , 75, 2     );

#undef TEST

      } if (test != 0) break;

      case 3: {
        // --------------------------------------------------------------------
        // TYPE-ERASED ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nTYPE-ERASED ALLOCATOR"
                  << "\n=====================" << std::endl;

        typedef XSTD::function<int(const char*)> Obj;

#define TEST(ctorArgs, expRet, expRaw, expAlloc) do {                        \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            {                                                                \
                Obj f ctorArgs;                                              \
                testFunction(f, sAlloc, expRet, expAlloc);                   \
                testCopyAndMove(f, expRet, expRaw, expAlloc);                \
            }                                                                \
            ASSERT(0 == sAllocCounters.blocks_outstanding());                \
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());       \
            sAllocCounters.clear();                                          \
            dfltTestRsrc.clear();                                            \
        } while (false)

        // Always allocate one block for functor and one block for type-erased
        // allocator.

        //                                             Raw   Alloc
        //   Ctor args                            Ret Blocks Blocks
        //   ===================================  === ====== ======
        TEST((allocator_arg, sAlloc)            ,  0, 0    , 2     );
        // g++ 4.6.3 internal error if use nullptr           
        // TEST((allocator_arg, sAlloc, nullptr),  0, 0    , 2     );
        TEST((allocator_arg, sAlloc, 0)         ,  0, 0    , 2     );
        TEST((allocator_arg, sAlloc, &std::atoi), 74, 0    , 2     );
        TEST((allocator_arg, sAlloc, lambda)    , 74, 1    , 2     );
        TEST((allocator_arg, sAlloc, functor)   , 75, 2    , 3     );

#undef TEST

      } if (test != 0) break;

      case 4: {
        // --------------------------------------------------------------------
        // ALLOCATOR_RESOURCE
        // --------------------------------------------------------------------
          
        std::cout << "\nALLOCATOR_RESOURCE"
                  << "\n==================" << std::endl;

        typedef XSTD::function<int(const char*)> Obj;

#define TEST(ctorArgs, expRet, expRaw, expAlloc) do {                        \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            {                                                                \
                Obj f ctorArgs;                                              \
                testFunction(f, &testRsrc, expRet, expAlloc);                \
                testCopyAndMove(f, expRet, expRaw, expAlloc);                \
            }                                                                \
            ASSERT(0 == testRsrc.counters().blocks_outstanding());           \
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());       \
            testRsrc.clear();                                                \
            dfltTestRsrc.clear();                                            \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                                Raw   Alloc
        //   Ctor args                               Ret Blocks Blocks
        //   ======================================  === ====== ======
        TEST((allocator_arg, &testRsrc)            ,  0, 0    , 1     );
        // g++ 4.6.3 internal error if use nullptr              
        // TEST((allocator_arg, &testRsrc, nullptr),  0, 0    , 1     );
        TEST((allocator_arg, &testRsrc, 0)         ,  0, 0    , 1     );
        TEST((allocator_arg, &testRsrc, &std::atoi), 74, 0    , 1     );
        TEST((allocator_arg, &testRsrc, lambda)    , 74, 1    , 1     );
        TEST((allocator_arg, &testRsrc, functor)   , 75, 2    , 2     );

#undef TEST

      } if (test != 0) break;

      case 5: {
        // --------------------------------------------------------------------
        // POLYMORPHIC_ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nPOLYMORPHIC_ALLOCATOR"
                  << "\n=====================" << std::endl;

        typedef XSTD::function<int(const char*)> Obj;

#define TEST(ctorArgs, expRet, expRaw, expAlloc) do {                        \
            if (verbose) std::cout << "    function" #ctorArgs << std::endl; \
            {                                                                \
                Obj f ctorArgs;                                              \
                testFunction(f, polyAlloc, expRet, expAlloc);                \
                testCopyAndMove(f, expRet, expRaw, expAlloc);                \
            }                                                                \
            ASSERT(0 == testRsrc.counters().blocks_outstanding());           \
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());       \
            testRsrc.clear();                                                \
            dfltTestRsrc.clear();                                            \
        } while (false)

        // Always allocate one block for functor but no additional blocks for
        // allocator.

        //                                                Raw   Alloc
        //   Ctor args                               Ret Blocks Blocks
        //   ======================================  === ====== ======
        TEST((allocator_arg, polyAlloc)            ,  0, 0    , 1     );
        // g++ 4.6.3 internal error if use nullptr              
        // TEST((allocator_arg, polyAlloc, nullptr),  0, 0    , 1     );
        TEST((allocator_arg, polyAlloc, 0)         ,  0, 0    , 1     );
        TEST((allocator_arg, polyAlloc, &std::atoi), 74, 0    , 1     );
        TEST((allocator_arg, polyAlloc, lambda)    , 74, 1    , 1     );
        TEST((allocator_arg, polyAlloc, functor)   , 75, 2    , 2     );

#undef TEST

      } if (test != 0) break;

      case 6: {
        // --------------------------------------------------------------------
        // COPY CONSTRUCTION (incomplete)
        // --------------------------------------------------------------------
          
        std::cout << "\nCOPY CONSTRUCTION"
                  << "\n=================" << std::endl;

        {
            TestResource testRsrc1, testRsrc2;
            XSTD::function<int(const char*)> f1(allocator_arg, &testRsrc1,
                                                lambda);
            ASSERT(&testRsrc1 == f1.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(0 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(8 == f1("8"));

            XSTD::polyalloc::polymorphic_allocator<char> testAlloc2(&testRsrc2);
            XSTD::function<int(const char*)> f2(allocator_arg, &testRsrc2, f1);
            ASSERT(&testRsrc2 == f2.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(9 == f2("9"));

            XSTD::function<int(const char*)> f3(f1);
            ASSERT(XSTD::polyalloc::allocator_resource::default_resource() ==
                   f3.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(1 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(10 == f3("10"));
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());

        {
            XSTD::function<int(const char*)> f0(std::atoi);
            ASSERT(XSTD::polyalloc::allocator_resource::default_resource() ==
                   f0.get_allocator_resource());
            ASSERT(0 == f0("0"));

            TestResource testRsrc1, testRsrc2;
            XSTD::function<int(const char*)> f1(allocator_arg, &testRsrc1, f0);
            ASSERT(&testRsrc1 == f1.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(0 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(8 == f1("8"));

            XSTD::polyalloc::polymorphic_allocator<char> testAlloc2(&testRsrc2);
            XSTD::function<int(const char*)> f2(allocator_arg, &testRsrc2, f1);
            ASSERT(&testRsrc2 == f2.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(9 == f2("9"));

            XSTD::function<int(const char*)> f3(f1);
            ASSERT(XSTD::polyalloc::allocator_resource::default_resource() ==
                   f3.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(10 == f3("10"));
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());

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

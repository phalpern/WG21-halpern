// xfunctional.t.cpp                  -*-C++-*-

#include "xfunctional.h"

#include <iostream>
#include <cstdlib>

using namespace std;

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//
//

//-----------------------------------------------------------------------------

//==========================================================================
//                  ASSERT TEST MACRO
//--------------------------------------------------------------------------

namespace {

int testStatus = 0;
bool verbose = false;

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

int func(const char* s)
{
    return std::atoi(s);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? atoi(argv[1]) : 0;
    verbose = argc > 2;
    // veryVerbose = argc > 3;
    // veryVeryVerbose = argc > 4;

//     int verbose = argc > 2;
//     int veryVerbose = argc > 3;
//     int veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

#define POLYALLOC XSTD::polyalloc::polymorphic_allocator

    XSTD::polyalloc::allocator_resource::set_default_resource(&dfltTestRsrc);

    TestResource testRsrc;
    POLYALLOC<char> polyAlloc(&testRsrc);
    SimpleAllocator<char>  sAlloc(
        const_cast<AllocCounters*>(&testRsrc.counters()));
    POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>) sAllocAdaptor(sAlloc);

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

        if (verbose) std::cout << "construct with no allocator" << std::endl;
        {
            XSTD::function<int(const char*)> f([](const char* s) {
                    return std::atoi(s); });
            ASSERT(&dfltTestRsrc == f.get_allocator_resource());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());

        if (verbose)
            std::cout << "construct with SimplAllocator" << std::endl;
        {
            XSTD::function<int(const char*)> f(allocator_arg, sAlloc,
                                               [](const char* s) {
                                                   return std::atoi(s); });
            ASSERT(sAllocAdaptor == *f.get_allocator_resource());
            // 2 blocks allocated, 1 for a copy of the functor, 1 for the copy
            // of the allocator:
            ASSERT(2 == sAlloc.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(6 == f("6"));
        }

        if (verbose)
            std::cout << "construct with allocator resource" << std::endl;
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
        // TEST CONSTRUCTION WITH ALLOCATOR
        // --------------------------------------------------------------------
          
        std::cout << "\nCONSTRUCTION WITH ALLOCATOR"
                  << "\n===========================" << std::endl;

        auto lambda = [](const char* s) { return std::atoi(s); };

        dfltTestRsrc.clear();
        testRsrc.clear();

        if (verbose) std::cout << "    function()" << std::endl;
        {
            XSTD::function<int(const char*)> f;
            ASSERT(&dfltTestRsrc == f.get_allocator_resource());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
        dfltTestRsrc.clear();

        if (verbose) std::cout << "    function(F)" << std::endl;
        {
            XSTD::function<int(const char*)> f(lambda);
            ASSERT(&dfltTestRsrc == f.get_allocator_resource());
            ASSERT(1 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
        dfltTestRsrc.clear();

        if (verbose)
            std::cout << "    function(allocator_arg, A)" << std::endl;
        {
            XSTD::polyalloc::polymorphic_allocator<char> testAlloc(&testRsrc);
            XSTD::function<int(const char*)> f(allocator_arg, testAlloc);
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
        }
        ASSERT(0 == testRsrc.counters().blocks_outstanding());
        ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
        dfltTestRsrc.clear();

        if (verbose)
            std::cout << "    function(allocator_arg, A, F)" << std::endl;
        {
            TestResource testRsrc;
            XSTD::polyalloc::polymorphic_allocator<char> testAlloc(&testRsrc);
            XSTD::function<int(const char*)> f(allocator_arg, testAlloc,
                                               lambda);
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(6 == f("6"));
        }

        {
            TestResource testRsrc;
            XSTD::function<int(const char*)> f(allocator_arg, &testRsrc,
                                               lambda);
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == dfltTestRsrc.counters().blocks_outstanding());
            ASSERT(7 == f("7"));
        }

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
            XSTD::function<int(const char*)> f0(func);
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

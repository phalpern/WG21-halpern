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
//                  STANDARD BDE ASSERT TEST MACRO
//--------------------------------------------------------------------------
// NOTE: THIS IS A LOW-LEVEL COMPONENT AND MAY NOT USE ANY C++ LIBRARY
// FUNCTIONS, INCLUDING IOSTREAMS.

namespace {

int testStatus = 0;

int verbose = 0;
int veryVerbose = 0;
int veryVeryVerbose = 0;

void aSsErT(int c, const char *s, int i) {
    if (c) {
        // NOTE: This implementation macros must use printf since
        //       cout uses new and must not be called during exception testing.
        printf(__FILE__ ":%d: Error: %s    (failed)\n", i, s);
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

}  // close unnamed namespace

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
//-----------------------------------------------------------------------------
// NOTE: This implementation of LOOP_ASSERT macros must use printf since
//       cout uses new and must not be called during exception testing.

#define LOOP_ASSERT(I,X) { \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) {                    \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP4_ASSERT(I,J,K,L,X) {                  \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\t"); \
                printf("%s", #L ": "); dbg_print(L); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

#define LOOP5_ASSERT(I,J,K,L,M,X) {                \
    if (!(X)) { printf("%s", #I ": "); dbg_print(I); printf("\t"); \
                printf("%s", #J ": "); dbg_print(J); printf("\t"); \
                printf("%s", #K ": "); dbg_print(K); printf("\t"); \
                printf("%s", #L ": "); dbg_print(L); printf("\t"); \
                printf("%s", #M ": "); dbg_print(M); printf("\n"); \
                fflush(stdout); aSsErT(1, #X, __LINE__); } }

//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define Q(X) printf("<| " #X " |>\n");     // Quote identifier literally.
#define P(X) dbg_print(#X " = ", X, "\n")  // Print identifier and value.
#define P_(X) dbg_print(#X " = ", X, ", ") // P(X) without '\n'
#define L_ __LINE__                        // current Line number
#define T_ putchar('\t');                  // Print a tab (w/o newline)

//=============================================================================
//                      GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

// Fundamental-type-specific print functions.
inline void dbg_print(char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(unsigned char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(signed char c) { printf("%c", c); fflush(stdout); }
inline void dbg_print(short val) { printf("%hd", val); fflush(stdout); }
inline void dbg_print(unsigned short val) {printf("%hu", val); fflush(stdout);}
inline void dbg_print(int val) { printf("%d", val); fflush(stdout); }
inline void dbg_print(unsigned int val) { printf("%u", val); fflush(stdout); }
inline void dbg_print(long val) { printf("%lu", val); fflush(stdout); }
inline void dbg_print(unsigned long val) { printf("%lu", val); fflush(stdout);}
// inline void dbg_print(Int64 val) { printf("%lld", val); fflush(stdout); }
// inline void dbg_print(Uint64 val) { printf("%llu", val); fflush(stdout); }
inline void dbg_print(float val) { printf("'%f'", val); fflush(stdout); }
inline void dbg_print(double val) { printf("'%f'", val); fflush(stdout); }
inline void dbg_print(const char* s) { printf("\"%s\"", s); fflush(stdout); }

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

AllocCounters globalCounters;

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

template <typename Tp>
class SimpleAllocator
{
    AllocCounters *m_counters;

  public:
    typedef Tp              value_type;

    SimpleAllocator(AllocCounters* c = &globalCounters) : m_counters(c) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : m_counters(other.counters()) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(m_counters->allocate(n*sizeof(Tp))); }

    void deallocate(Tp* p, std::size_t n)
        { m_counters->deallocate(p, n*sizeof(Tp)); }

    AllocCounters* counters() const { return m_counters; }
};

template <typename Tp1, typename Tp2>
bool operator==(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return a.counters() == b.counters();
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
    veryVerbose = argc > 3;
    veryVeryVerbose = argc > 4;

    int firstTest = 1;
    int lastTest = 99;
    if (test)
        firstTest = lastTest = test;

  for (test = firstTest; test <= lastTest; ++test) {

    printf("TEST " __FILE__ " CASE %d\n", test);

    switch (test) {
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

        if (verbose) printf("\nBREATHING TEST"
                            "\n==============\n");

        XSTD::function<int(const char*)> f([](const char* s) {
                return std::atoi(s); });
        int x = f("5");
        ASSERT(5 == x);

      } break;

      case 2: {
        // --------------------------------------------------------------------
        // TEST CONSTRUCTION WITH ALLOCATOR
        // --------------------------------------------------------------------
          
        if (verbose) printf("\nBREATHING TEST"
                            "\n==============\n");

        TestResource theDefaultResource;
        XSTD::polyalloc::allocator_resource::set_default_resource(
            &theDefaultResource);

        auto lambda = [](const char* s) { return std::atoi(s); };

        {
            XSTD::function<int(const char*)> f(lambda);
            ASSERT(&theDefaultResource == f.get_allocator_resource());
            ASSERT(1 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(5 == f("5"));
        }
        ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
        theDefaultResource.clear();

        {
            TestResource testRsrc;
            XSTD::polyalloc::polymorphic_allocator<char> testAlloc(&testRsrc);
            XSTD::function<int(const char*)> f(allocator_arg, testAlloc,
                                               lambda);
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(6 == f("6"));
        }

        {
            TestResource testRsrc;
            XSTD::function<int(const char*)> f(allocator_arg, &testRsrc,
                                               lambda);
            ASSERT(&testRsrc == f.get_allocator_resource());
            ASSERT(1 == testRsrc.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(7 == f("7"));
        }

        {
            TestResource testRsrc1, testRsrc2;
            XSTD::function<int(const char*)> f1(allocator_arg, &testRsrc1,
                                                lambda);
            ASSERT(&testRsrc1 == f1.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(0 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(8 == f1("8"));

            XSTD::polyalloc::polymorphic_allocator<char> testAlloc2(&testRsrc2);
            XSTD::function<int(const char*)> f2(allocator_arg, &testRsrc2, f1);
            ASSERT(&testRsrc2 == f2.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(9 == f2("9"));

            XSTD::function<int(const char*)> f3(f1);
            ASSERT(XSTD::polyalloc::allocator_resource::default_resource() ==
                   f3.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(1 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(10 == f3("10"));
        }
        ASSERT(0 == theDefaultResource.counters().blocks_outstanding());

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
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(8 == f1("8"));

            XSTD::polyalloc::polymorphic_allocator<char> testAlloc2(&testRsrc2);
            XSTD::function<int(const char*)> f2(allocator_arg, &testRsrc2, f1);
            ASSERT(&testRsrc2 == f2.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(9 == f2("9"));

            XSTD::function<int(const char*)> f3(f1);
            ASSERT(XSTD::polyalloc::allocator_resource::default_resource() ==
                   f3.get_allocator_resource());
            ASSERT(1 == testRsrc1.counters().blocks_outstanding());
            ASSERT(1 == testRsrc2.counters().blocks_outstanding());
            ASSERT(0 == theDefaultResource.counters().blocks_outstanding());
            ASSERT(10 == f3("10"));
        }
        ASSERT(0 == theDefaultResource.counters().blocks_outstanding());

      } break;

      default: {
        if (lastTest < 99)
        {
          fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
          testStatus = -1;
        }
        test = lastTest;
      }
    }
  } // End for

    if (testStatus > 0) {
        fprintf(stderr, "Error, non-zero test status = %d.\n", testStatus);
    }

    return testStatus;
}

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2012
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------

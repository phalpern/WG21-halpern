/* polymorphic_allocator.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <polymorphic_allocator.h>
#include <../allocator_traits/xstd_list.h>

#include <iostream>
#include <cstdlib>
#include <climits>
#include <cstring>

//==========================================================================
//                  ASSERT TEST MACRO
//--------------------------------------------------------------------------
static int testStatus = 0;

static void aSsErT(int c, const char *s, int i) {
    if (c) {
        std::cout << __FILE__ << ":" << i << ": error: " << s
                  << "    (failed)" << std::endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

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
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------
static inline int min(int a, int b) { return a < b ? a : b; }
    // Return the minimum of the specified 'a' and 'b' arguments.

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

template class XSTD::polyalloc::polymorphic_allocator<double>;
template class XSTD::list<double,
                          XSTD::polyalloc::polymorphic_allocator<double> >;
template class SimpleAllocator<double>;
template class XSTD::list<double, SimpleAllocator<double> >;

struct UniqDummyType { void zzzzz(UniqDummyType, bool) { } };
typedef void (UniqDummyType::*UniqPointerType)(UniqDummyType);

typedef void (UniqDummyType::*ConvertibleToBoolType)(UniqDummyType, bool);
const ConvertibleToBoolType ConvertibleToTrue = &UniqDummyType::zzzzz;

template <typename _Tp> struct unvoid { typedef _Tp type; };
template <> struct unvoid<void> { struct type { }; };
template <> struct unvoid<const void> { struct type { }; };

template <typename Alloc>
class SimpleString
{
    typedef XSTD::allocator_traits<Alloc> AllocTraits;

    Alloc alloc_;
    typename AllocTraits::pointer data_;

public:
    typedef Alloc allocator_type;
    
    SimpleString(const Alloc& a = Alloc()) : alloc_(a), data_(nullptr) { }
    SimpleString(const char* s, const Alloc& a = Alloc())
        : alloc_(a), data_(nullptr) { assign(s); }

    SimpleString(const SimpleString& other)
        : alloc_(AllocTraits::select_on_container_copy_construction(
                     other.alloc_))
        , data_(nullptr) { assign(other.data_); }

    SimpleString(SimpleString&& other)
        : alloc_(std::move(other.alloc_))
        , data_(other.data_) { other.data_ = nullptr; }

    SimpleString(const SimpleString& other, const Alloc& a)
        : alloc_(a), data_(nullptr) { assign(other.data_); }

    SimpleString(SimpleString&& other, const Alloc& a)
        : alloc_(a), data_(nullptr)
        { *this = std::move(other); }

    SimpleString& operator=(const SimpleString& other) {
        if (this == &other)
            return *this;
        clear();
        if (AllocTraits::propagate_on_container_copy_assignment::value)
            alloc_ = other.alloc_;
        assign(other.data_);
        return *this;
    }

    SimpleString& operator=(SimpleString&& other) {
        if (this == &other)
            return *this;
        else if (alloc_ == other.alloc_)
            std::swap(data_, other.data_);
        else if (AllocTraits::propagate_on_container_move_assignment::value)
        {
            clear();
            alloc_ = std::move(other.alloc_);
            data_ = other.data_;
            other.data_ = nullptr;
        }
        else
            assign(other.c_str());
        return *this;
    }

    ~SimpleString() { clear(); }

    void clear() {
        if (data_)
            AllocTraits::deallocate(alloc_, data_, std::strlen(&*data_) + 1);
    }        

    void assign(const char* s) {
        clear();
        data_ = AllocTraits::allocate(alloc_, std::strlen(s) + 1);
        std::strcpy(&*data_, s);
    }

    const char* c_str() const { return &(*data_); }

    Alloc get_allocator() const { return alloc_; }
};

template <typename Alloc>
inline
bool operator==(const SimpleString<Alloc>& a, const SimpleString<Alloc>& b)
{
    return 0 == std::strcmp(a.c_str(), b.c_str());
}

template <typename Alloc>
inline
bool operator!=(const SimpleString<Alloc>& a, const SimpleString<Alloc>& b)
{
    return ! (a == b);
}

template <typename Alloc>
inline
bool operator==(const SimpleString<Alloc>& a, const char *b)
{
    return 0 == std::strcmp(a.c_str(), b);
}

template <typename Alloc>
inline
bool operator!=(const SimpleString<Alloc>& a, const char *b)
{
    return ! (a == b);
}

template <typename Alloc>
inline
bool operator==(const char *a, const SimpleString<Alloc>& b)
{
    return 0 == std::strcmp(a, b.c_str());
}

template <typename Alloc>
inline
bool operator!=(const char *a, const SimpleString<Alloc>& b)
{
    return ! (a == b);
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    using namespace XSTD::polyalloc;

    int test = argc > 1 ? atoi(argv[1]) : 0;
//     int verbose = argc > 2;
//     int veryVerbose = argc > 3;
//     int veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

#define POLYALLOC XSTD::polyalloc::polymorphic_allocator

    allocator_resource::set_default_resource(&dfltTestRsrc);

    switch (test) { case 0: // Do all cases for test-case 0
      case 1:
      {
        // --------------------------------------------------------------------
        // BREATHING TEST
        // --------------------------------------------------------------------

        std::cout << "\nBREATHING TEST"
                  << "\n==============" << std::endl;

        {
            // Test conversion constructor
            TestResource ar;
            const POLYALLOC<double> a1(&ar);
            POLYALLOC<char> a2(a1);
            ASSERT(a2.resource() == &ar);
        }

        TestResource x, y, z;
        AllocCounters &xc = const_cast<AllocCounters&>(x.counters()),
                      &yc = const_cast<AllocCounters&>(y.counters());

        // Simple use of list with polymorphic allocator
        {
            typedef POLYALLOC<int> Alloc;

            XSTD::list<int, Alloc> lx(&x);

            lx.push_back(3);
            ASSERT(1 == lx.size());
            ASSERT(2 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Outer allocator is polymorphic, inner is not.
        {
            typedef SimpleString<SimpleAllocator<char> > String;
            typedef POLYALLOC<String> Alloc;

            XSTD::list<String, Alloc> lx(&x);
            ASSERT(1 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT("hello" == lx.back());
            ASSERT(2 == x.counters().blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            lx.push_back("goodbye");
            ASSERT(2 == lx.size());
            ASSERT("hello" == lx.front());
            ASSERT("goodbye" == lx.back());
            ASSERT(3 == x.counters().blocks_outstanding());
            ASSERT(2 == globalCounters.blocks_outstanding());
            ASSERT(SimpleAllocator<char>() == lx.front().get_allocator());
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == y.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Inner allocator is polymorphic, outer is not.
        {
            typedef SimpleString<POLYALLOC<char> > String;
            typedef SimpleAllocator<String> Alloc;

            XSTD::list<String, Alloc> lx(&xc);
            ASSERT(1 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT("hello" == lx.back());
            ASSERT(2 == x.counters().blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            lx.push_back("goodbye");
            ASSERT(2 == lx.size());
            ASSERT("hello" == lx.front());
            ASSERT("goodbye" == lx.back());
            ASSERT(3 == x.counters().blocks_outstanding());
            ASSERT(2 == globalCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc == lx.front().get_allocator().resource());
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Both outer and inner allocators are polymorphic.
        {
            typedef SimpleString<POLYALLOC<char> > String;
            typedef POLYALLOC<String> Alloc;

            XSTD::list<String, Alloc> lx(&x);
            ASSERT(1 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT("hello" == lx.back());
            ASSERT(3 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            lx.push_back("goodbye");
            ASSERT(2 == lx.size());
            ASSERT("hello" == lx.front());
            ASSERT("goodbye" == lx.back());
            ASSERT(5 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            ASSERT(&x == lx.front().get_allocator().resource());
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Test container copy construction
        {
            typedef SimpleString<POLYALLOC<char> > String;
            typedef POLYALLOC<String> Alloc;

            XSTD::list<String, Alloc> lx(&x);

            lx.push_back("hello");
            lx.push_back("goodbye");
            ASSERT(2 == lx.size());
            ASSERT("hello" == lx.front());
            ASSERT("goodbye" == lx.back());
            ASSERT(5 == x.counters().blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            ASSERT(&x == lx.front().get_allocator().resource());

            XSTD::list<String, Alloc> lg(lx);
            ASSERT(2 == lg.size());
            ASSERT("hello" == lg.front());
            ASSERT("goodbye" == lg.back());
            ASSERT(5 == x.counters().blocks_outstanding());
            ASSERT(5 == globalCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc == lg.front().get_allocator().resource());

            XSTD::list<String, Alloc> ly(lx, &y);
            ASSERT(2 == lg.size());
            ASSERT("hello" == ly.front());
            ASSERT("goodbye" == ly.back());
            ASSERT(5 == x.counters().blocks_outstanding());
            ASSERT(5 == y.counters().blocks_outstanding());
            ASSERT(5 == globalCounters.blocks_outstanding());
            ASSERT(&y == ly.front().get_allocator().resource());
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == y.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Test resource_adaptor
        {
            typedef SimpleString<polymorphic_allocator<char> > String;
            typedef XSTD::list<String, polymorphic_allocator<String> > strlist;
            typedef XSTD::list<strlist, polymorphic_allocator<strlist> >
                strlist2;

            SimpleAllocator<char> sax(&xc);
            SimpleAllocator<char> say(&yc);
            POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>) crx(sax);
            POLYALLOC_RESOURCE_ADAPTOR(SimpleAllocator<char>) cry(say);

            strlist a(&crx);
            strlist2 b(&cry);

            ASSERT(0 == a.size());
            ASSERT(0 == std::distance(a.begin(), a.end()));
            a.push_back("hello");
            ASSERT(1 == a.size());
            ASSERT("hello" == a.front());
            ASSERT(1 == std::distance(a.begin(), a.end()));
            a.push_back("goodbye");
            ASSERT(2 == a.size());
            ASSERT("hello" == a.front());
            ASSERT("goodbye" == a.back());
            ASSERT(2 == std::distance(a.begin(), a.end()));
            ASSERT(5 == x.counters().blocks_outstanding());
            b.push_back(a);
            ASSERT(1 == b.size());
            ASSERT(2 == b.front().size());
            ASSERT("hello" == b.front().front());
            ASSERT("goodbye" == b.front().back());
            ASSERT(5 == x.counters().blocks_outstanding());
            LOOP_ASSERT(y.counters().blocks_outstanding(),
                        7 == y.counters().blocks_outstanding());

            b.emplace_front(3, "repeat");
            ASSERT(2 == b.size());
            ASSERT(3 == b.front().size());
            ASSERT(5 == x.counters().blocks_outstanding());
            LOOP_ASSERT(y.counters().blocks_outstanding(),
                        15 == y.counters().blocks_outstanding());

            static const char* const exp[] = {
                "repeat", "repeat", "repeat", "hello", "goodbye"
            };

            int e = 0;
            for (strlist2::iterator i = b.begin(); i != b.end(); ++i) {
                ASSERT(i->get_allocator().resource() == &cry);
                for (strlist::iterator j = i->begin(); j != i->end(); ++j) {
                    ASSERT(j->get_allocator().resource() == &cry);
                    ASSERT(*j == exp[e++]);
                }
            }
        }
        LOOP_ASSERT(x.counters().blocks_outstanding(),
                    0 == x.counters().blocks_outstanding());
        LOOP_ASSERT(y.counters().blocks_outstanding(),
                    0 == y.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        // Test construct() using pairs
        {
            typedef SimpleString<POLYALLOC<char> > String;
            typedef POLYALLOC<String> Alloc;
            typedef std::pair<String, int> StrInt;

            XSTD::list<StrInt, SimpleAllocator<StrInt> > lx(&xc);
            XSTD::list<StrInt, Alloc> ly(&y);
            
            lx.push_back(StrInt("hello", 5));
            ASSERT(1 == lx.size());
            ASSERT(2 == x.counters().blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc == lx.front().first.get_allocator().resource());
            ASSERT("hello" == lx.front().first);
            ASSERT(5 == lx.front().second);
            ly.push_back(StrInt("goodbye", 6));
            ASSERT(1 == ly.size());
            ASSERT(3 == y.counters().blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(&y == ly.front().first.get_allocator().resource());
            ASSERT("goodbye" == ly.front().first);
            ASSERT(6 == ly.front().second);
            ly.emplace_front("howdy", 9);
            ASSERT(2 == ly.size());
            ASSERT(5 == y.counters().blocks_outstanding());
            ASSERT(&y == ly.front().first.get_allocator().resource());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT("howdy" == ly.front().first);
            ASSERT(9 == ly.front().second);
            ASSERT("goodbye" == ly.back().first);
            ASSERT(6 == ly.back().second);
        }
        ASSERT(0 == x.counters().blocks_outstanding());
        ASSERT(0 == y.counters().blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

      } if (test != 0) break;

      break;

      default: {
        std::cerr << "WARNING: CASE `" << test << "' NOT FOUND." << std::endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        std::cerr << "Error, non-zero test status = " << testStatus << "."
                  << std::endl;
    }

    return testStatus;
}

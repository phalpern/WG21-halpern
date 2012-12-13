/* scoped_allocator.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <polymorphic_allocator.h>
#include <xstd_list.h>

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
    int num_allocs_;
    int num_deallocs_;
    int bytes_allocated_;
    int bytes_deallocated_;

    union Header {
        void*       align_;
        std::size_t size_;
    };

  public:
    AllocCounters()
	: num_allocs_(0)
	, num_deallocs_(0)
	, bytes_allocated_(0)
	, bytes_deallocated_(0)
	{ }

    void* allocate(std::size_t nbytes) {
	ASSERT(this != nullptr);
        std::size_t blocksize =
            (nbytes + 2*sizeof(Header) - 1) & ~(sizeof(Header)-1);
        Header* ret = static_cast<Header*>(operator new(blocksize));
        ret->size_ = nbytes;
        ++ret;
        ++num_allocs_;
        bytes_allocated_ += nbytes;
        return ret;
    }

    void deallocate(void* p, std::size_t nbytes) {
        Header* h = static_cast<Header*>(p) - 1;
        ASSERT(nbytes == h->size_);
	h->size_ = 0xdeadbeaf;
        operator delete((void*)h);
        ++num_deallocs_;
        bytes_deallocated_ += nbytes;
    }

    int blocks_outstanding() const { return num_allocs_ - num_deallocs_; }
    int bytes_outstanding() const
        { return bytes_allocated_ - bytes_deallocated_; }

    void dump(std::ostream& os, const char* msg) {
	os << msg << ":\n";
        os << "  num allocs = " << num_allocs_ << '\n';
        os << "  num deallocs = " << num_deallocs_ << '\n';
        os << "  outstanding allocs = " << blocks_outstanding() << '\n';
        os << "  bytes allocated = " << bytes_allocated_ << '\n';
        os << "  bytes deallocated = " << bytes_deallocated_ << '\n';
        os << "  outstanding bytes = " << bytes_outstanding() << '\n';
        os << std::endl;
    }
};

AllocCounters globalCounters;

template <typename Tp>
class SimpleAllocator
{
    AllocCounters *counters_;

  public:
    typedef Tp              value_type;

    SimpleAllocator(AllocCounters* c = &globalCounters) : counters_(c) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : counters_(other.counters()) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(counters_->allocate(n*sizeof(Tp))); }

    void deallocate(Tp* p, std::size_t n)
        { counters_->deallocate(p, n*sizeof(Tp)); }

    AllocCounters* counters() const { return counters_; }
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

template class SimpleAllocator<double>;
template class XSTD::list<double, SimpleAllocator<double> >;
template class XSTD::scoped_allocator_adaptor<SimpleAllocator<double> >;

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

    const XSTD::scoped_allocator_adaptor<SimpleAllocator<double>, SimpleAllocator<int> > a1;
    XSTD::scoped_allocator_adaptor<SimpleAllocator<char>, SimpleAllocator<int> > a2(a1);

    {
        const int myi = 3;
        const void *myvp = &myi;
        const int *myip = static_cast<const int*>(myvp);
        ASSERT(myip == &myi);
    }

    switch (test) { case 0: // Do all cases for test-case 0
      case 1:
      {
        // --------------------------------------------------------------------
        // TEST using SimpleAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC SimpleAllocator

        std::cout << "\nSimpleAllocator"
                  << "\n===============" << std::endl;

        AllocCounters x, y, z;

        {
            typedef XSTD::scoped_allocator_adaptor<TESTALLOC<int> > Saa;

            XSTD::list<int, TESTALLOC<int> > lx(&x);
            TESTALLOC<int> yr(&y);
            Saa ya(yr);
            XSTD::list<int, Saa> ly(ya);

            lx.push_back(3);
            ASSERT(1 == lx.size());
            ASSERT(2 == x.blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
            ly.push_back(4);
            ASSERT(1 == ly.size());
            ASSERT(2 == y.blocks_outstanding());
            ASSERT(0 == globalCounters.blocks_outstanding());
        }
        ASSERT(0 == x.blocks_outstanding());
        ASSERT(0 == y.blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        {
            typedef SimpleString<TESTALLOC<char> > String;
            typedef XSTD::scoped_allocator_adaptor<TESTALLOC<String> > Saa;

            XSTD::list<String, TESTALLOC<String> > lx(&x);
            XSTD::list<String, Saa> ly(&y);
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT(2 == x.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ly.push_back("goodbye");
            ASSERT(1 == ly.size());
            ASSERT(3 == y.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(TESTALLOC<char>(&y) == ly.front().get_allocator());
        }
        ASSERT(0 == x.blocks_outstanding());
        ASSERT(0 == y.blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        {
            typedef SimpleString<TESTALLOC<char> > String;
            typedef XSTD::scoped_allocator_adaptor<TESTALLOC<String>,
                                                         TESTALLOC<int> > Saa;

            XSTD::list<String, TESTALLOC<String> > lx(&x);
            Saa sa(&y, &z);
            XSTD::list<String, Saa> ly(sa);
            ASSERT(1 == x.blocks_outstanding());
            ASSERT(1 == y.blocks_outstanding());
            ASSERT(0 == z.blocks_outstanding());
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT(2 == x.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ly.push_back("goodbye");
            ASSERT(1 == ly.size());
            ASSERT(2 == y.blocks_outstanding());
            ASSERT(1 == z.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(TESTALLOC<char>(&z) == ly.front().get_allocator());
        }
        ASSERT(0 == x.blocks_outstanding());
        ASSERT(0 == y.blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        {
            typedef SimpleString<TESTALLOC<char> > String;
            typedef XSTD::scoped_allocator_adaptor<TESTALLOC<String> > Saas;
            typedef XSTD::scoped_allocator_adaptor<Saas,
                                                        TESTALLOC<int> > Saa;

            XSTD::list<String, TESTALLOC<String> > lx(&x);
            XSTD::list<String, Saa> ly(Saa(&y, &z));
            
            lx.push_back("hello");
            ASSERT(1 == lx.size());
            ASSERT(2 == x.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ly.push_back("goodbye");
            ASSERT(1 == ly.size());
            ASSERT(2 == y.blocks_outstanding());
            ASSERT(1 == z.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(TESTALLOC<char>(&z) == ly.front().get_allocator());
        }
        ASSERT(0 == x.blocks_outstanding());
        ASSERT(0 == y.blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        {
            typedef SimpleString<polymorphic_allocator<char> > String;
            typedef XSTD::list<String, polymorphic_allocator<String> > strlist;
            typedef XSTD::list<strlist, polymorphic_allocator<strlist> >
                strlist2;

            resource_adaptor_mf<SimpleAllocator<char> >::type crx(&x);
            resource_adaptor_mf<SimpleAllocator<char> >::type cry(&y);

            strlist a(&crx);
            strlist2 b(&cry);

            ASSERT(0 == a.size());
            ASSERT(0 == std::distance(a.begin(), a.end()));
            a.push_back("hello");
            ASSERT(1 == a.size());
            ASSERT(1 == std::distance(a.begin(), a.end()));
            a.push_back("goodbye");
            ASSERT(2 == a.size());
            ASSERT(2 == std::distance(a.begin(), a.end()));
            ASSERT(5 == x.blocks_outstanding());
            b.push_back(a);
            ASSERT(1 == b.size());
            ASSERT(2 == b.front().size());
            ASSERT(5 == x.blocks_outstanding());
            LOOP_ASSERT(y.blocks_outstanding(), 7 == y.blocks_outstanding());

            b.emplace_front(3, "repeat");
            ASSERT(2 == b.size());
            ASSERT(3 == b.front().size());
            ASSERT(5 == x.blocks_outstanding());
            LOOP_ASSERT(y.blocks_outstanding(), 15 == y.blocks_outstanding());

            static const char* const exp[] = {
                "repeat", "repeat", "repeat", "hello", "goodbye"
            };

            int e = 0;
            for (strlist2::iterator i = b.begin(); i != b.end(); ++i) {
                ASSERT(i->get_allocator().resource() == &cry);
                for (strlist::iterator j = i->begin(); j != i->end(); ++j) {
                    ASSERT(j->get_allocator().resource() == &cry);
                    ASSERT(0 == std::strcmp(j->c_str(), exp[e++]));
                }
            }
        }
        LOOP_ASSERT(x.blocks_outstanding(), 0 == x.blocks_outstanding());
        LOOP_ASSERT(y.blocks_outstanding(), 0 == y.blocks_outstanding());
        ASSERT(0 == globalCounters.blocks_outstanding());

        {
            typedef SimpleString<TESTALLOC<char> > String;
            typedef std::pair<String, int> StrInt;

            typedef XSTD::scoped_allocator_adaptor<TESTALLOC<StrInt> > Saa;
            XSTD::list<StrInt, TESTALLOC<StrInt> > lx(&x);
            XSTD::list<StrInt, Saa> ly(&y);
            
            lx.push_back(StrInt("hello", 5));
            ASSERT(1 == lx.size());
            ASSERT(2 == x.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(0 == std::strcmp("hello", lx.front().first.c_str()));
            ASSERT(5 == lx.front().second);
            ly.push_back(StrInt("goodbye", 6));
            ASSERT(1 == ly.size());
            ASSERT(3 == y.blocks_outstanding());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(TESTALLOC<char>(&y) == ly.front().first.get_allocator());
            ASSERT(0 == std::strcmp("goodbye", ly.front().first.c_str()));
            ASSERT(6 == ly.front().second);
            ly.emplace_front("howdy", 9);
            ASSERT(2 == ly.size());
            ASSERT(5 == y.blocks_outstanding());
            ASSERT(TESTALLOC<char>(&y) == ly.front().first.get_allocator());
            ASSERT(1 == globalCounters.blocks_outstanding());
            ASSERT(0 == std::strcmp("howdy", ly.front().first.c_str()));
            ASSERT(9 == ly.front().second);
            ASSERT(0 == std::strcmp("goodbye", ly.back().first.c_str()));
        }
        ASSERT(0 == x.blocks_outstanding());
        ASSERT(0 == y.blocks_outstanding());
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

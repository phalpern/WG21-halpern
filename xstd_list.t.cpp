/* xstd_list.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <xstd_list.h>

#include <iostream>
#include <cstdlib>
#include <climits>

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
#define P(X) std::cout << #X " = " << (X) << std::endl; // Print ID and value.
#define Q(X) std::cout << "<| " #X " |>" << std::endl;  // Quote ID literally.
#define P_(X) std::cout << #X " = " << (X) << ", " << std::flush; // P(X) no nl
#define L_ __LINE__                                // current Line number
#define T_ std::cout << "\t" << std::flush;        // Print a tab (w/o newline)

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

enum { VERBOSE_ARG_NUM = 2, VERY_VERBOSE_ARG_NUM, VERY_VERY_VERBOSE_ARG_NUM };

static int verbose = 0;
static int veryVerbose = 0;
static int veryVeryVerbose = 0;

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

//=============================================================================
//                  CLASSES FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

class AllocResource
{
    int num_allocs_;
    int num_deallocs_;
    int bytes_allocated_;
    int bytes_deallocated_;

    union Header {
        void* align_;
        int   size_;
    };

  public:
    AllocResource()
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

AllocResource defaultResource;

template <typename Tp>
class SimpleAllocator
{
    AllocResource *resource_;

  public:
    typedef Tp              value_type;

    SimpleAllocator(AllocResource* ar = nullptr) : resource_(ar) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : resource_(other.resource()) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(resource_->allocate(n*sizeof(Tp))); }

    void deallocate(Tp* p, std::size_t n)
        { resource_->deallocate(p, n*sizeof(Tp)); }

    AllocResource* resource() const { return resource_; }
};

template <typename Tp1, typename Tp2>
bool operator==(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return a.resource() == b.resource();
}

template <typename Tp1, typename Tp2>
bool operator!=(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return ! (a == b);
}

template class SimpleAllocator<double>;
template class XSTD::allocator_traits<SimpleAllocator<double> >;
template class XSTD::list<double, SimpleAllocator<double> >;

struct UniqDummyType { void zzzzz(UniqDummyType, bool) { } };
typedef void (UniqDummyType::*UniqPointerType)(UniqDummyType);

typedef void (UniqDummyType::*ConvertibleToBoolType)(UniqDummyType, bool);
const ConvertibleToBoolType ConvertibleToTrue = &UniqDummyType::zzzzz;

template <typename Tp>
class FancyPointer
{
    template <typename T> friend class FancyAllocator;

    Tp* value_;
public:
    typedef Tp value_type;

    FancyPointer(UniqPointerType p = nullptr)
	: value_(0) { ASSERT(p == nullptr); }
    template <typename T> FancyPointer(const FancyPointer<T>& p)
	{ value_ = p.ptr(); }
    explicit FancyPointer(const FancyPointer<void>& p)
	: value_(static_cast<Tp*>(p.ptr())) { }
    explicit FancyPointer(const FancyPointer<const void>& p)
	: value_(static_cast<const Tp*>(p.ptr())) { }

    typename std::add_lvalue_reference<Tp>::type
      operator*() const { return *value_; }
    Tp* operator->() const { return value_; }
    Tp* ptr() const { return value_; }

    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }
};

template <typename Tp1, typename Tp2>
bool operator==(FancyPointer<Tp1> a, FancyPointer<Tp2> b)
{
    return a.ptr() == b.ptr();
}

template <typename Tp1, typename Tp2>
bool operator!=(FancyPointer<Tp1> a, FancyPointer<Tp2> b)
{
    return ! (a == b);
}

template <typename Tp>
class FancyAllocator
{
    AllocResource *resource_;

  public:
    typedef Tp              value_type;

    typedef FancyPointer<Tp>         pointer;
    typedef FancyPointer<const Tp>   const_pointer;

    typedef FancyPointer<void>       void_pointer;
    typedef FancyPointer<const void> const_void_pointer;

    typedef std::ptrdiff_t  difference_type;
    typedef std::size_t     size_type;

    template <typename T>
    struct rebind
    {
	typedef FancyAllocator<T> other;
    };

    FancyAllocator(AllocResource* ar = nullptr) : resource_(ar) { }

    // Required constructor
    template <typename T>
    FancyAllocator(const FancyAllocator<T>& other)
        : resource_(other.resource()) { }

    pointer allocate(size_type n) {
	pointer ret;
	ret.value_ = static_cast<Tp*>(resource_->allocate(n*sizeof(Tp)));
	return ret;
    }
    pointer allocate(size_type n, const_void_pointer hint)
        { return allocate(n); }

    void deallocate(pointer p, size_type n)
        { resource_->deallocate(p.ptr(), n*sizeof(Tp)); }

    template <typename T, typename... Args>
      void construct(T* p, Args&&... args)
        { new (static_cast<void*>(p)) T(std::forward<Args>(args)...); }

    template <typename T>
      void destroy(T* p)
        { p->~T(); }

    size_type max_size() const
        { return INT_MAX; }

    pointer address(value_type& r) const {
	pointer ret;
	ret.value_ = XSTD::addressof(r);
	return ret;
    }
    const_pointer address(const value_type& r) const {
	const_pointer ret;
	ret.value_ = XSTD::addressof(r);
	return ret;
    }

    // FancyAllocator propagation on construction
    FancyAllocator select_on_container_copy_construction() const
        { return *this; }

    // FancyAllocator propagation functions.  Return true if *this was
    // modified.
    bool on_container_copy_assignment(const FancyAllocator& rhs)
        { return false; }
    bool on_container_move_assignment(FancyAllocator&& rhs)
        { return false; }
    bool on_container_swap(FancyAllocator& other)
        { return false; }

    AllocResource* resource() const { return resource_; }
};

template <typename T1, typename T2>
bool operator==(const FancyAllocator<T1>& a, const FancyAllocator<T2>& b)
{
    return a.resource() == b.resource();
}

template <typename T1, typename T2>
bool operator!=(const FancyAllocator<T1>& a, const FancyAllocator<T2>& b)
{
    return ! (a == b);
}

template class FancyAllocator<double>;
template class XSTD::allocator_traits<FancyAllocator<double> >;
template class XSTD::list<double, FancyAllocator<double> >;


// An allocator that propagates on, move assignment, copy assignment, and
// swap, but NOT on copy construction.  It's propagation semantics are the
// reverse of the default and would probably not be very useful except for
// testing.
template <typename Tp>
class WeirdAllocator
{
    AllocResource *resource_;

  public:
    typedef Tp              value_type;

    WeirdAllocator(AllocResource* ar = &defaultResource) : resource_(ar) { }

    // Required constructor
    template <typename T>
    WeirdAllocator(const WeirdAllocator<T>& other)
        : resource_(other.resource()) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(resource_->allocate(n*sizeof(Tp))); }

    void deallocate(Tp* p, std::size_t n)
        { resource_->deallocate(p, n*sizeof(Tp)); }

    // WeirdAllocator does not propagate on construction
    WeirdAllocator select_on_container_copy_construction() const
        { return WeirdAllocator(); }

    // WeirdAllocator propagation functions.  Return true if *this was
    // modified.
    bool on_container_copy_assignment(const WeirdAllocator& rhs)
        { resource_ = rhs.resource_; return true; }
    bool on_container_move_assignment(WeirdAllocator&& rhs) {
        resource_ = rhs.resource_;
        rhs.resource_ = &defaultResource;
        return true;
    }
    bool on_container_swap(WeirdAllocator& other)
        { std::swap(resource_, other.resource_); return true; }

    AllocResource* resource() const { return resource_; }
};

template <typename Tp1, typename Tp2>
bool operator==(const WeirdAllocator<Tp1>& a, const WeirdAllocator<Tp2>& b)
{
    return a.resource() == b.resource();
}

template <typename Tp1, typename Tp2>
bool operator!=(const WeirdAllocator<Tp1>& a, const WeirdAllocator<Tp2>& b)
{
    return ! (a == b);
}

//=============================================================================
//                              TEST FUNCTION
//-----------------------------------------------------------------------------

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test = argc > 1 ? std::atoi(argv[1]) : 0;
    verbose = argc > 2;
    veryVerbose = argc > 3;
    veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__ << " CASE " << test << std::endl;

    switch (test) { case 0:  // Zero runs all tests
      case 1:
      {
        // --------------------------------------------------------------------
        // TEST list operations using SimpleAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC SimpleAllocator

        std::cout << "\nSimpleAllocator"
                  << "\n===============" << std::endl;

        AllocResource ar;

        {
            TESTALLOC<int> a(&ar);
            typedef XSTD::allocator_traits<TESTALLOC<int> >::pointer
                PtrType;
            ASSERT(0 == ar.blocks_outstanding());
            ASSERT(0 == ar.bytes_outstanding());

            PtrType x = a.allocate(1);
            ASSERT(1 == ar.blocks_outstanding());
            ASSERT(sizeof(int) == ar.bytes_outstanding());

            PtrType y = a.allocate(3);
            ASSERT(2 == ar.blocks_outstanding());
            ASSERT(4*sizeof(int) == ar.bytes_outstanding());

            a.deallocate(x, 1);
            ASSERT(1 == ar.blocks_outstanding());
            ASSERT(3*sizeof(int) == ar.bytes_outstanding());

            a.deallocate(y, 3);
            ASSERT(0 == ar.blocks_outstanding());
            ASSERT(0 == ar.bytes_outstanding());

            if (verbose) {
                ar.dump(std::cout, "After dealloc all");
            }
        }

        {
            TESTALLOC<int> a(&ar);

            ASSERT(0 == ar.bytes_outstanding());
            ASSERT(0 == ar.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > lst(a);
            ASSERT(0 == lst.size());
            ASSERT(lst.empty());
            ASSERT(1 == ar.blocks_outstanding());

            lst.push_back(4);
            ASSERT(1 == lst.size());
            ASSERT(! lst.empty());
            ASSERT(4 == lst.front());
            ASSERT(4 == lst.back());
            ASSERT(2 == ar.blocks_outstanding());

            if (verbose) {
                ar.dump(std::cout, "After push_back(4)");
            }

        }
        ASSERT(0 == ar.blocks_outstanding());
        ASSERT(0 == ar.bytes_outstanding());

      } if (test != 0) break;

      case 2:
      {
        // --------------------------------------------------------------------
        // TEST list operations using FancyAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC FancyAllocator

        std::cout << "\nFancyAllocator"
                  << "\n==============" << std::endl;

        AllocResource ar;

        {
            TESTALLOC<int> a(&ar);
            typedef XSTD::allocator_traits<TESTALLOC<int> >::pointer
                PtrType;
            ASSERT(0 == ar.blocks_outstanding());
            ASSERT(0 == ar.bytes_outstanding());

            PtrType x = a.allocate(1);
            ASSERT(1 == ar.blocks_outstanding());
            ASSERT(sizeof(int) == ar.bytes_outstanding());

            PtrType y = a.allocate(3);
            ASSERT(2 == ar.blocks_outstanding());
            ASSERT(4*sizeof(int) == ar.bytes_outstanding());

            a.deallocate(x, 1);
            ASSERT(1 == ar.blocks_outstanding());
            ASSERT(3*sizeof(int) == ar.bytes_outstanding());

            a.deallocate(y, 3);
            ASSERT(0 == ar.blocks_outstanding());
            ASSERT(0 == ar.bytes_outstanding());

            if (verbose) {
                ar.dump(std::cout, "After dealloc all");
            }
        }

        {
            TESTALLOC<int> a(&ar);

            ASSERT(0 == ar.bytes_outstanding());
            ASSERT(0 == ar.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > lst(a);
            ASSERT(0 == lst.size());
            ASSERT(lst.empty());
            ASSERT(1 == ar.blocks_outstanding());

            lst.push_back(4);
            ASSERT(1 == lst.size());
            ASSERT(! lst.empty());
            ASSERT(4 == lst.front());
            ASSERT(4 == lst.back());
            ASSERT(2 == ar.blocks_outstanding());

            if (verbose) {
                ar.dump(std::cout, "After push_back(4)");
            }

        }
        ASSERT(0 == ar.blocks_outstanding());
        ASSERT(0 == ar.bytes_outstanding());

      } if (test != 0) break;

      case 3:
      {
        // --------------------------------------------------------------------
        // TEST allocator propagation using SimpleAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC SimpleAllocator

        std::cout << "\nSimpleAllocator propagation"
                  << "\n===========================" << std::endl;

        AllocResource ar1, ar2;

        // Test that allocator propagates on copy construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(x);
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(4 == ar1.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }
        
        // Test that allocator propagates on move construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(std::move(x));
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(3 == ar1.blocks_outstanding());
            ASSERT(&y.front() == frontptr);
        }
        
        // Test that allocator does not propagate on copy assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = x;
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a2);
            ASSERT(2 == ar1.blocks_outstanding());
            ASSERT(2 == ar2.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }

        // Test that allocator does not propagate on move assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = std::move(x);
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a2);
            ASSERT(2 == ar1.blocks_outstanding()); // implementation-specific
            ASSERT(2 == ar2.blocks_outstanding()); // required behavior
            ASSERT(&x.front() == frontptr); // implementation-specific
            ASSERT(&y.front() != frontptr); // required behavior
        }

        // Test that swap works with equal allocators
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* xfrontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a1);
            y.push_back(99);
            y.push_front(98);
            int* yfrontptr = &y.front();
            int* ybackptr = &y.back();
            ASSERT(2 == y.size());
            ASSERT(98 == y.front());
            ASSERT(99 == y.back());
            ASSERT(y.get_allocator() == a1);

            x.swap(y);
            ASSERT(2 == x.size());
            ASSERT(98 == x.front());
            ASSERT(99 == x.back());
            ASSERT(x.get_allocator() == a1);
            ASSERT(&x.front() == yfrontptr);
            ASSERT(&x.back() == ybackptr);
            ASSERT(5 == ar1.blocks_outstanding());
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(&y.front() == xfrontptr);
        }

      } if (test != 0) break;

      case 4:
      {
        // --------------------------------------------------------------------
        // TEST allocator propagation using FancyAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC FancyAllocator

        std::cout << "\nFancyAllocator propagation"
                  << "\n==========================" << std::endl;

        AllocResource ar1, ar2;

        // Test that allocator propagates on copy construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(x);
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(4 == ar1.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }
        
        // Test that allocator propagates on move construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(std::move(x));
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(3 == ar1.blocks_outstanding());
            ASSERT(&y.front() == frontptr);
        }
        
        // Test that allocator does not propagate on copy assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = x;
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a2);
            ASSERT(2 == ar1.blocks_outstanding());
            ASSERT(2 == ar2.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }

        // Test that allocator does not propagate on move assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = std::move(x);
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a2);
            ASSERT(2 == ar1.blocks_outstanding()); // implementation-specific
            ASSERT(2 == ar2.blocks_outstanding()); // required behavior
            ASSERT(&x.front() == frontptr); // implementation-specific
            ASSERT(&y.front() != frontptr); // required behavior
        }

        // Test that swap works with equal allocators
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* xfrontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a1);
            y.push_back(99);
            y.push_front(98);
            int* yfrontptr = &y.front();
            int* ybackptr = &y.back();
            ASSERT(2 == y.size());
            ASSERT(98 == y.front());
            ASSERT(99 == y.back());
            ASSERT(y.get_allocator() == a1);

            x.swap(y);
            ASSERT(2 == x.size());
            ASSERT(98 == x.front());
            ASSERT(99 == x.back());
            ASSERT(x.get_allocator() == a1);
            ASSERT(&x.front() == yfrontptr);
            ASSERT(&x.back() == ybackptr);
            ASSERT(5 == ar1.blocks_outstanding());
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(&y.front() == xfrontptr);
        }

      } if (test != 0) break;

      case 5:
      {
        // --------------------------------------------------------------------
        // TEST allocator propagation using WeirdAllocator
        // --------------------------------------------------------------------

#undef TESTALLOC
#define TESTALLOC WeirdAllocator

        std::cout << "\nWeirdAllocator propagation"
                  << "\n==========================" << std::endl;

        AllocResource ar1, ar2;

        // Test that allocator doesn't propagate on copy construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(x);
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator().resource() == &defaultResource);
            ASSERT(2 == ar1.blocks_outstanding());
            ASSERT(2 == defaultResource.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }
        
        // Test that allocator propagates on move construction.
        {
            TESTALLOC<int> a1(&ar1);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(std::move(x));
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(3 == ar1.blocks_outstanding());
            ASSERT(&y.front() == frontptr);
        }
        
        // Test that allocator propagates on copy assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = x;
            ASSERT(y == x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(4 == ar1.blocks_outstanding());
            ASSERT(0 == ar2.blocks_outstanding());
            ASSERT(&x.front() == frontptr);
            ASSERT(&y.front() != frontptr);
        }

        // Test that allocator propagates on move assignment.
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* frontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y = std::move(x);
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            // implementation-specific:
            ASSERT(2 == ar1.blocks_outstanding());
            ASSERT(1 == defaultResource.blocks_outstanding());

            // required behavior:
            ASSERT(0 == ar2.blocks_outstanding()); 
            ASSERT(&y.front() == frontptr);
        }

        // Test that swap works with unequal allocators
        {
            TESTALLOC<int> a1(&ar1);
            TESTALLOC<int> a2(&ar2);

            XSTD::list<int, TESTALLOC<int> > x(a1);
            x.push_back(3);
            int* xfrontptr = &x.front();
            ASSERT(1 == x.size());
            ASSERT(3 == x.front());
            ASSERT(x.get_allocator() == a1);
            ASSERT(2 == ar1.blocks_outstanding());

            XSTD::list<int, TESTALLOC<int> > y(a2);
            y.push_back(99);
            y.push_front(98);
            int* yfrontptr = &y.front();
            int* ybackptr = &y.back();
            ASSERT(2 == y.size());
            ASSERT(98 == y.front());
            ASSERT(99 == y.back());
            ASSERT(y.get_allocator() == a2);

            x.swap(y);
            ASSERT(2 == x.size());
            ASSERT(98 == x.front());
            ASSERT(99 == x.back());
            ASSERT(x.get_allocator() == a2);
            ASSERT(&x.front() == yfrontptr);
            ASSERT(&x.back() == ybackptr);
            ASSERT(3 == ar2.blocks_outstanding());
            ASSERT(1 == y.size());
            ASSERT(3 == y.front());
            ASSERT(y.get_allocator() == a1);
            ASSERT(&y.front() == xfrontptr);
            ASSERT(2 == ar1.blocks_outstanding());
        }

      } if (test != 0) break;

      break; // Break at end of numbered tests

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

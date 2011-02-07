// xstd_list.t.cpp                  -*-C++-*-

#include <xstd_list.h>

#include <iostream>
#include <cstdlib>
#include <climits>

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//
//

//-----------------------------------------------------------------------------

//==========================================================================
//                  STANDARD BDE ASSERT TEST MACRO
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

template <typename Tp>
class SimpleAllocator
{
    AllocResource *resource_;

  public:
    typedef Tp              value_type;

    template <typename T>
    struct rebind
    {
	typedef SimpleAllocator<T> other;
    };

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
    friend class GenericFancyPointer;
    friend class ConstGenericFancyPointer;

    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<void> p);

    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<const void> p);

    Tp* value_;
public:
    FancyPointer(UniqPointerType p = nullptr)
	: value_(0) { ASSERT(p == nullptr); }
    template <typename T> FancyPointer(const FancyPointer<T>& p)
	{ value_ = p.ptr(); }

    Tp& operator*() const { return *value_; }
    Tp* operator->() const { return value_; }
    Tp* ptr() const { return value_; }

    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }
};

template <>
class FancyPointer<void>
{
    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<void> p);

    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<const void> p);

    void* value_;
public:
    FancyPointer(UniqPointerType p = nullptr)
	: value_(0) { ASSERT(p == nullptr); }
    template <typename T> FancyPointer(const FancyPointer<T>& p)
	: value_(p.ptr()) { }

    void* ptr() const { return value_; }
    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }
};

template <>
class FancyPointer<const void>
{
    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<void> p);

    template <typename T>
    friend FancyPointer<T> pointer_rebind(FancyPointer<const void> p);

    const void* value_;
public:
    FancyPointer(UniqPointerType p = nullptr)
	: value_(0) { ASSERT(p == nullptr); }
    template <typename T> FancyPointer(const FancyPointer<T>& p)
	: value_(p.ptr()) { }

    const void* ptr() const { return value_; }
    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }
};

template <typename T>
inline FancyPointer<T> pointer_rebind(FancyPointer<void> p)
{
    FancyPointer<T> ret;
    ret.value_ = static_cast<T*>(p.ptr());
    return ret;
}

template <typename T>
inline FancyPointer<T> pointer_rebind(FancyPointer<const void> p)
{
    FancyPointer<T> ret;
    ret.value_ = static_cast<T*>(p.ptr());
    return ret;
}

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

#ifdef TEMPLATE_ALIASES
    template <typename T> using rebind_type = FancyAllocator<T>;
#else // !TEMPLATE_ALIASES
    template <typename T>
    struct rebind
    {
	typedef FancyAllocator<T> other;
    };
#endif // !TEMPLATE_ALIASES

    FancyAllocator(AllocResource* ar = nullptr) : resource_(ar) { }

    // Required constructor
    template <typename T>
    FancyAllocator(const FancyAllocator<T>& other)
        : resource_(other.resource()) { }

    // FancyAllocator propagation on construction
    static FancyAllocator
    select_on_container_copy_construction(const FancyAllocator& rhs)
        { return rhs; }

    // FancyAllocator propagation functions.  Return true if *this was
    // modified.
    bool on_container_copy_assignment(const FancyAllocator& rhs)
        { return false; }
    bool on_container_move_assignment(FancyAllocator&& rhs)
        { return false; }
    bool on_container_swap(FancyAllocator& other)
        { return false; }

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
        // TEST using SimpleAllocator
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
        // TEST using FancyAllocator
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

// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2009
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------

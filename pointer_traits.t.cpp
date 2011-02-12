/* pointer_traits.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <pointer_traits.h>

#include <iostream>
#include <cstdlib>

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

#define IS_SAME(T,U) (std::is_same<T,U>::value)

//=============================================================================
//                  CLASSES FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

struct UniqDummyType { void zzzzz(UniqDummyType, bool) { } };
typedef void (UniqDummyType::*UniqPointerType)(UniqDummyType);

typedef void (UniqDummyType::*ConvertibleToBoolType)(UniqDummyType, bool);
const ConvertibleToBoolType ConvertibleToTrue = &UniqDummyType::zzzzz;

template <typename Tp>
class Pointer1
{
    Tp* value_;

public:
    Pointer1(Tp* p = nullptr)
	: value_(p) { }
    template <typename T> Pointer1(const Pointer1<T>& p)
	{ value_ = p.ptr(); }
    explicit Pointer1(const Pointer1<void>& p)
	: value_(static_cast<Tp*>(p.ptr())) { }
    explicit Pointer1(const Pointer1<const void>& p)
	: value_(static_cast<const Tp*>(p.ptr())) { }

    typename std::add_lvalue_reference<Tp>::type
      operator*() const { return *value_; }
    Tp* operator->() const { return value_; }
    Tp* ptr() const { return value_; }

    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }
};

template <typename Tp1, typename Tp2>
bool operator==(Pointer1<Tp1> a, Pointer1<Tp2> b)
{
    return a.ptr() == b.ptr();
}

template <typename Tp1, typename Tp2>
bool operator!=(Pointer1<Tp1> a, Pointer1<Tp2> b)
{
    return ! (a == b);
}

template <typename Unused, typename Tp>
class Pointer2
{
    Tp* value_;

public:
    typedef Tp element_type;
    typedef int difference_type;

#ifdef TEMPLATE_ALIASES
    template <typename U> using rebind = Pointer2<Unused,U>;
#else
    template <typename U> struct rebind {
        typedef Pointer2<Unused,U> other;
    };
#endif

    Pointer2(Tp* p = nullptr)
	: value_(p) { }
    template <typename T> Pointer2(const Pointer2<Unused,T>& p)
	{ value_ = p.ptr(); }
//     explicit Pointer2(const Pointer2<Unused,void>& p)
// 	: value_(static_cast<Tp*>(p.ptr())) { }
//     explicit Pointer2(const Pointer2<Unused,const void>& p)
// 	: value_(static_cast<const Tp*>(p.ptr())) { }

    typename std::add_lvalue_reference<Tp>::type
      operator*() const { return *value_; }
    Tp* operator->() const { return value_; }
    Tp* ptr() const { return value_; }

    static Pointer2 pointer_to(Tp& r) { return Pointer2(XSTD::addressof(r)); } 

    operator ConvertibleToBoolType() const
        { return value_ ? ConvertibleToTrue : nullptr; }

    template <typename T>
    Pointer2<Unused,T> static_pointer_cast() const {
        return Pointer2<Unused,T>(static_cast<T*>(value_));
    }

    template <typename T>
    Pointer2<Unused,T> dynamic_pointer_cast() const {
        return Pointer2<Unused,T>(dynamic_cast<T*>(value_));
    }
};

template <typename Unused, typename Tp1, typename Tp2>
bool operator==(Pointer2<Unused,Tp1> a, Pointer2<Unused,Tp2> b)
{
    return a.ptr() == b.ptr();
}

template <typename Unused, typename Tp1, typename Tp2>
bool operator!=(Pointer2<Unused,Tp1> a, Pointer2<Unused,Tp2> b)
{
    return ! (a == b);
}

struct Base {
    virtual ~Base() { }
    int i;
};

struct Derived : Base {
    double d;
};

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
        // TEST raw pointer traits
        // --------------------------------------------------------------------

        std::cout << "\nraw pointer traits"
                  << "\n==================" << std::endl;

        typedef double T;
        typedef int    U;
        typedef T* TPtr;
        typedef U* UPtr;
        typedef const T* CTPtr;
        typedef XSTD::pointer_traits<TPtr> TPtrTraits;

        ASSERT(!IS_SAME(TPtr,UPtr));
        ASSERT( IS_SAME(TPtr,TPtrTraits::pointer));
        ASSERT( IS_SAME(T,TPtrTraits::element_type));
        ASSERT( IS_SAME(std::ptrdiff_t, TPtrTraits::difference_type));
#ifdef TEMPLATE_ALIASES
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>));
#else
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>::other));
#endif

        T t;
        const TPtr tp(&t);
        T& tr = *tp;
        const T& ctr = *tp;
        auto tp2 = TPtrTraits::pointer_to(tr);
        auto tp3 = XSTD::pointer_traits<CTPtr>::pointer_to(ctr);
        ASSERT( IS_SAME(decltype(tp2),TPtr));
        ASSERT(tp2 == tp);
        ASSERT( IS_SAME(decltype(tp3),CTPtr))
#ifdef TEMPLATE_ALIASES
        ASSERT( IS_SAME(decltype(tp3), TPtrTraits::rebind<const T>));
#else
        ASSERT( IS_SAME(decltype(tp3), TPtrTraits::rebind<const T>::other));
#endif
        ASSERT(tp3 == tp);

        CTPtr ctp1 = tp;
        auto tp4 = XSTD::const_pointer_cast<T>(ctp1);
        ASSERT( IS_SAME(decltype(tp4),TPtr));
        ASSERT(tp4 == tp);

        typedef Base* BPtr;
        typedef Derived* DPtr;

        Derived d;
        BPtr bp1 = &d;
        const DPtr dp1 = &d;
        auto dp2 = XSTD::static_pointer_cast<Derived>(bp1);
        ASSERT( IS_SAME(decltype(dp2),DPtr));
        ASSERT(dp1 == dp2);

        auto dp3 = XSTD::dynamic_pointer_cast<Derived>(bp1);
        ASSERT( IS_SAME(decltype(dp3),DPtr));
        ASSERT(dp1 == dp3);

      } if (test != 0) break;

      case 2:
      {
        // --------------------------------------------------------------------
        // TEST Pointer1 traits
        // --------------------------------------------------------------------

        std::cout << "\nPointer1 traits"
                  << "\n===============" << std::endl;

        typedef double T;
        typedef int    U;
        typedef Pointer1<T> TPtr;
        typedef Pointer1<U> UPtr;
        typedef Pointer1<const T> CTPtr;
        typedef XSTD::pointer_traits<TPtr> TPtrTraits;

        ASSERT(!IS_SAME(TPtr,UPtr));
        ASSERT( IS_SAME(TPtr,TPtrTraits::pointer));
        ASSERT( IS_SAME(T,TPtrTraits::element_type));
        ASSERT( IS_SAME(std::ptrdiff_t, TPtrTraits::difference_type));
#ifdef TEMPLATE_ALIASES
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>));
#else
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>::other));
#endif

        T t;
        CTPtr ctp1(&t);
        auto tp4 = XSTD::const_pointer_cast<T>(ctp1);
        ASSERT( IS_SAME(decltype(tp4),TPtr));
        ASSERT(tp4 == ctp1);

        // Pointer1 has no pointer_to() function.

        typedef Pointer1<Base> BPtr;
        typedef Pointer1<Derived> DPtr;

        Derived d;
        const DPtr dp1(&d);
        Pointer1<void> vp1(dp1);
        auto dp2 = XSTD::static_pointer_cast<Derived>(vp1);
        ASSERT( IS_SAME(decltype(dp2),DPtr));
        ASSERT(dp1 == dp2);

      } if (test != 0) break;

      case 3:
      {
        // --------------------------------------------------------------------
        // TEST Pointer2 traits
        // --------------------------------------------------------------------

        std::cout << "\nPointer2 traits"
                  << "\n===============" << std::endl;

        typedef double T;
        typedef int    U;
        typedef Pointer2<void,T> TPtr;
        typedef Pointer2<void,U> UPtr;
        typedef Pointer2<void,const T> CTPtr;
        typedef XSTD::pointer_traits<TPtr> TPtrTraits;

        ASSERT(!IS_SAME(TPtr,UPtr));
        ASSERT( IS_SAME(TPtr,TPtrTraits::pointer));
        ASSERT( IS_SAME(T,TPtrTraits::element_type));
        ASSERT( IS_SAME(std::ptrdiff_t, TPtrTraits::difference_type));
#ifdef TEMPLATE_ALIASES
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>));
#else
        ASSERT( IS_SAME(UPtr,TPtrTraits::rebind<U>::other));
#endif

        T t;
        const TPtr tp(&t);
        T& tr = *tp;
        const T& ctr = *tp;
        auto tp2 = TPtrTraits::pointer_to(tr);
        auto tp3 = XSTD::pointer_traits<CTPtr>::pointer_to(ctr);
        ASSERT( IS_SAME(decltype(tp2),TPtr));
        ASSERT(tp2 == tp);
        ASSERT( IS_SAME(decltype(tp3),CTPtr))
#ifdef TEMPLATE_ALIASES
        ASSERT( IS_SAME(decltype(tp3), TPtrTraits::rebind<const T>));
#else
        ASSERT( IS_SAME(decltype(tp3), TPtrTraits::rebind<const T>::other));
#endif
        ASSERT(tp3 == tp);

        CTPtr ctp1 = tp;
        auto tp4 = XSTD::const_pointer_cast<T>(ctp1);
        ASSERT( IS_SAME(decltype(tp4),TPtr));
        ASSERT(tp4 == tp);

//        auto tpx = XSTD::const_pointer_cast<U>(tp);

        typedef Pointer2<int,Base> BPtr;
        typedef Pointer2<int,Derived> DPtr;

        Derived d;
        const DPtr dp1(&d);
        const BPtr bp1(dp1);
        auto dp2 = XSTD::static_pointer_cast<Derived>(bp1);
        ASSERT( IS_SAME(decltype(dp2),DPtr));
        ASSERT(dp1 == dp2);

        auto dp3 = XSTD::dynamic_pointer_cast<Derived>(BPtr(dp1));
        ASSERT( IS_SAME(decltype(dp3),DPtr));
        ASSERT(dp1 == dp3);

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

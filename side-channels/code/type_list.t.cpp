/* type_list.t.cpp                                                    -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <type_list.h>

#include <type_traits>
#include <cassert>

#define UNPAREN(...) UNPAREN_CLEANUP(UNPAREN_NORMALIZE __VA_ARGS__)

// Whether invoked as `UNPAREN_NORMALIZE(ARGS, ...)` (with parentheses) or
// `UNPAREN_NORMALIZE ARGS, ...` (without parentheses), the resulting
// *normalized* expansion is `UNPAREN_NORMALIZE ARGS, ...`.  Note that the
// second form is not actually a macro expansion but rather the result of *not*
// expanding a function-like macro (because of the absence of parentheses).
#define UNPAREN_NORMALIZE(...) UNPAREN_NORMALIZE __VA_ARGS__

// Invoked as `UNPAREN_CLEANUP(UNPAREN_NORMALIZE ARGS, ...)` and expand it to
// `UNPAREN_CLEANUP(UNPAREN_REMOVE_UNPAREN_NORMALIZE ARGS, ...)`, which, in
// turn, expands to simply `ARGS, ...` (because
// `UNPAREN_REMOVE_UNPAREN_NORMALIZE` expands to nothing).  The indirection
// through `UNPAREN_CLEANUP2` is needed because the splice operator (`##`) is
// eager and it is necessary to expand any macros within the argument list
// before applying the splice.
#define UNPAREN_CLEANUP(...) UNPAREN_CLEANUP2(__VA_ARGS__)
#define UNPAREN_CLEANUP2(...) UNPAREN_REMOVE_ ## __VA_ARGS__

// Expand to nothing.  This expression-like macro is "invoked" as
// `UNPAREN_UNPAREN_NORMALIZE ARGS, ...` and leaves only `ARGS, ...`.
#define UNPAREN_REMOVE_UNPAREN_NORMALIZE

#define ASSERT_SAME(T1, ...) \
  static_assert(std::is_same_v<UNPAREN(T1), __VA_ARGS__>)

using EmptyTL = type_list<>;
using TestTL  = type_list<char, unsigned short, unsigned, float>;

// Test `type_list::size()`
static_assert(0 == EmptyTL::size());
static_assert(4 == TestTL::size());

// Test `type_list::head` and `type_list::tail`
ASSERT_SAME(TestTL::head, char);
ASSERT_SAME(TestTL::tail, type_list<unsigned short, unsigned, float>);

// Test `type_list_size` metafunction
static_assert(0 == type_list_size_v<EmptyTL>);
static_assert(4 == type_list_size_v<TestTL>);

// Test `type_list_element` metafunction
ASSERT_SAME(char          , type_list_element_t<0, TestTL>);
ASSERT_SAME(unsigned short, type_list_element_t<1, TestTL>);
ASSERT_SAME(unsigned int  , type_list_element_t<2, TestTL>);
ASSERT_SAME(float         , type_list_element_t<3, TestTL>);

// Test `type_list_concat` metafunction
using TestTLSquared = type_list<char, unsigned short, unsigned, float,
                                char, unsigned short, unsigned, float>;
ASSERT_SAME(EmptyTL      , type_list_concat_t<EmptyTL, EmptyTL>);
ASSERT_SAME(TestTL       , type_list_concat_t<TestTL , EmptyTL>);
ASSERT_SAME(TestTL       , type_list_concat_t<EmptyTL, TestTL >);
ASSERT_SAME(TestTLSquared, type_list_concat_t<TestTL , TestTL >);

// Test `type_list_count`
template <std::size_t SZ>
struct size_test
{
  template <class T>
  struct match
  {
    static constexpr inline bool value = (sizeof(T) == SZ);
  };

  template <class T>
  struct at_least
  {
    static constexpr inline bool value = (sizeof(T) >= SZ);
  };
};

static_assert(0 == type_list_count_v<EmptyTL, std::is_unsigned>);
static_assert(2 == type_list_count_v<TestTL , std::is_unsigned>);
static_assert(0 == type_list_count_v<EmptyTL, size_test<1>::match>);
static_assert(1 == type_list_count_v<TestTL,  size_test<1>::match>);
static_assert(1 == type_list_count_v<TestTL,  size_test<2>::match>);
static_assert(2 == type_list_count_v<TestTL,  size_test<4>::match>);
static_assert(0 == type_list_count_v<EmptyTL, size_test<1>::at_least>);
static_assert(4 == type_list_count_v<TestTL,  size_test<1>::at_least>);
static_assert(3 == type_list_count_v<TestTL,  size_test<2>::at_least>);
static_assert(2 == type_list_count_v<TestTL,  size_test<4>::at_least>);

static_assert(0 == type_list_find_v<EmptyTL, std::is_unsigned>);
static_assert(0 == type_list_find_v<TestTL , std::is_signed>);
static_assert(1 == type_list_find_v<TestTL , std::is_unsigned>);
static_assert(0 == type_list_find_v<EmptyTL, size_test<1>::match>);
static_assert(0 == type_list_find_v<TestTL,  size_test<1>::match>);
static_assert(1 == type_list_find_v<TestTL,  size_test<2>::match>);
static_assert(2 == type_list_find_v<TestTL,  size_test<4>::match>);
static_assert(4 == type_list_find_v<TestTL,  size_test<5>::match>);
static_assert(0 == type_list_find_v<EmptyTL, size_test<1>::at_least>);
static_assert(0 == type_list_find_v<TestTL,  size_test<1>::at_least>);
static_assert(1 == type_list_find_v<TestTL,  size_test<2>::at_least>);
static_assert(2 == type_list_find_v<TestTL,  size_test<4>::at_least>);
static_assert(4 == type_list_find_v<TestTL,  size_test<5>::at_least>);

int main()
{
}

// Local Variables:
// c-basic-offset: 2
// End:

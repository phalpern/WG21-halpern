/* polymorphic_allocator.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <resource_adaptor.h>

#include <iostream>
#include <deque>

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

# define TEST_ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  CLASSES FOR TESTING
//-----------------------------------------------------------------------------

// Description of an allocated block.
struct Block
{
    constexpr Block(size_t sz, size_t a) : d_size(sz), d_align(a) { }

    size_t d_size;
    size_t d_align;
};

// Dummy allocator doesn't actually allocate memory, but allocates objects of
// type `Block` containing the size and alignment of requested memory.
template <typename Tp>
class DummyAllocator
{
    std::deque<Block> *d_blocks_p;

  public:
    typedef Tp value_type;

    DummyAllocator(std::deque<Block>* blocks_p) : d_blocks_p(blocks_p) { }

    // Required constructor
    template <typename T>
    DummyAllocator(const DummyAllocator<T>& other)
        : d_blocks_p(other.blocks()) { }

    Tp* allocate(std::size_t n) {
        return reinterpret_cast<Tp*>(
            &d_blocks_p->emplace_back(sizeof(Tp)*n, alignof(Tp)));
    }

    void deallocate(Tp* p, std::size_t n) {
        Block *bp = reinterpret_cast<Block*>(p);
        TEST_ASSERT(bp->d_size == sizeof(Tp) * n);
        TEST_ASSERT(bp->d_align == alignof(Tp));
        bp->d_size = bp->d_align = 0;
    }

    std::deque<Block> *blocks() const { return d_blocks_p; }
};

template <typename Tp1, typename Tp2>
bool operator==(const DummyAllocator<Tp1>& a, const DummyAllocator<Tp2>& b)
{
    return a.blocks() == b.blocks();
}

template <typename Tp1, typename Tp2>
bool operator!=(const DummyAllocator<Tp1>& a, const DummyAllocator<Tp2>& b)
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
    using namespace XPMR;

#ifdef QUICK_TEST
    XPMR::resource_adaptor<std::allocator<char>, 64> crx;
    void *p = crx.allocate(1, 4);
    crx.deallocate(p, 1, 4);
#else
    {
        // Test with default `MaxAlignment`

        std::deque<Block> blocks;

        XPMR::resource_adaptor<DummyAllocator<char>> crx(&blocks);

        // Shouldn't compile because 9 is not a power of 2
        // XPMR::resource_adaptor<DummyAllocator<char>, 9> bad(&blocks);

        std::size_t a;
        for (a = 1; a <= XSTD::max_align_v; a *= 2) {
            Block* b1 = static_cast<Block*>(crx.allocate(1, a));
            TEST_ASSERT(b1->d_size == a);  // 1 rounded up to alignment
            TEST_ASSERT(b1->d_align == a);
            Block* b2 = static_cast<Block*>(crx.allocate(a, a));
            TEST_ASSERT(b2->d_size == a);
            TEST_ASSERT(b2->d_align == a);
            Block* b3 = static_cast<Block*>(crx.allocate(3 * a, a));
            TEST_ASSERT(b3->d_size == 3 * a);
            TEST_ASSERT(b3->d_align == a);

            crx.deallocate(b1, 1,     a);
            crx.deallocate(b2, a,     a);
            crx.deallocate(b3, 3 * a, a);
        }

        // `a` is now double `MaxAlign`
        try {
            [[maybe_unused]] void* p = crx.allocate(1, a);
            TEST_ASSERT(false && "Allocation should have failed");
        }
        catch (const std::bad_alloc&)
        {
        }
    }

    {
        // Test with over-aligned `MaxAlignment`

        std::deque<Block> blocks;

        XPMR::resource_adaptor<DummyAllocator<short>,
                               4 * XSTD::max_align_v> crx(&blocks);

        std::size_t a;
        for (a = 1; a <= 4 * XSTD::max_align_v; a *= 2) {
            Block* b1 = static_cast<Block*>(crx.allocate(1, a));
            TEST_ASSERT(b1->d_size == a);
            TEST_ASSERT(b1->d_align == a);
            Block* b2 = static_cast<Block*>(crx.allocate(a, a));
            TEST_ASSERT(b2->d_size == a);
            TEST_ASSERT(b2->d_align == a);
            Block* b3 = static_cast<Block*>(crx.allocate(3 * a, a));
            TEST_ASSERT(b3->d_size == 3 * a);
            TEST_ASSERT(b3->d_align == a);

            crx.deallocate(b1, 1,     a);
            crx.deallocate(b2, a,     a);
            crx.deallocate(b3, 3 * a, a);
        }

        // `a` is now double `MaxAlign`
        try {
            [[maybe_unused]] void* p = crx.allocate(1, a);
            TEST_ASSERT(false && "Allocation should have failed");
        }
        catch (const std::bad_alloc&)
        {
        }
    }

    return testStatus;
#endif // ! QUICK_TEST
}

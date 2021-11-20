// aligned_type.t.cpp                                                 -*-C++-*-

#include "aligned_type.h"

#include <iostream>
#include <cassert>

//=============================================================================
//                  FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

template <size_t A, size_t ReqSz, size_t ExpSz>
constexpr void testRawAlignedStorageImp()
{
    using Obj = XSTD::raw_aligned_storage<A, ReqSz>;

    static_assert(std::is_same_v<Obj, typename Obj::type>, "`type` check");
    static_assert(Obj::alignment == A, "`alignment` member check");
    static_assert(Obj::size      == ExpSz, "`size` member check");
    static_assert(alignof(Obj)   == A, "alignment check");
    static_assert(sizeof(Obj)    == ExpSz, "size check");

    Obj x;
    static_assert(x.data() == static_cast<void*>(&x));
}

template <size_t A>
constexpr void testRawAlignedStorage()
{
    // Test using default size
    using Obj = XSTD::raw_aligned_storage<A>;

    static_assert(Obj::size == A, "size constant check");
    static_assert(Obj::alignment == A, "alignment constant check");
    static_assert(sizeof(Obj) == A, "size check");
    static_assert(alignof(Obj) == A, "alignment check");

    // Test with different requested sizes

    //                          A  req size               exp size
    //                          -  ---------------------  --------
    testRawAlignedStorageImp<A, 1                    , A       >();
    testRawAlignedStorageImp<A, A                    , A       >();
    testRawAlignedStorageImp<A, 2*A                  , 2*A     >();
    testRawAlignedStorageImp<A, 2*A + 1              , 3*A     >();
    testRawAlignedStorageImp<A, 3*A - (A > 1 ? 1 : 0), 3*A     >();
}

template <size_t A, typename T>
constexpr void testAlignedType()
{
    static_assert(std::is_same_v<XSTD::aligned_type<A>, T>, "");
}

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main()
{
    using XSTD::raw_aligned_storage;
    using XSTD::aligned_storage_for;

    testRawAlignedStorage<0x000001>();
    testRawAlignedStorage<0x000002>();
    testRawAlignedStorage<0x000004>();
    testRawAlignedStorage<0x000008>();
    testRawAlignedStorage<0x000010>();
    testRawAlignedStorage<0x000020>();
    testRawAlignedStorage<0x000040>();
    testRawAlignedStorage<0x000080>();
    testRawAlignedStorage<0x000100>();
    testRawAlignedStorage<0x000200>();
    testRawAlignedStorage<0x000400>();

    static_assert(aligned_storage_for<int>::alignment == alignof(int));
    static_assert(aligned_storage_for<int>::size == sizeof(int));
    static_assert(aligned_storage_for<int[2]>::alignment == alignof(int));
    static_assert(aligned_storage_for<int[2]>::size == 2 * sizeof(int));
    aligned_storage_for<int> is;
    static_assert(&is.object() == static_cast<void*>(&is));

    testAlignedType<0x000001, unsigned char>();
    testAlignedType<0x000002, unsigned short>();
    testAlignedType<0x000004, unsigned int>();
    testAlignedType<0x000008, unsigned long>();
    testAlignedType<0x000010, long double>();
    testAlignedType<0x000020, raw_aligned_storage<0x000020>>();
    testAlignedType<0x000040, raw_aligned_storage<0x000040>>();
    testAlignedType<0x000080, raw_aligned_storage<0x000080>>();
    testAlignedType<0x000100, raw_aligned_storage<0x000100>>();
    testAlignedType<0x000200, raw_aligned_storage<0x000200>>();
    testAlignedType<0x000400, raw_aligned_storage<0x000400>>();
    testAlignedType<0x000800, raw_aligned_storage<0x000800>>();
    testAlignedType<0x001000, raw_aligned_storage<0x001000>>();
    testAlignedType<0x002000, raw_aligned_storage<0x002000>>();
    testAlignedType<0x004000, raw_aligned_storage<0x004000>>();
    testAlignedType<0x008000, raw_aligned_storage<0x008000>>();
    testAlignedType<0x010000, raw_aligned_storage<0x010000>>();
    testAlignedType<0x020000, raw_aligned_storage<0x020000>>();
    testAlignedType<0x040000, raw_aligned_storage<0x040000>>();
    testAlignedType<0x080000, raw_aligned_storage<0x080000>>();
}

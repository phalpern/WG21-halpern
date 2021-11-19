// aligned_type.t.cpp                                                 -*-C++-*-

#include "aligned_type.h"

#include <iostream>

//=============================================================================
//                  FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

template <size_t A, size_t ReqSz, size_t ExpSz>
constexpr void testAlignedObjectStorageImp()
{
    using Obj = XSTD::aligned_object_storage<A, ReqSz>;

    static_assert(std::is_same_v<Obj, typename Obj::type>, "`type` check");
    static_assert(Obj::alignment == A, "`alignment` member check");
    static_assert(Obj::size      == ExpSz, "`size` member check");
    static_assert(alignof(Obj)   == A, "alignment check");
    static_assert(sizeof(Obj)   == ExpSz, "size check");

    Obj x;
    static_assert(x.get_data() == static_cast<void*>(&x));
}

template <size_t A>
constexpr void testAlignedObjectStorage()
{
    // Test using default size
    using Obj = XSTD::aligned_object_storage<A>;

    static_assert(Obj::size == A, "size constant check");
    static_assert(Obj::alignment == A, "alignment constant check");
    static_assert(sizeof(Obj) == A, "size check");
    static_assert(alignof(Obj) == A, "alignment check");

    // Test with different requested sizes

    //                          A  req size               exp size
    //                          -  ---------------------  --------
    testAlignedObjectStorageImp<A, 1                    , A       >();
    testAlignedObjectStorageImp<A, A                    , A       >();
    testAlignedObjectStorageImp<A, 2*A                  , 2*A     >();
    testAlignedObjectStorageImp<A, 2*A + 1              , 3*A     >();
    testAlignedObjectStorageImp<A, 3*A - (A > 1 ? 1 : 0), 3*A     >();
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
    using XSTD::aligned_object_storage;

    testAlignedObjectStorage<0x000001>();
    testAlignedObjectStorage<0x000002>();
    testAlignedObjectStorage<0x000004>();
    testAlignedObjectStorage<0x000008>();
    testAlignedObjectStorage<0x000010>();
    testAlignedObjectStorage<0x000020>();
    testAlignedObjectStorage<0x000040>();
    testAlignedObjectStorage<0x000080>();
    testAlignedObjectStorage<0x000100>();
    testAlignedObjectStorage<0x000200>();
    testAlignedObjectStorage<0x000400>();

    testAlignedType<0x000001, unsigned char>();
    testAlignedType<0x000002, unsigned short>();
    testAlignedType<0x000004, unsigned int>();
    testAlignedType<0x000008, unsigned long>();
    testAlignedType<0x000010, long double>();
    testAlignedType<0x000020, aligned_object_storage<0x000020>>();
    testAlignedType<0x000040, aligned_object_storage<0x000040>>();
    testAlignedType<0x000080, aligned_object_storage<0x000080>>();
    testAlignedType<0x000100, aligned_object_storage<0x000100>>();
    testAlignedType<0x000200, aligned_object_storage<0x000200>>();
    testAlignedType<0x000400, aligned_object_storage<0x000400>>();
    testAlignedType<0x000800, aligned_object_storage<0x000800>>();
    testAlignedType<0x001000, aligned_object_storage<0x001000>>();
    testAlignedType<0x002000, aligned_object_storage<0x002000>>();
    testAlignedType<0x004000, aligned_object_storage<0x004000>>();
    testAlignedType<0x008000, aligned_object_storage<0x008000>>();
    testAlignedType<0x010000, aligned_object_storage<0x010000>>();
    testAlignedType<0x020000, aligned_object_storage<0x020000>>();
    testAlignedType<0x040000, aligned_object_storage<0x040000>>();
    testAlignedType<0x080000, aligned_object_storage<0x080000>>();
}

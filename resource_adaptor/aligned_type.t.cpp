// aligned_type.t.cpp                                                 -*-C++-*-

#include "aligned_type.h"

#include <iostream>

//=============================================================================
//                  FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

template <size_t ReqSz, size_t A, size_t ExpSz>
constexpr void testAlignedObjectStorageImp()
{
    using Obj = XSTD::aligned_object_storage<ReqSz, A>;

    static_assert(std::is_same_v<Obj, typename Obj::type>, "`type` check");
    static_assert(Obj::alignment == A, "`alignment` member check");
    static_assert(Obj::size      == ExpSz, "`size` member check");
    static_assert(alignof(Obj)   == A, "alignment check");
    static_assert(sizeof(Obj)   == ExpSz, "size check");

    Obj x;
    static_assert(x.storage() == static_cast<void*>(&x));
}

template <size_t A>
constexpr void testAlignedObjectStorage()
{

    //                          req size               A  exp size
    //                          ---------------------  -  --------
    testAlignedObjectStorageImp<1                    , A, A       >();
    testAlignedObjectStorageImp<A                    , A, A       >();
    testAlignedObjectStorageImp<2*A                  , A, 2*A     >();
    testAlignedObjectStorageImp<2*A + 1              , A, 3*A     >();
    testAlignedObjectStorageImp<3*A - (A > 1 ? 1 : 0), A, 3*A     >();
}

template <size_t LoSz = 1, size_t HiSz = 1024>
constexpr size_t testAlignedObjectDefaultAlign()
{
    if constexpr (LoSz == HiSz) {
        // Recursion stop: Test `Sz`
        constexpr size_t Sz = LoSz;

        using Obj = XSTD::aligned_object_storage<Sz>;

        static_assert(0 == (Obj::alignment & (Obj::alignment - 1)), "2^n algn");
        static_assert(Obj::size == Sz, "Size check");
        static_assert(Sz % Obj::alignment == 0, "Min alignment check");
        static_assert(Obj::alignment <= alignof(max_align_t), "Align limit");
        static_assert(Obj::alignment == alignof(max_align_t) ||
                      Sz % (2 * Obj::alignment) != 0, "Max alignment check");

        return Obj::alignment;
    }
    else {
        // Recursively check subranges using a tree-structured recursion to
        // minimize compile-time stack depth.
        constexpr size_t MidSz = (LoSz + HiSz) / 2;
        return (testAlignedObjectDefaultAlign<LoSz, MidSz>() |
                testAlignedObjectDefaultAlign<MidSz + 1, HiSz>());
    }
};

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

    static_assert(0 != testAlignedObjectDefaultAlign(), "default alignment");

    testAlignedType<0x000001, unsigned char>();
    testAlignedType<0x000002, unsigned short>();
    testAlignedType<0x000004, unsigned int>();
    testAlignedType<0x000008, unsigned long>();
    testAlignedType<0x000010, std::max_align_t>();
    testAlignedType<0x000020, aligned_object_storage<0x000020, 0x000020>>();
    testAlignedType<0x000040, aligned_object_storage<0x000040, 0x000040>>();
    testAlignedType<0x000080, aligned_object_storage<0x000080, 0x000080>>();
    testAlignedType<0x000100, aligned_object_storage<0x000100, 0x000100>>();
    testAlignedType<0x000200, aligned_object_storage<0x000200, 0x000200>>();
    testAlignedType<0x000400, aligned_object_storage<0x000400, 0x000400>>();
    testAlignedType<0x000800, aligned_object_storage<0x000800, 0x000800>>();
    testAlignedType<0x001000, aligned_object_storage<0x001000, 0x001000>>();
    testAlignedType<0x002000, aligned_object_storage<0x002000, 0x002000>>();
    testAlignedType<0x004000, aligned_object_storage<0x004000, 0x004000>>();
    testAlignedType<0x008000, aligned_object_storage<0x008000, 0x008000>>();
    testAlignedType<0x010000, aligned_object_storage<0x010000, 0x010000>>();
    testAlignedType<0x020000, aligned_object_storage<0x020000, 0x020000>>();
    testAlignedType<0x040000, aligned_object_storage<0x040000, 0x040000>>();
    testAlignedType<0x080000, aligned_object_storage<0x080000, 0x080000>>();
}

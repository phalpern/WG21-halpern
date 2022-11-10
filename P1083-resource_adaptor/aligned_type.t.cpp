// aligned_type.t.cpp                                                 -*-C++-*-

#include "aligned_type.h"

#include <iostream>
#include <cassert>

//=============================================================================
//                  FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------

template <size_t A, size_t ReqSz, size_t ExpSz>
constexpr void testAlignedRawStorageImp()
{
    using Obj = XSTD::aligned_raw_storage<A, ReqSz>;

    static_assert(std::is_same_v<Obj, typename Obj::type>, "`type` check");
    static_assert(Obj::alignment == A,     "`alignment` member check");
    static_assert(Obj::size      == ExpSz, "`size` member check");
    static_assert(alignof(Obj)   == A,     "alignment check");
    static_assert(sizeof(Obj)    == ExpSz, "size check");

    Obj       x{};
    const Obj cx{};
    static_assert(x.data() == static_cast<void*>(&x),
                  "data() return check");
    static_assert(cx.data() == static_cast<const void*>(&cx),
                  "const data() return check");
}

template <size_t A>
constexpr void testAlignedRawStorage()
{
    // Test using default size
    using Obj = XSTD::aligned_raw_storage<A>;

    static_assert(Obj::size      == A, "size constant check");
    static_assert(Obj::alignment == A, "alignment constant check");
    static_assert(sizeof(Obj)    == A, "size check");
    static_assert(alignof(Obj)   == A, "alignment check");

    // Test with different requested sizes

    //                          A  req size            exp size
    //                          -  ------------------  --------
    testAlignedRawStorageImp<A, 1                    , A       >();
    testAlignedRawStorageImp<A, A                    , A       >();
    testAlignedRawStorageImp<A, 2*A                  , 2*A     >();
    testAlignedRawStorageImp<A, 2*A + 1              , 3*A     >();
    testAlignedRawStorageImp<A, 3*A - (A > 1 ? 1 : 0), 3*A     >();
}

template <typename T, size_t ExpSz>
constexpr void testAlignedObjectStorageImp()
{
    using Obj = XSTD::aligned_object_storage<T>;

    constexpr std::size_t A = alignof(T);

    static_assert(Obj::alignment == A,     "`alignment` member check");
    static_assert(Obj::size      == ExpSz, "`size` member check");
    static_assert(alignof(Obj)   == A,     "alignment check");
    static_assert(sizeof(Obj)    == ExpSz, "size check");

    Obj       x{};
    const Obj cx{};

    static_assert(x.data() == static_cast<void*>(&x),
                  "data() return check");
    static_assert(cx.data() == static_cast<const void*>(&cx),
                  "const data() return check");

    static_assert(std::is_same_v<decltype(x.object()), T&>,
                  "object() type check");
    static_assert(std::is_same_v<decltype(cx.object()), const T&>,
                  "const object() type check");

    assert(&x.object()  == reinterpret_cast<T*>(&x));
    assert(&cx.object() == reinterpret_cast<const T*>(&cx));
}

template <typename T, size_t ExpSz>
constexpr void testAlignedObjectStorage()
{
    testAlignedObjectStorageImp<               T, ExpSz>();
    testAlignedObjectStorageImp<const          T, ExpSz>();
    testAlignedObjectStorageImp<      volatile T, ExpSz>();
    testAlignedObjectStorageImp<const volatile T, ExpSz>();
}

template <size_t A, typename T>
constexpr void testAlignedType()
{
    static_assert(std::is_same_v<XSTD::aligned_type<A>, T>, "");
}

// Test struct.
struct X
{
    float f[3];
};

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main()
{
    using XSTD::aligned_raw_storage;
    using XSTD::aligned_object_storage;

    testAlignedRawStorage<0x000001>();
    testAlignedRawStorage<0x000002>();
    testAlignedRawStorage<0x000004>();
    testAlignedRawStorage<0x000008>();
    testAlignedRawStorage<0x000010>();
    testAlignedRawStorage<0x000020>();
    testAlignedRawStorage<0x000040>();
    testAlignedRawStorage<0x000080>();
    testAlignedRawStorage<0x000100>();
    testAlignedRawStorage<0x000200>();
    testAlignedRawStorage<0x000400>();

    //                       T       exp size
    //                       ------  ---------------
    testAlignedObjectStorage<int   , sizeof(int)    >();
    testAlignedObjectStorage<int[2], 2 * sizeof(int)>();
    testAlignedObjectStorage<X     , sizeof(X)      >();
    testAlignedObjectStorage<X[3]  , 3 * sizeof(X)  >();

    testAlignedType<0x000001, unsigned char>();
    testAlignedType<0x000002, unsigned short>();
    testAlignedType<0x000004, unsigned int>();
    testAlignedType<0x000008, unsigned long>();
    testAlignedType<0x000010, long double>();
    testAlignedType<0x000020, aligned_raw_storage<0x000020>>();
    testAlignedType<0x000040, aligned_raw_storage<0x000040>>();
    testAlignedType<0x000080, aligned_raw_storage<0x000080>>();
    testAlignedType<0x000100, aligned_raw_storage<0x000100>>();
    testAlignedType<0x000200, aligned_raw_storage<0x000200>>();
    testAlignedType<0x000400, aligned_raw_storage<0x000400>>();
    testAlignedType<0x000800, aligned_raw_storage<0x000800>>();
    testAlignedType<0x001000, aligned_raw_storage<0x001000>>();
    testAlignedType<0x002000, aligned_raw_storage<0x002000>>();
    testAlignedType<0x004000, aligned_raw_storage<0x004000>>();
    testAlignedType<0x008000, aligned_raw_storage<0x008000>>();
    testAlignedType<0x010000, aligned_raw_storage<0x010000>>();
    testAlignedType<0x020000, aligned_raw_storage<0x020000>>();
    testAlignedType<0x040000, aligned_raw_storage<0x040000>>();
    testAlignedType<0x080000, aligned_raw_storage<0x080000>>();
}

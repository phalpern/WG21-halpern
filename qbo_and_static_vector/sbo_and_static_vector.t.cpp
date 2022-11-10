#include <sbo_and_static_vector.h>
#include <iostream>

template <class T>
bool isWithin(const T& obj, const void* p)
{
    auto b = reinterpret_cast<const unsigned char*>(&obj);
    auto e = reinterpret_cast<const unsigned char*>(1 + &obj);
    auto a = static_cast<const unsigned char*>(p);
    return (b <= a && a < e);
}

int main()
{
    std::static_vector<int, 10> sv;

    for (int i = 0; i < 10; ++i)
    {
        sv.push_back(i);
    }

    assert(isWithin(sv, &sv.front()));
    assert(isWithin(sv, &sv.back()));

    std::sbo_vector<int, 10> sbv;

    for (int i = 0; i < 10; ++i)
    {
        sbv.push_back(i);
    }

    assert(0 == sbv.front());
    assert(9 == sbv.back());
    assert(isWithin(sbv, &sbv.front()));
    assert(isWithin(sbv, &sbv.back()));

    sbv.push_back(10);
    assert(0 == sbv.front());
    assert(10 == sbv.back());
    assert(! isWithin(sbv, &sbv.front()));
    assert(! isWithin(sbv, &sbv.back()));
}

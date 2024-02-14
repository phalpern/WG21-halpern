#include <variant>
#include <memory_resource>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

using namespace std;

int main()
{
pmr::monotonic_buffer_resource rsrc;
pmr::polymorphic_allocator<> alloc{ &rsrc };
using V = variant<pmr::string, int>;
V v = make_obj_using_allocator<V>(alloc,
                                  in_place_type<pmr::string>, "hello");
// assert(get<0>(v).get_allocator() == alloc);  // FAILS

pmr::vector<pmr::string> vs(alloc);
pmr::vector<V>           vv(alloc);

vs.emplace_back("hello");
vv.emplace_back("hello");

assert(vs.back().get_allocator() == alloc);          // OK
//assert(get<0>(vv.back()).get_allocator() == alloc);  // FAILS

vv.emplace_back(in_place_type<pmr::string>, "hello", alloc);
assert(get<0>(vv.back()).get_allocator() == alloc);  // OK

vv.back() = 5;          // Change alternative to int
vv.back() = "goodbye";  // Change alternative back to `pmr::string`
// assert(get<0>(vv.back()).get_allocator() == alloc);  // FAILS

V v1{ 5 };  // Does not use an allocator
V v2{ "hello" };  // Uses a default-constructed allocator
v1 = pmr::string("goodbye", alloc);
v2 = pmr::string("goodbye", alloc);
assert(get<0>(v1).get_allocator() == alloc);  // OK, set by move construction
//assert(get<0>(v2).get_allocator() == alloc);  // ERROR, set by assignment
}

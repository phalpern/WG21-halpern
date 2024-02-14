#include <optional>
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
using Opt = optional<pmr::string>;
Opt o = make_obj_using_allocator<Opt>(alloc, in_place, "hello");
// assert(o->get_allocator() == alloc);  // FAILS

pmr::vector<pmr::string> vs(alloc);
pmr::vector<Opt>         vo(alloc);

vs.emplace_back("hello");
vo.emplace_back("hello");

assert(vs.back().get_allocator() == alloc);          // OK
//assert(vo.back()->get_allocator() == alloc);  // FAILS

vo.emplace_back(in_place, "hello", alloc);
assert(vo.back()->get_allocator() == alloc);  // OK

vo.back() = nullopt;    // Disengage
vo.back() = "goodbye";  // Re-engage
// assert(vo.back()->get_allocator() == alloc);  // FAILS

Opt o1{nullopt};    // Disengaged -- does not use an allocator
Opt o2{ "hello" };  // String uses a default-constructed allocator
o1 = pmr::string("goodbye", alloc);    // Constructs the string
o2 = pmr::string("goodbye", alloc);    // Assigns to the string
assert(o1->get_allocator() == alloc);  // OK, set by move construction
//assert(o2->get_allocator() == alloc);  // ERROR, set by assignment
}

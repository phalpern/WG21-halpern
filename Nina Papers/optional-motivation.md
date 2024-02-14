Motivation
==========

General Motivation for allocator-aware types
--------------------------------------------

*Note: The text below is borrowed nearly verbetim from
[P3002](http://wg21.link/P3002R1), which proposes a general policy for when
types should use allocators*

Memory management is a major part of building software. Numerous facilities in
the C++ Standard library exist to give the programmer maximum control over how
their program uses memory:

* `std::unique_ptr` and `std::shared_ptr` are parameterized with *deleter*
  objects that control, among other things, how memory resources are reclaimed.

* `std::vector` is preferred over other containers in many cases because its
  use of contiguous memory provides optimal cache locality and minimizes
  allocate/deallocate operations. Indeed, the LEWG has spent a lot of time on
  `flat_set` ([P1222R0](http://wg21.link/P1222R0)) and `flat_map`
  ([P0429R3](http://wg21.link/P0429R3)), whose underlying structure defaults to
  `vector` for this reason.

* Operators `new` and `delete` are replaceable, giving programmers global
  control over how memory is allocated.

* The C++ object model makes a clear distinction between an object's memory
  footprint and it's lifetime.

* Language constructs such as `void*` and `reinterpet_cast` provide
  fine-grained access to objects' underlying memory.

* Standard containers and strings are parameterized with allocators, providing
  object-level control over memory allocation and element construction.

This fine-grained control over memory that C++ gives the programmer is a large
part of why C++ is applicable to so many domains --- from embedded systems with
limited memory budgets to games, high-frequency trading, and scientific
simulations that require cache locality, thread affinity, and other
memory-related performance optimizations.

An in-depth description of the value proposition for allocator-aware software
can be found in [P2035R0](http://wg21.link/P2035R0).  Standard containers are
the most ubiquitous examples of *allocator-aware* types. Their `allocator_type`
and `get_allocator` members and allocator-parameterized constructors allow them
to be used like Lego® parts that can be combined and nested as necessary while
retaining full programmer control over how the whole assembly allocates
memory. For *scoped allocators* --- those that apply not only to the top-level
container, but also to its elements --- having each element of a container
support a predictable allocator-aware interface is crucial to giving the
programmer the ability to allocate all memory from a single memory resource,
such as an arena or pool. Note that the allocator is a *configuration*
parameter of an object and does not contribute to its value.

In short, the principles underlying this policy proposal are:

1. **The Standard Library should be general and flexible**. To the extent
   possible, the user of a library class should have control over how memory is
   allocated.

2. **The Standard Library should be consistent**. The use of allocators should
   be consistent with the existing allocator-aware classes and class templates,
   especially those in the containers library.

3. **The parts of the Standard Library should work together**. If one part of
   the library gives the user control over memory allocation but another part
   does not, then the second part undermines the utility of the first.

4. **The Standard Library should encapsulate complexity**.  Fully general
   application of allocators is potentially complex and is best left to the
   experts implementing the Standard Library.  Users can choose their own
   subset of desirable allocator behavior only if the underlying Library
   classes allow them to choose their preferred approach, whether it be
   stateless allocators, statically typed allocators, polymorphic allocators,
   or no allocators.

Motivation for an Allocator-aware `optional`
--------------------------------------------

Although the engaged object  in an `std::optional` can be initialized with
any set of valid constructor arguments, including allocator arguments, the fact
that the `optional` itself is not allocator-aware prevents it from working
consistently with other parts of the standard library, specifically those parts
that depend on *uses-allocator construction* (section
[allocator.uses.construction]) in the standard). For example:

```
pmr::monotonic_buffer_resource rsrc;
pmr::polymorphic_allocator<> alloc{ &rsrc };
using Opt = optional<pmr::string>;
Opt o = make_obj_using_allocator<Opt>(alloc, in_place, "hello");
assert(o->get_allocator() == alloc);  // FAILS
```

Even though an allocator is supplied, it is not used to construct the
`pmr::string` within the resulting `optional` object because `optional` does
not have the necessary hooks for `make_obj_using_allocator` to recognize it as
being allocator-aware.  Note that, although this example and the ones that
follow use `pmr::polymorphic_allocator`, the same issues would apply to any
scoped allocator.

*Uses-allocator construction* is rarely used directly in user code.  Instead,
it is used within the implementation of standard containers and scoped
allocators to ensure that the allocator used to construct the container is also
used to construct its elements.  Continuing the example above, consider what
happens if an `optional` is stored in a `pmr::vector`, compared to storing a
truly allocator-aware type (`pmr::string`):

```
pmr::vector<pmr::string> vs(alloc);
pmr::vector<Opt>         vo(alloc);

vs.emplace_back("hello");
vo.emplace_back("hello");

assert(vs.back().get_allocator() == alloc);   // OK
assert(vo.back()->get_allocator() == alloc);  // FAILS
```

An important invariant when using a scoped allocator such as
`pmr::polymorphic_allocator` is that the same allocator is used throughout an
object hierarchy. It is impossible to ensure that this invariant is preserved
when using `std::optional`, even if the element is originally inserted with the
correct allocator, because `optional` does not remember the allocator used to
construct it and cannot therefore re-instate the allocator going from
disengaged to engaged:

```
vo.emplace_back(in_place, "hello", alloc);
assert(vo.back()->get_allocator() == alloc);  // OK

vo.back() = nullopt;    // Disengage
vo.back() = "goodbye";  // Re-engage
assert(vo.back()->get_allocator() == alloc);  // FAILS
```

Finally, when using assignment, the value stored in the `optional` is set
sometimes by construction and other times by assignment. Depending on the
allocator type's propagation traits, it is difficult to reason about the
resulting allocator:

```
Opt o1{nullopt};    // Disengaged -- does not use an allocator
Opt o2{ "hello" };  // String uses a default-constructed allocator
o1 = pmr::string("goodbye", alloc);    // Constructs the string
o2 = pmr::string("goodbye", alloc);    // Assigns to the string
assert(o1->get_allocator() == alloc);  // OK, set by move construction
assert(o2->get_allocator() == alloc);  // ERROR, set by assignment
```

Summary of the proposed feature
===============================

This paper proposes an allocator-aware `optional`. Unfortunately, it would be
complicated to add an allocator to the current `std::optional` without causing
API and ABI compatibility issues and/or imposing long compile times on code
that does not benefit from the change. For this reason, we are proposing a new
class template, `basic_optional`, which works like `optional`, but adds
allocator support.

The key attributes of `basic_optional` that make it different from `optional`
are:

- It has an optional `Allocator` template parameter.  If not supplied,
  `Allocator` is determined from `T::allocator_type` if that exists, otherwise
  `std::allocator<byte>`.

- It has a public `allocator_type` member to advertise that it is
  allocator-aware.

- There are allocator-enabled versions of every constructor.

- The supplied allocator is forwarded to the constructor of the engaged object,
  if that object uses a compatible allocator type.  The allocator is retained
  so that it can be supplied to the contained object whenever the
  `basic_optional` goes from disengaged to engaged. Allocator traits, including
  propagation traits, are respected.

- It has an alias, `pmr::optional`, for which `Allocator` is hard-coded to
  `pmr::polymorphic_allocator<>`.

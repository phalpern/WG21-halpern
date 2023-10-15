% Complete programmer manual for AA classes
% Pablo Halpern <<phalpern@halpernwightsoftware.com>>
% <!-- $TimeStamp$ -->2023-10-15 08:23 EDT<!-- $ -->
**NOT FOR WG21 DISTRIBUTION IN THIS FORM**

Abstract
========

Classes and class templates that allocate memory or contain subobjects that
allocate memory should provide a way for the user to control the source of such
memory.  As part of the effort to create a standing document of policies and
guidelines for Standard Library design, this document describes when a new
class or class template should have an allocator, how such allocator-awareness
should be rendered, and the rationale for these policies.  The guidelines are
illustrated by citing examples in the existing Standard.

Principles
==========

1. The user of a library class should have control over how memory is allocated
   by that class.

2. The library should be consistent with regard to the kind of things that use
   allocators and the interface by which allocators are supplied to objects.

3. The use of allocators should be consistent with the existing containers
   library.

Guidelines
==========

When should a class (or class template) be allocator-aware (AA)?
----------------------------------------------------------------

A library class should use an allocator if either or both of the following is
true.

o The class contains a subpart that uses an allocator.

o The class allocates (or sometimes allocates) memory for its own use.


Guidelines for determing a class's allocator type
-------------------------------------------------

o If an AA class does not use its allocator except during construction (i.e.,
  to construct subobjects), then the allocator may be supplied as a template
  argument to the constructor and not stored.  The allocator type in this case
  is *ephemeral*.

o Otherwise, when if `T` is an AA class template, `T` the allocator type should
  be a template parameter `A` and `T::allocator_type` should be an
  alias for `A` (or a variant of `A` formed by
  `allocator_traits<A>::rebind_alloc`)

o Otherwise, if AA class `X` cannot take an allocator template parameter (e.g.,
  if it is not a template), the allocator type should be a specialization of
  `pmr::polymorphic_allocator` and `X::allocator_type` should be an alias for
  that allocator type.

Guidelines for all AA classes
-----------------------------

Let `X` be an AA class, `alloc` be an allocator used to construct `X`, `A` be
the type of `alloc`, and `y` be an object that is logically part of 'X'.  The
following guidelines apply to `X` and, recursively, to `y`.  Note that the
target of a smart pointer is *not* logically part of the pointer, so many of
these guidelines do not apply.

o Every constructor of `X` (including its copy and move constructors) should be
  callable with an allocator argument used to establish `alloc`. The allocator
  is typically optional, either through overloading or via defaulted
  parameters. The parameter lists for the AA constructors must conform to
  either the *leading allocator convention* or *trailing allocator convention*,
  as described in section [allocator.uses.construction] (*Uses-allocator
  construction*) of the Standard.

o When called without an allocator argument, the move constructor should get
  `alloc` from the moved-from object.

o When called without an allocator argument, the copy constructor should get
  `alloc` from `allocator_traits<A>::select_on_container_copy_construction(c)`,
  where `c` is the allocator for the copied-from object.

o Throughout the lifetime of `X`, its allocator never changes unless one of the
  propagation traits (POCCA, POCMA, or POCS), is true, and then only by the
  corresponding operation.

o Any memory allocated by `X` for the purpose of holding objects that are
  logically part of `X` should allocated from `alloc`.

o If `y` is in allocated memory (e.g., it is a container element), it should be
  initialized by calling `allocator_traits<A>::construct(alloc, &y,` *args*`)`,
  where *args* represents the non-allocator constructor arguments to `y`.  For
  the purpose of these guidelines, a small-object buffer is considered
  allocated memory.

o If `y` is not in allocated memory (e.g., it is a member variable of `X`), it
  should be initialized via *uses-allocator construction with allocator*
  `alloc`.

o If `T` is a class template having allocator parameter `TA`, then `T` should
  supply a default type for `TA` of `allocator<V>`, where `V` is either
  `std::byte` or a reasonable value type of `T`.

o If `T` is a class template having allocator parameter `TA`, then `pmr::T`
  should be an alias for `T` supplying `polymorphic_allocator<V>` for `TA`,
  where `V` is either `std::byte` or a reasonable value type of `T`.

o If `X` has no member `X::allocator_type` (i.e., because the allocator type is
  ephemeral), then `uses_allocator<X, A>` should be specialized to inherit from
  `true_type` for all `A` that are valid allocator constructor arguments for
  `X` (typically, all object types).  This specialization will allow `X` to
  participate in uses-allocator construction.

o Whenever feasable, `X` should have a `const` member function
  `get_allocator()` that returns `alloc`.

When should a function have an allocator parameter?
---------------------------------------------------

A library function should accept an allocator argument if any of the following
is true.

o The function returns an object whose type uses an allocator, especially if
  the function is clearly a factory function (e.g., `make_*`).

o The function returns a pointer or smart pointer to a dynamcially allocated
  object.

o There is some benefit to giving the caller control over how the function
  allocates temporary memory. This is a rare case because, although many
  algorithms that allocate significant auxiliary storage can benefit from
  customed memory allocation, most would use their *own* local allocators tuned
  for efficiency of the algorithm, rather than an allocator provided by the
  caller.

Rationale
=========

Allocators are used in projects where to improve cache performance, take
advantage of special memory regions, and take advantage of parallel
hardware. The list of guidelines herein is relatively long because of the
ubiquity of memory allocation in most code as well as the somewhat twisted road
that allocators have taken in the evolution from C++98 to the more usable
version in present day C++; backward compatibility necessitates adaptors such
as `allocator_traits` whereas flexibility necessitates numerous "knobs and
dials." Although user-written AA types can fruitfully follow a subset of these
guidelines the Standard most endeavor to support the widest possible range of
users.

Type-erasure for an allocator is discouraged because

a. Its benefits are more cleanly achieved using `pmr::polymorphic_allocator`

b. It adds implementation complexity and performance risks.

a. Allocator Propagation traits cannot readily be honored


Examples in the current WP
==========================


Acknowledgments
===============

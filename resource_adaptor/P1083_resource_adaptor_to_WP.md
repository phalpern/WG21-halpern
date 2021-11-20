% D1083r4 | Move resource_adaptor from Library TS to the C++ WP
% Pablo Halpern <phalpern@halpernwightsoftware.com>
% 2021-11-05 | Target audience: LWG

Abstract
========

When the polymorphic allocator infrastructure was moved from the Library
Fundamentals TS to the C++17 working draft, `pmr::resource_adaptor` was left
behind. The decision not to move `pmr::resource_adaptor` was deliberately
conservative, but the absence of `resource_adaptor` in the standard is a
hole that must be plugged for a smooth transition to the ubiquitous use of
`polymorphic_allocator`, as proposed in [P0339](http://wg21.link/p0339) and
[P0987](http://wg21.link/p0987).  This paper proposes that
`pmr::resource_adaptor` be moved from the LFTS and added to the C++20 working
draft.

Status
======

On Oct 5, 2021, a subgroup of LWG reviewed P1083R3 and found an issue in the
way the max alignment supported by `pmr::resource_adaptor` was specified in the
paper. There was general consensus that a `MaxAlignment` template parameter
would be preferable, but the change was considered to be of a design nature and
therefore requires LEWG review.  The R4 version of this paper contains the
changes for LEWG review

History
=======

Changes from R3 to R4
---------------------

* Added an `aligned_type` metafunction, which evaluates to a trivial type
  having a specified size and alignment. To fully specify the result of
  instantiating `aligned_type`, `aligned_object_storage` was also introduced.
* Drive-by fix (optional): Added `max_aligned_v` as an easier spelling of
  `alignof(max_aligned_t)`.
* Added a second template argument to `pmr::resource_adaptor`, specifying the
  maximum  alignment to be supported by the instantiation (default
  `max_align_v`).
* A few editorial changes to comply with LWG style.

Changes from R2 to R3 (in Kona and pre-Cologne)
-----------------------------------------------

* Changed __`resource-adaptor-imp`__ to kabob case.
* Removed special member functions (copy/move ctors, etc.) and let them be
  auto-generated.
* Added a requirement that the `Allocator` template parameter must support
  rebinding to any non-class, non-over-aligned type. This allows the
  implementation of `do_allocate` to dispatch to a suitably rebound copy of
  the allocator as needed to support any native alignment argument.

Changes from R1 to R2 (in San Diego)
------------------------------------

* Paper was forwarded from LEWG to LWG on Tuesday, 2018-10-06
* Copied the formal wording from the LFTS directly into this paper
* Minor wording changes as per initial LWG review
* Rebased to the October 2018 draft of the C++ WP

Changes from R0 to R1 (pre-San Diego)
-------------------------------------

* Added a note for LWG to consider clarifying the alignment requirements for
  `resource_adaptor<A>::do_allocate()`.
* Changed rebind type from `char` to `byte`.
* Rebased to July 2018 draft of the C++ WP.

Motivation
==========

It is expected that more and more classes, especially those that would not
otherwise be templates, will use `pmr::polymorphic_allocator<byte>` to allocate
memory rather than specifying an allocator as a template parameter. In order to
pass an allocator to one of these classes, the allocator must either already be
a polymorphic allocator, or must be adapted from a non-polymorphic allocator.
The process of adaptation is facilitated by `pmr::resource_adaptor`, which is a
simple class template, has been in the LFTS for a long time, and has been fully
implemented. It is therefore a low-risk, high-benefit component to add to the
C++ WP.

Design decisions (for LEWG review)
==================================

The standard has a type, `std::max_align_t`, whose alignment is at least as
great as that of every scalar type. I found that I was continually referring to
the *value*, `alignof(std::max_align_t)`. In fact, *every single use* of
`max_align_t` in the standard is as an argument to `alignof`. As a drive-by
fix, therefore, this proposal introduces the constant `max_align_v` as a more
straightforward spelling of `alignof(max_align_t)`.  Note that LEWG might want
to change the name of this constant (e.g., by removing the "_v") and that the
introduction of this constant is completely severable from the proposal if it
is deemed undesirable.

The class template `std::aligned_object_storage` is intended to replace and
correct the problems with `std::aligned_storage`, which has been deprecated
(see [P1413](wg21.link/P1413)).  Specifically, it is not a metafunction, but a
`struct` template, and it provides direct access to its data buffer. The
relationship between size and alignment is specifically described in the
wording, so programmers can rely on it not wasting space. The alignment
parameter is still specified as a number rather than as a type (as needed for
low-level types like `pmr::resource_adaptor`), and the result must be cast to
the desired type before it's used, but it would be trivial to
provide an `aligned_storage_for` wrapper class, either in client code
or in the standard (not part of this proposal, but could be added):

```C++
template <typename T>
struct aligned_storage_for : aligned_object_storage<alignof(T), sizeof(T)>
{
    constexpr T& object() { return *static_cast<T *>(this->data()); }
    constexpr const T& object() const
        { return *static_cast<const T*>(this->data()); }
};
```

Finally, `aligned_object_storage` provides a `type` member
for backwards compatibility with the deprecated `aligned_storage`
metafunction. The `aligned_object_storage` class template is specified as being
in header `<utility>`, but LEWG could consider putting it in `<memory>`,
instead.

The alias template `aligned_type` is effectively a metafunction that returns a
scalar type if possible, otherwise `aligned_object_storage`. Its use in this
specification allows `pmr::resource_adaptor` to work with minimalist
allocators, including ones that can be rebound only for scalar types.  For
over-aligned values, it uses `aligned_object_storage`; for this reason,
`aligned_type` is put in the same header as `aligned_object_storage`.

The `pmr::resource_adaptor` wraps an object having a type that meets the
allocator requirements. Its `do_allocate` virtual member function supplies
aligned memory by invoking the `allocate` member function on the wrapped
allocator. The only way to supply alignment information to the wrapped
allocator is to rebind it for a `value_type` having the desired alignment but,
because the alignment is specified to `pmr::resource_adaptor::allocate` at run
time, the implementation must rebind its allocator for every possible alignment
and dynamically choose the correct one. In order to keep the number of such
rebound instantiations manageable and reduce the requirements on the allocator
type, an upper limit (default `max_align_v`) can be specified when
instantiating `pmr::resource_adaptor`. This recent change was made after
discussion with members of LWG, and with their encouragement.


Impact on the standard
======================

`pmr::resource_adaptor` is a pure library extension requiring no changes to
the core language nor to any existing classes in the standard library. A couple
of general-purpose templates (`aligned_type` and `aligned_object_storage`) are
also added as pure library extensions.

Implementation Experience
=========================

A full implementation of the current proposal can be found in GitHub at
https://github.com/phalpern/WG21-halpern/tree/P1083/resource_adaptor.

The version described in the Library Fundamentals TS has been implemented by
multiple vendors in the `std::experimental::pmr` namespace.


Formal Wording
==============

_This proposal is based on the Library Fundamentals TS v2,
[N4617](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/n4617.pdf) and
the October 2021 draft of the C++ WP,
[N4901](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2021/n4901.pdf).

_In section 17.2.1 [cstddef.syn] of the C++WP, add the following definition
sometime after the declaration of `max_align_t` in header `<cstddef>`:_

    constexpr size_t max_align_v = alignof(max_align_t);

_In section 20.10.2 [memory.syn], add the following declarations to `<memory>`
(probably near the top):_

    template <size_t Align, size_t Sz> struct aligned_object_storage;
    template <size_t Align> using aligned_type = see below;

_Prior to section 20.10.3, add the description of these new templates:_

**20.10.x Aligned storage [aligned.storage]

**20.10.x.1 Raw aligned storage [aligned.storage.raw]**

**20.10.x.1.1 struct template `raw_aligned_storage` [aligned.storage.raw.class]**

```cpp
namespace std {
  template <size_t Align, size_t Sz = Align>
  struct raw_aligned_storage
  {
    static constexpr size_t alignment = Align;
    static constexpr size_t size      = (Sz + Align - 1) & ~(Align - 1);

    using type = raw_aligned_storage;

    constexpr       void* data()       noexcept;
    constexpr const void* data() const noexcept;

    alignas(alignment) byte buffer[size];
  };
}
```

> _Mandates_: `Align` is a power of 2, `Sz > 0`

An instantiation of template `raw_aligned_storage` is a standard-layout trivial
type that provides storage having the specified alignment and size, where the
size is rounded up to the nearest multiple of `Align`.

**20.10.x.1.1 Public member functions [aligned.storage.raw.mem]**

constexpr       void* data()       noexcept;
constexpr const void* data() const noexcept;

> _Returns_: `buffer`.

**20.10.x.2 Aligned type [aligned.type]**

`template <size_t Align> using aligned_type =` _see below_;

> _Mandates_: `Align` is a power of 2.

If a scalar type, `T` exists such that `alignof(T) == Align` and `sizeof(T) ==
Align`, then `aligned_type<Align>` is an alias for `T`; otherwise, it is an
alias for `raw_aligned_storage<Align, Align>`. If more than one scalar matches
the requirements for `T`, the implementation shall choose the same one for
every instantiation of `aligned_type` for a given alignment.

_In section 20.12.1 [mem.res.syn], add the following declaration
immediately after the declaration of
`operator!=(const polymorphic_allocator...)`:_

    // 19.12.x resource adaptor
    // The name resource-adaptor-imp is for exposition only.
    template <class Allocator> class resource-adaptor-imp;

    template <class Allocator>
      using resource_adaptor = resource-adaptor-imp<
        typename allocator_traits<Allocator>::template rebind_alloc<byte>>;

_Insert between sections 19.12.3 [mem.poly.allocator.class] and 19.12.4
[mem.res.global] of the C++ WP, the following section, taken with
modifications from section 8.7 of the LFTS v2:_

**19.12.x Alias template resource_adaptor [memory.resource.adaptor]**

**19.12.x.1 `resource_adaptor` [memory.resource.adaptor.overview]**

An instance of `resource_adaptor<Allocator>` is an adaptor that wraps a
`memory_resource` interface around `Allocator`.  `resource_adaptor<X<T>>` and
`resource_adaptor<X<U>>` are the same type for any allocator template `X` and
types `T` and `U`. In addition to the _Cpp17Allocator_ requirements
(§15.5.3.5), the `Allocator` parameter to `resource_adaptor` shall meet the
following additional requirements:

 - `typename allocator_traits<Allocator>::pointer` shall denote the type
   `allocator_traits< Allocator>::value_type*`.

 - `typename allocator_traits<Allocator>::const_pointer` shall denote the type
    to `allocator_traits< Allocator>::value_type const*`.

 - `typename allocator_traits<Allocator>::void_pointer` shall denote the type
   `void*`.

 - `typename allocator_traits<Allocator>::const_void_pointer` shall denote the
    type `void const*`.

 - Calls to
   `allocator_traits<Allocator>::template rebind_traits<T>::allocate` and
   `allocator_traits< Allocator>::template rebind_traits<T>::deallocate`
   shall be well-formed for all non-class, non-over-aligned types `T`;
   no diagnostic required.

```
// The name resource-adaptor-imp is for exposition only.
template <class Allocator>
class resource-adaptor-imp : public memory_resource {
  Allocator m_alloc; // exposition only

public:
  using allocator_type = Allocator;

  resource-adaptor-imp() = default;
  resource-adaptor-imp(const resource-adaptor-imp&) noexcept = default;
  resource-adaptor-imp(resource-adaptor-imp&&) noexcept = default;

  explicit resource-adaptor-imp(const Allocator& a2) noexcept;
  explicit resource-adaptor-imp(Allocator&& a2) noexcept;

  resource-adaptor-imp& operator=(const resource-adaptor-imp&) = default;

  allocator_type get_allocator() const { return m_alloc; }

protected:
  void* do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void* p, size_t bytes, size_t alignment) override;
  bool do_is_equal(const memory_resource& other) const noexcept override;
};
```

**19.12.x.2 `resource-adaptor-imp` constructors [memory.resource.adaptor.ctor]**

`explicit resource-adaptor-imp(const Allocator& a2) noexcept;`

> _Effects_: Initializes `m_alloc` with `a2`.

`explicit resource-adaptor-imp(Allocator&& a2) noexcept;`

> _Effects_: Initializes `m_alloc` with `std::move(a2)`.

**19.12.x.3 `resource-adaptor-imp` member functions [memory.resource.adaptor.mem]**

`void* do_allocate(size_t bytes, size_t alignment);`

> _Preconditions:_ `alignment` is a power of two.

> _Returns:_
> `allocator_traits<Allocator>::rebind_alloc<U>(m_alloc).allocate(n)`, where
> `U` is a type having the specified `alignment`, limited to an
> implementation-defined maximum alignment not less than
> `hardware_destructive_interference_size` and `n` is `bytes` divided by
> `alignof(U)`, rounded up to the nearest integral value.

> _Throws:_ nothing unless the underlying allocator throws.

`void do_deallocate(void* p, size_t bytes, size_t alignment);`

> _Preconditions_: `p` has been returned from a prior call to `allocate(bytes,
> alignment)` on a memory resource equal to `*this`, and the storage at `p`
> has not yet been deallocated.

> _Effects:_ Let `U` be a type having the specified `alignment`, limited to an
> implementation-defined maximum alignment not less than
> `hardware_destructive_interference_size` and let `n` be `bytes / alignof(U)`,
> rounded up to the nearest integral value; Invoke
> `allocator_traits<Allocator>::rebind_alloc<U>(m_alloc).deallocate(p, n)`

`bool do_is_equal(const memory_resource& other) const noexcept;`

> Let `p` be `dynamic_cast<const resource-adaptor-imp*>(&other)`.

> _Returns_: `false` if `p` is null; otherwise the value of `m_alloc == p->m_alloc`.

References
==========

[N4810](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2019/n4810.pdf);
Working Draft, Standard for Programming Language C++, Richard Smith, editor,
2019-03-15.

[N4617](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/n4617.pdf):
Programming Languages - C++ Extensions for Library Fundamentals,
Version 2, 2016-11-28.

[P0339](http://wg21.link/p0339):
polymorphic_allocator<> as a vocabulary type, Pablo Halpern, 2018-04-02.

[P0987](http://wg21.link/p0987):
polymorphic_allocator<byte> instead of type-erasure, Pablo Halpern,
2018-04-02.

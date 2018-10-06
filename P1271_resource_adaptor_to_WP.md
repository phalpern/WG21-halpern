% P1271r0 | Move resource_adaptor from Library TS to the C++ WP
% Pablo Halpern <phalpern@halpernwightsoftware.com>
% 2018-10-05 | Target audience: Library Evolution

Abstract
========

When the polymorphic allocator infrastructure was moved from the Library
Fundamentals TS to the C++17 working draft, `pmr::resource_adaptor` was left
behind. It is not clear whether this was an omission or a deliberately
conservative move, but the absence of `resource_adaptor` in the standard is a
hole that must be plugged for a smooth transition to the ubiquitous use of
`polymorphic_allocator`, as proposed in [P0339](http://wg21.link/p0339) and
[P0987](http://wg21.link/p0987).  This paper proposes that
`pmr::resource_adaptor` be moved from the LFTS and added to the C++20 working
draft.

Motivation
==========

It is expected that more and more classes, especially those that would not
otherwise be templates, will use `pmr::polymorphic_allocator<byte>` to
allocate memory. In order to pass an allocator to one of these classes, the
allocator must either already be a polymorphic allocator, or must be adapted
from a non-polymorphic allocator.  The process of adaptation is facilitated by
`pmr::resource_adaptor`, which is a simple class template, has been in the
LFTS for a long time, and has been fully implemented. It is therefore a
low-risk, high-benefit component to add to the C++ WP.

Impact on the standard
======================

`pmr::resource_adaptor` is a pure library extension requiring no changes to
the core language nor to any existing classes in the standard library.

Proposal
========

This proposal is based on the Library Fundamentals TS v2,
[N4617](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/n4617.pdf) and
the July 2018 draft of the C++ WP,
[N4762](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/n4762.pdf).

In section 19.12.1 [mem.res.syn] of the C++ WD, add the following declaration
immediately after the declaration of
`operator!=(const polymorphic_allocator...)`:

    // 19.12.x resource adaptor
    // The name resource_adaptor_imp is for exposition only.
    template <class Allocator> class resource_adaptor_imp;

    template <class Allocator>
      using resource_adaptor = resource_adaptor_imp<
        typename allocator_traits<Allocator>::template rebind_alloc<byte>>;

Insert between sections 19.12.3 [mem.poly.allocator.class] and 19.12.4
[mem.res.global] of the C++ WP, the whole of section 8.7
[memory.resource.adaptor] from the LFTS v2, including all of its subsections,
renumbered appropriately.

References
==========

[N4762](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/n4762.pdf):
Working Draft, Standard for Programming Language C++, Richard Smith, editor,
2018-07-07.

[N4617](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/n4617.pdf):
Programming Languages - C++ Extensions for Library Fundamentals,
Version 2, 2016-11-28.

[P0339](http://wg21.link/p0339):
polymorphic_allocator<> as a vocabulary type, Pablo Halpern, 2018-04-02.

[P0987](http://wg21.link/p0987):
polymorphic_allocator<byte> instead of type-erasure, Pablo Halpern,
2018-04-02.

---
title: "Side Channels: Hierarchical Overview of Three Proposals"
document: none
date: <!-- $TimeStamp$ -->2025-05-30 15:32 EDT<!-- $ -->
audience: Internal
author:
  - name: Pablo Halpern
    email: <phalpern@halpernwightsoftware.com>
toc: true
toc-depth: 2
---

Introduction
============

Discussions of language support for allocators yielded three parts that
can be proposed separately, where each part is dependent on the part
before:

-   **Viral class properties** that cause extra "parameters" to be added
    automatically to every constructor. We put "parameters" in quotes
    because the original concept was that these would not be traditional
    parameters but some other parameter-like side channel.

-   **Special side-channel argument values** for defaulted parameters
    that automatically determine their value from the surrounding
    context, e.g., if no explicit value is provided for the allocator
    parameter in a member constructor, it should get its value from the
    surrounding class's allocator argument.

-   A syntax for passing arguments that bind to these special
    parameters. After some discussion, we determined that **named
    parameters** would be the most straightforward way and the easiest
    proposal to sell to the committee.

The last feature needs to be completed before the one above, which in
turn needs to be completed before the first feature. Each feature is
briefly described below.

Viral Class Properties
======================

The top-level goal that started this process was to provide allocator
support in the C++ language using a mechanism that would be generalized
to other cross-cutting concerns such as logging and execution contexts.
We want to minimize the amount of boilerplate code needed to pass
allocators (and loggers and execution contexts) around. Ideally,
allocator-aware classes could follow the rule of zero, with the compiler
automatically generating constructors that allow allocators to be passed
in. A single annotation on a low-level type would indicate that such
type is allocator-aware. The property established by the annotation
would be viral such that a class having a member or base class of an
allocator-aware type would automatically be allocator aware itself, and
thus have the appropriate allocator-passing mechanism available on every
constructor. For example,

```cpp
class string @**allocator_aware**@ {
 public:
  // All constructors implicitly have a mechanism for being passed an allocator.
  string();
  string(const char *s);
  string(const string&);
  string(string&&);
  // ...
};

// Foo is automatically @**allocator-aware**@ because it has a @**string**@ member.
struct Foo {
  string name;
  int value;
};
```

Note that Foo uses the rule of zero, yet the automatically generated
constructors would still accept an allocator parameter. More than almost
anything else, achieving the ability to apply to rule of zero to
allocator-aware types is our fundamental goal.

Special Side-Channel Argument Values for Defaulted Parameters
=============================================================

In the specific case of allocators, boilerplate code could be reduced if
an object's constructor automatically passed its allocator argument to
its allocator-aware subobjects. More generally, it is an invariant
quality of certain arguments that, in a function-call hierarchy, they
should be automatically propagated from caller to callee. We have
tentatively dubbed the arguments that participate in this automatic and
invisible propagation *side-channel arguments*. When a value is not
explicitly provided for a side channel, the side-channel argument should
get its value by means of a mechanism that runs in the calling context, and
which can reflect on that context. Such reflection might need to
determine whether the parameter is for a constructor, whether that
constructor is for a full object or a subobject, static vs. automatic
storage duration, and/or whether the object is the return object of the
calling function.

Aside from allocators, such side-channel initialization would be useful
for things like execution contexts and dynamically scoped variables,
often replacing the use of global or thread-local variables, which make
code difficult to reason about, can be inefficient, and can produce
incorrect results in some cases (e.g., coroutines and other parallel agents
that are distinct from threads).

We do not yet have even a straw-man syntax for these special
initializers.

Named Parameters
================

Until recently (May 2025), we thought that side channel arguments would
be supplied by a syntax outside of the argument list of a function,
utilizing the using keyword:

```cpp
String hi("hello") using MyAllocator;
```

The advantage of this syntax is that the (usually optional) side-channel
does not pollute the argument list, obviating overload-disambiguation
tricks such as `allocator_arg_t`. After pursuing this approach for some
time, it became clear to us that this syntax could be used as a hacky
way to get the benefits of named parameters and that named parameters
would be a more natural way to express the intent:

```cpp
String hi("hello", .allocator = MyAllocator);
```

Unlike a regular (positional) parameter, a named parameter can be added
to any function, even one that has default arguments or a variadic
parameter list, without creating overloading or call-site ambiguities:

```cpp
// Without named allocator argument
template <class... Args>
string foo(int i=0, std::pmr::polymorphic_allocator<> alloc={});

// With named allocator argument (straw-man syntax)
template <class... Args>
string bar(int i=0, std::pmr::polymorphic_allocator<> .allocator={});

auto s1 = foo();           // OK: i = 0, alloc = {}
auto s2 = foo(3);          // OK: i = 3, alloc = {}
auto s3 = foo(3, MyAlloc); // OK: i = 3, alloc = MyAlloc
auto s4 = foo(MyAlloc);    // Error: No match

auto s5 = bar();                      // OK: i = 0, alloc = {}
auto s6 = bar(3);                     // OK: i = 3, alloc = {}
auto s7 = bar(3, .allocator=MyAlloc); // OK: i = 3, allocator = MyAlloc
auto s8 = bar(.allocator = MyAlloc);  // OK: i = 0, allocator = MyAlloc
```

People have wanted to add named parameters to C++ for a long time, but
we are not aware of a concerted effort to overcome the complexities of
mixing named and positional arguments, overload resolution, pack
expansion, pack indexing, and declaration/definition concurrence. We
believe that we have a viable starting point that involves baking the
named parameters into the function type, but we will need to do some
research into the previous failed efforts to avoid falling into a trap.
If we can overcome the technical obstacles, we think that WG21 will be
very interested in adding this feature to C++.

Road Map
========

The viral-property proposal is dependent on the other two and the
side-channel-argument proposal is dependent on the named-parameter
proposal. We should, therefore, pursue named parameters first, then
side-channel arguments, then viral-properties. This order is not only
correct from a dependency standpoint but also starts from the most
general proposal to the least general one, thus maximizing the chances
of success. By the time we are ready to tackle viral annotations, the
code-generation aspects of reflection are likely to be more developed,
so it might even be possible for this feature to be largely or
completely implemented as a library feature.

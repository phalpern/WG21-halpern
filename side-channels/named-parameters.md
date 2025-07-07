---
title: "Named Parameters"
document: named-parameters
date: <!-- $TimeStamp$ -->2025-07-05 18:10 EDT<!-- $ -->
audience: LEWGI
author:
  - name: Pablo Halpern
    email: <phalpern@halpernwightsoftware.com>
working-draft: N5008
toc: true
toc-depth: 2
---

# Abstract

This document contains Pablo's Musings on named parameters.

# Very Short Summary

This feature would use a function-parameter declaration syntax similar to that
of designated initializers. A function would be declared something like this:

```cpp
template <class Shape, class Transform>
Shape munge(const Shape&     shape,
            const Transform& .transform,
            double           .scale     = 1.0,
            const Point&     .translate = {0.0, 0.0}
```

The `munge` function would be called like this:

```cpp
// Munge `myShape` using the specified lambda and scale by 1.5. The translation
// is not specifed, so the default translation is used.
auto newShape = munge(myShape,
                      .transform = [](Shape& s){ ... },
                      .scale     = 1.5);
```

The syntax used in this document is far from fully baked. For the purposes of
this document we declare a named parameter by prefixing it's name with a dot
(`.`), a.k.a., a period. The dot followed by the parameter name is called the
*designator* for the parameter. When passing arguments to a function, we
associate an argument value with a named parameter by preceding the argument
with the parameter designator and the assignment operator (`.name = arg`). The
argument-passing syntax resembles that of designated initializers for
aggregates ([dcl.init.aggr]{.sref}).


# Designated and Positional Parameter Properties

We introduce two new Boolean properties for function parameters:

- **Designated** - Indicate that the parameter's name applies not only to a
  local variable declaration, but is used to identify the matching argument at
  the call site. _Named_ rolls off the tongue more easily, but does not
  distinguish between local names and semantically significant designators.
- **Positional** - Indicates that the position of the parameter in the overall
  list of parameters is meaningful. All C++26 parameters are positional.

In this proposal, a function parameter can be

1. Undesignated and positional (only option in C++26)
2. Designated and nonpositional
3. Designated and positional

Josh is not sure that the third combination is needed for a successful feature,
but Pablo is convinced that it is needed to simplify wide adoption in existing
libraries. For example, `std::basic_string` has at least 17 constructors, many
of which cry out for designated parameters. It be much more palatable to adapt
most of the existing constructors rather than to create new ones. For example,

::: cmptable

### Before

```cpp
constexpr explicit basic_string(const Allocator& a) noexcept;
```

### After

```cpp
constexpr explicit basic_string(const Allocator& .allocator) noexcept;
```

---

```cpp
constexpr basic_string(const basic_string&);
constexpr basic_string(const basic_string& str, size_type pos,
                       const Allocator& a = Allocator());
constexpr basic_string(const basic_string& str, size_type pos, size_type n,
                       const Allocator& a = Allocator());
constexpr basic_string(const basic_string&, const Allocator&);
```

```cpp
constexpr basic_string(const basic_string& str,
                       size_type           .pos       = 0,
                       size_type           .n         = max_size(),
                       const Allocator&    .allocator = Allocator());
constexpr basic_string(const basic_string& str, size_type pos, const Allocator&);
constexpr basic_string(const basic_string&, const Allocator&);
```

---

```cpp
constexpr basic_string(const charT* s, size_type n,
                       const Allocator& a = Allocator());
constexpr basic_string(const charT* s, const Allocator& a = Allocator());
```

```cpp
constexpr basic_string(const charT*     s,
                       size_type        .n         = 0,
                       const Allocator& .allocator = {});
constexpr basic_string(const charT* s, const Allocator& a);
```
:::

If the `.pos`, `.n`, and `.allocator` designated parameters are also
positional, the second column is both simpler to specify and more expressive
than the first, while being fully API compatible. Without designated positional
parameters, the existing constructors would remain unchanged and an additional
constructor would be added for each group of related existing ones.

For now, we will explore a design space that includes designated positional
parameters, so that we can find the difficulties and possible was of mitigating
them. A straw-man syntax for separating positional designated parameters from
nonpositional designated parameters within a declaration is a single dot
psuedo-argument:

```cpp
// `func` has positional designated parameters `x` and `y` and
// nonpositional designated parameters `a` and `b`.
void func(int .x, int .y, ., float .a, std::string .b = {});
```

# Parameter Declaration Order

Parameters are grouped in a function declaration in this order (any or all
groups might be empty):

1. Undesignated parameters without default argument values
2. Designated positional parameters without default argument values
3. Designated and undesignated positional parameters with default argument values
4. An undesignated parameter pack
5. The nonpositional parameter separator (tentatively `.`) followed by designated
   nonpositional parameters.

Examples:

```cpp
void f1(int x, int y = 0, ., double .scale = 1.0);

template <class... V>
void f2(int x, int .y = 0, int z = 0, V&&...v, ., int .origin, int .offset = 0);
```

# Argument Specification Order

When calling a function with designated parameters, the following rules apply.

1. An argument must be supplied for every positional parameter that doesn't
   have a default argument value.
2. Arguments with designators must come after arguments without
   designators.
3. Arguments with designators, whether positional or not, may appear in any
   order.

The following function-call examples assume the definitions of `f1` and `f2`
from the previous section.

```cpp
f1(9, 8, .scale = 2.0);  // All arguments are explicitly specified
f1(9, .scale = 1.5);     // y = 0 by default
f1(9, 8);                // scale = 1.0 by default
f1(9, .scale = .5, 8);   // Ill formed: undesignated arg after designated arg

// x = 1, y = 2, z = 3, v...[0] = 'd', v...[1] = 0.4, origin = 0, offset = 0
f2(1, 2, 3, 'd', 0.4, .origin = 0);

// x = 1, y = 2, z = 0, sizeof...(v) = 0, origin = 0, offset = 1
f2(1, .origin = 0, .y = 2, .offset = 1);

f2(1, 2, 3, 'd', 0.4);  // Ill-formed: no value specified for origin

// Ill-formed: undesignated arg after designated arg
f2(1, .y = 2, 3, 'd', 0.4, .origin = 0);
```

# Function Signatures, Virtual functions, and Overloading

For two functions to have the same signature, they must follow all of the C++26
rules as well as the following:

* The types of the positional parameters appear in the same order, irrespective
  of whether they are designated.
* Designators for positional parameters are the same and appear in the same
  positions relative to the overall list of the positional parameters.
* The nonpositional designated parameters have the same types and designators
  (but do not necessarily appear in the same order).

Two functions in the same scope having the same name and signature refer to the
same function. The usual ill-formed rule applies if they have different return
types.

A function in a derived class having the same name and signature as a virtual
function in one of it's base classes overrides that virtual function.  The
usual ill-formed rule applies if the overriding function's return type is not
covariant with respect to the base class function's.

Multiple functions in the same scope having different signatures form an
overload set. Calling one of these functions with a particular set of
positional and designated arguments can result in an overload ambiguity. The
ambiguity manifests at the call site, not at the point where these functions
are declared:

```
class Stream
{
public:
  enum class Mode { READ, WRITE, APPEND };

  ...
  // Both overloads take two designated positional arguments
  int open(Mode .mode, const std::string .filename);
  int open(Mode .mode, const std::string .url);
  ...
};

void f()
{
  Stream s;
  s.open(Stream::Mode::READ, .filename = "myinput.txt");    // OK
  s.open(Stream::Mode::READ, "https::/isocpp.org/myinput"); // Error: ambiguous
}
```

# Interaction with Overloading

If two functions in the same scope have the same name but different
prototypes,

# Interaction with Function Pointers and the Type System

Designated parameters are encoded in the type system.

# Interaction with Parameter Packs

# Possible enhancements

- Override arguments for replacing elements of a parameter pack

- Ignorable named arguments for function calls that might not have such a
   named parameter.

# ABI Considerations

---
title: "Named Parameters"
document: named-parameters
# $TimeStamp$
date: 2025-08-08 18:30 EDT
# $
audience: LEWGI
author:
  - name: Pablo Halpern
    email: <phalpern@halpernwightsoftware.com>
  - name: Joshua Berne
    email: <jberne4@bloomberg.net>
working-draft: N5008
toc: true
toc-depth: 2
---

# Abstract

Naming specific arguments at a function call site is supported in a number of
popular programming languages, including Python, Swift, C#, Ada, and
JavaScript, among others. This document describes how C++ can benefit from
adopting a syntax for designating named parameters, provides a number of
motivating use cases, and provides a proposed syntax and semantics for the
proposed new named-parameter feature. Though formal wording is not proposed at
this time, we thoroughly explores how the new feature would interact
with other aspects of C++, including overload resolution and the type
system. We also explore why previous proposals in this area have failed and how
this proposal overcomes the deficiencies of those earlier proposals. The
proposal is in the exploratory phase and its audience is EWGI.

Note to early reviewers: This document started out as a collection of Pablo's
musings but turned into something that is well on the way to being a complete
proposal.

# Motivation

Existing languages that provide the ability to name arguments at a function
call site (e.g., Python, Swift, and Ada), benefit from easier to read
(self-documenting) function calls, resistance to certain common errors (such as
out-of-order arguments), and easier-to-evolve function interfaces. Along with
the benefits enjoyed by other languages, named parameters would benefit C++ in
ways peculiar to its syntactic weaknesses, such as potential overload
ambiguities and constraints on default- and variadic-argument
ordering. Additionally, named parameters would make C++ more expressive by
expanding the range of potential overloads and supporting cleaner idioms for
generic programming.  Each of these benefits is described in detail in the
[Motivating Examples](#motivating-examples) section, below.

In the absence of named parameters, programmers have devised a number of
library-only techniques attempting to get some of the benefits of named
parameters in C++, especially with respect to making calls easier to read, less
error prone, and/or making the interface easier to evolve. These techniques are
ad-hoc, inconsistent with one another, and none of them look like normal
parameter declarations. None of them allow arguments to be specified in an
arbitrary order. Two common techniques, designated aggregate initializers and
tagged parameters, are described below, along with their shortcomings:

## Workaround: Designated Aggregate Initializers

### Description

This technique uses a `struct` to aggregate a function's parameters. Rather
than having _n_ parameters, the function has a single parameter of a special
`struct` type having _n_ members:

```cpp
// function-argument struct
struct MakeWindowParams {
  int width;
  int height;
  int left         = INT_MIN;
  int top          = INT_MIN;
  Color background = Color::white;
  Color foreground = Color::black;
};

// function prototype
WindowHandle = MakeWindow(MakeWindowParams&& args);

// function call
auto win = MakeWindow({ .width = wd, .height = ht, .foreground = fg });
```

### Shortcomings

- Every function potentially needs a custom struct.
- The caller cannot provide arguments other than in declared order although
  [@P3405R0] would change that, if adopted.
- Does not work with template argument deduction.
- For default-constructible parameter types, there is no way to force the
  caller to supply a value. In the code above, there is no way to force the
  caller to specify width and height, for example.


## Workaround: Tagged Arguments

### Description

This technique identifies an argument by preceding it with a _tag_ having a
unique mono-value type whose sole purpose is specifying the desired argument:

```cpp
namespace std {

// tag type and tag variable (@[allocator.tag]{.sref}@)
struct allocator_arg_t { explicit allocator_arg_t() = default; };
inline constexpr allocator_arg_t allocator_arg{};

// Owned object type (@[indirect.syn]{.sref}@)
template <class T, class Allocator = allocator<T>>
class indirect
{
public:
  // Construct with an optional allocator.
  constexpr explicit indirect();
  constexpr explicit indirect(allocator_arg_t, const Allocator& a);
  template<class U = T>
    constexpr explicit indirect(U&& u);
  template<class U = T>
    constexpr explicit indirect(allocator_arg_t, const Allocator& a, U&& u);
  //...
};

}
```

The above example is taken directly from the C++ Standard Library. In the
absence of the `allocator_arg_t` tag, the third constructor overload could be
selected when the second was intended. An `indirect` object constructed using
the second constructor would look as follows.

```cpp
std::indirect<int, AllocType> x(std::allocator_arg, myAllocator);
```

### Shortcomings

- Each "named" parameter requires a separate tag, typically in namespace
  scope; it is not practical to create one for every argument in a significant
  library.
- The call syntax is not pretty; it is not immediately obvious what is going
  on.
- Realistically, this technique does not try to emulate named parameters, but
  it is used as a workaround for their absence.


# A Very Short Summary of the Proposal

Before getting to the motivating use cases, it is helpful to have a general
sense of what is being proposed. The syntax described here is used in
comparison tables within the [Motivating Use Cases](#motivating-use-cases)
section.

The proposed feature would use a function-parameter declaration syntax similar
to that of designated initializers. A function would be declared something like
this:

```cpp
template <class Shape, class Transform>
Shape munge(const Shape&     shape,
            const Transform& .transform,
            .,
            double           .scale     = 1.0,
            const Point&     .translate = {0.0, 0.0});
```

The single dot psuedo-parameter is used to separate positional parameters form
non-positional parameters. Non-positional arguments can be specified only using
the named-argument syntax whereas positional arguments can be specified with or
without a name.

The `munge` function would be called like this:

```cpp
// Munge `myShape` using the specified lambda and scale by 1.5. The translation
// is not specifed, so the default translation is used.
auto newShape = munge(myShape,
                      .transform = [](Shape& s){ ... },
                      .scale     = 1.5);
```

The syntax used in this document is not from fully baked. For the purposes of
this document we declare a named parameter by prefixing it's name with a dot
(`.`), a.k.a., a period. The dot followed by the parameter name is called the
*designator* for the parameter. When passing arguments to a function, we
associate an argument value with a named parameter by preceding the argument
with the parameter designator and the assignment operator (`.name = arg`). The
argument-passing syntax resembles that of designated initializers for
aggregates ([dcl.init.aggr]{.sref}).


# Motivating Use Cases

Consider a `makeWindow` function that creates a rectangular user-interface
window. Of its 6 parameters, only the width and height are required. If the
top-left is not specified, the window manager will place the window on the
screen according to some algorithm. If the background and foreground colors are
not specified, white and black are used, respectively:

```cpp
WinHandle makeWindow(int width,                       int height,
                     int left         = INT_MIN,      int top = INT_MIN,
                     Color background = Color::white, Color foreground = Color::black);
```

Although some might consider the number of parameters to be excessive, it is
borderline at worst, and it quite a straightforward interface. Nevertheless, it
is inconvenient and error prone to use. A number of problems with this
interface are detailed below, followed by a description of how each is solved
using named parameters having the syntax proposed in this paper.

## Less Error Prone

The first 4 parameters all have type `int`. It is thus very easy to supply
arguments in the wrong order or even assume the wrong meaning:

::: cmptable

### Without Named Parameters

```cpp
int top   = 100,   left  = 150;
int height = 1100, width = 1000;

// BUG, but compiles
auto w = makeWindow(top, left, height, width);
```

### With Named Parameters

```cpp
int top    = 100,  left  = 150;
int height = 1100, width = 1000;

// OK
auto w = makeWindow(.top=top,       .left=left,
                    .height=height, .width=width);
```

---

```cpp
int top    = 100,  left  = 150;
int bottom = 1100, right = 1250;

// BUG, but compiles
auto w = makeWindow(top, left, bottom, right);
```

```cpp
int top    = 100,  left = 150;
int bottom = 1100, right = 1250;

// Error: No such parameters `.bottom` and `.right`
auto w = makeWindow(.top=top,       .left=left,
                    .bottom=bottom, .right=right);
```
:::

For some functions, the "obvious" order of arguments is not the correct
order. In the first case, we often talk about the "top-left corner", but
traditionally the x coordinate (left) comes first.  Moreover it is
counterintuitive to put the width and height before the top and left, but the
author of `makeWindow` had no choice because mandatory arguments must always
come before optional ones. The named arguments can appear in any order, so the
version on the right yields the expected result.

In the second case, the arguments are not only in the wrong order, but are
ascribed the wrong meaning. Again, the code looks correct and compiles
successfully, but result is not what is desired. The version on the right will
flag the error at compile time, as the misunderstanding results in a mismatch
between the argument name and parameter name.

## More Convenient

Default arguments are a convenience feature, but to supply a non-default value
for one parameter, the caller must provide a non-default value for all
preceding parameters:

::: cmptable

### Without Named Parameters

```cpp
auto w = makeWindow(w, h, INT_MIN, INT_MIN, Color::grey50);
```

### With Named Parameters

```cpp
auto w = makeWindow(w, h, .foreground = Color::grey50);
```

:::

In the example above, the `INT_MIN` arguments on the left are pure noise and
make the call harder to write and read. The code on the right is clean and
minimal.

## Self Documenting

Most code is read more often than it's written. Reading code, e.g., during a
code review, is more convenient and has a lower cognitive load when you don't
need to look up the parameter list for a function.

::: cmptable

### Without Named Parameters

```cpp
// What are all those numbers?
auto w = makeWindow(1000, 1500, 10, 20);
```

### With Named Parameters

```cpp
// Obvious meaning
auto w = makeWindow(.width = 1000, .height = 1500,
                    .left  = 10,   .top = 20);
```

:::

Named arguments reduce the cognitive load for both reading and writing
functions having as few as two parameters, with an exponentially bigger benefit
for each additional parameter.

## Simpler Code Evolution and Fewer Overloads

Consider this function, whose parameter list ends with a variadic parameter
pack:

```cpp
template <class T = int, class... Args>
void calculate(T& x = {}, Args&&... args);
```

A later version of the library adds the ability to specify an optional
`ThreadPool` object that `calculate` can use to speed up computation. There is
no place to put the `ThreadPool` parameter without breaking the API, either in
the original function or by adding an overload. A solution used in a few places
in the standard library is to create a tag type that precedes the `ThreadPool`
parameter, where the tag type is expected never to be used for any other
purpose. In effect, the tag type is a poor emulation of named parameters and
necessitates a new overload for the updated function. In contrast, using real
named parameters, we can simply add the `ThreadPool` parameter on the existing
function:

::: cmptable

### Without Named Parameters

```cpp
// Define tag type
struct ThreadPool_arg_t {
  explicit ThreadPool_arg_t() = default;
};
inline constexpr ThreadPool_arg_t ThreadPool_arg{};

template <class T = double, class... Args>
T calculate(T& x = {}, Args&&... args);

template <class T = double, class... Args>
T calculate(ThreadPool_arg_t, ThreadPool& threadpool,
            T& x = {}, Args&&... args);

auto r1 = calculate(19.4, "encode", 1.0);
auto r2 = calculate(ThreadPool_arg, myThreadPool,
                    19.4, "encode", 1.0);
```

### With Named Parameters

```cpp









template <class T = double, class... Args>
T calculate(T& x = {}, Args&&... args, .,
            ThreadPool& .threadpool = defaultThreadPool);

auto r1 = calculate(19.4, "encode", 1.0);
auto r2 = calculate(19.4, "encode", 1.0,
                    .threadpool = myThreadPool);
```

:::

## More Expressive Overloading

Consider a `Point` class representing a point in 2-dimensional space. A `Point`
can be constructed using either Cartesian coordinates or polar coordinates.  In
both cases, the constructor take two parameters of type `double`. Named
parameters allow the two constructors to coexist without using a tag type:

::: cmptable

### Without Named Parameters

```cpp
struct polar_coord_t { explicit polar_coord_t() = default };
inline constexpr polar_coord_t polar_coord{};

class Point
{
public:
  constexpr Point(double x, double y);
  constexpr Point(polar_coord_t, double radius, double angle);
};

Point p1(4.0, 3.0);
Point p2(polar_coord, 5.0, 0.6435);
```

### With Named Parameters

```cpp



class Point
{
public:
  constexpr Point(double .x, double .y);
  constexpr Point(., double .radius, double .angle);
};

Point p1(4.0, 3.0); // Alternative: Point p1(.x=4.0, .y=3.0);
Point p2(.radius=5.0, .angle=0.6435);
```

:::

## More Expressive Generic Programming

Although named arguments have been proposed several times (see the
[Previous Attempts](#previous-attempts) section) promoting code-clarity and
fewer errors, the proximate reason for this proposal is for generic
programming. In particular, it is often desirable to pass a contextual argument
through multiple levels of generic code to a function or constructor that needs
it. Prime examples are allocators, logging streams, and thread
pools. Currently, passing such a parameter requires rigid parameter-order rules
that might conflict with one another. Typically a function having one of these
cross-cutting parameters must put that parameter last (which conflicts with
variadic parameters) or first, but both conventions cause conflicts if two or
more of these parameters are needed. All of those issues are obviated through
the use of named parameters:

```cpp
template <class... Args, class Allocator = std::allocator<>>
class Service {
public:
  Service(int x, Args&&... args, .,
          Logger& .logger      = global_logger(),
          Allocator .allocator = {});

  void doSomething();
};

template <class S, class... CtorArgs>
void func(CtorArgs&& ctorArgs)
{
  Logger myLogger;

  // Generically pass a logger to the `S` constructor.
  S svc(std::forward<CtorArgs>(ctorArgs)..., .logger = myLogger)
  svc.doSomething();
}
```

In the above example, the only thing `func` needs to know to pass a logger to
the `S` constructor is that the constructor takes a named parameter named
"logger." Some of the enhancements described in
[Possible Enhancements](#possible-enhancements) would make even this knowledge
unnecessary.

A planned future proposal built on top of this named-parameter proposal would
expand on this feature by allowing default arguments to get their values from
the caller's environment, replacing some current (unsafe) uses of global and
thread-local variables. This future proposal would replace variables having
global scope with specially declared parameters having global names, but not
global values.


# Nomenclature

We introduce two new terms for describing function parameters:

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
constexpr explicit
  basic_string(const Allocator& a) noexcept;
```

### After

```cpp
constexpr explicit
  basic_string(const Allocator& .allocator) noexcept;
```

---

```cpp
constexpr basic_string(const basic_string&);
constexpr basic_string(const basic_string&,
                       const Allocator&   );
constexpr basic_string(const basic_string& str,
                       size_type           pos,
                       const Allocator&    a = Allocator());
constexpr basic_string(const basic_string& str,
                       size_type           pos,
                       size_type           n,
                       const Allocator&    a = Allocator());
```

```cpp
constexpr basic_string(const basic_string&);
constexpr basic_string(const basic_string&,
                       const Allocator&    .allocator);
constexpr basic_string(const basic_string& str,
                       size_type           .pos,
                       const Allocator&    .allocator = {});
constexpr basic_string(const basic_string& str,
                       size_type           .pos,
                       size_type           .n,
                       const Allocator&    .allocator = {});
```

---

```cpp
constexpr basic_string(const charT*     s,
                       size_type        n,
                       const Allocator& a = Allocator());
constexpr basic_string(const charT*     s,
                       const Allocator& a = Allocator());
```

```cpp
constexpr basic_string(const charT*     s,
                       size_type        .n,
                       const Allocator& .allocator = {});
constexpr basic_string(const charT* s,
                       const Allocator& .allocator = {});
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
pseudo-argument:

```cpp
// `func` has positional designated parameters `x` and `y` and
// nonpositional designated parameters `a` and `b`.
void func(int .x, int .y, ., float .a, std::string .b = {});
```

# Parameter and Argument Order

Parameters in a function declaration must appear in the following order (any or
all of the following may be absent):

1. Undesignated parameters without default arguments
2. Designated positional parameters without default arguments
3. Designated and undesignated positional parameters with default arguments
4. An undesignated variadic parameter (ellipsis)
5. The nonpositional parameter separator (tentatively `.`) followed by
   designated nonpositional parameters. The order of nonpositional parameters
   is not significant.

Examples:

```cpp
void f1(int x, int y = 0, ., double .scale = 1.0);

template <class... V>
void f2(int x, int .y = 0, int z = 0, V&&...v, ., int .origin, int .offset = 0);
```

When calling a function, undesignated arguments must appear before first, in
positional order, followed by designated arguments. The relative ordering of
designated arguments is not significant.

Examples:

```cpp
f1(9, 8, .scale = 2.0);  // All arguments are explicitly specified
f1(9, .scale = 1.5);     // y = 0 by default
f1(9, 8);                // scale = 1.0 by default
f1(9, .scale = .5, 8);   // Ill formed: undesignated arg after designated arg

// x = 1, y = 2, z = 3, v...[0] = 'd', v...[1] = 0.4, origin = 0, offset = 0
f2(1, 2, 3, 'd', 0.4, .origin = 0);

// x = 1, y = 2, z = 0, sizeof...(v) = 0, origin = 0, offset = 1
f2(1, .origin = 0, .y = 2, .offset = 1);

f2(1, 2, 3, 'd', 0.4);  // Overload failure: no value specified for origin

// Ill-formed: undesignated arg after designated arg
f2(1, .y = 2, 3, 'd', 0.4, .origin = 0);
```

# Interaction with Overload Resolution

Parameter designators are part of a function's signature (and thus its type,
see [Interaction with Type System](#interaction-with-type-system)).  It is thus
possible to overload solely on designator names:

```cpp
class Stream
{
public:
  enum class Mode { READ, WRITE, APPEND };

  ...
  // distinct overloads of `open`
  int open(Mode .mode, const std::string .filename); // overload #1
  int open(Mode .mode, const std::string .url);      // overload #2
  ...
};
```

Changes in overload resolution ([over.match]{.sref}) should be
modest, mostly affecting the first step of identifying viable functions in
section [over.match.viable]{.sref}. Rather than specifying this step based on
the number of arguments, we specify an algorithm for matching each argument to
a corresponding function parameter.

For a function call having _N_ positional arguments PA~i~, for integer _i_ in
the range 1..._N_, followed by _M_ designated arguments, DA~d~, where each _d_
is a designator name (not an integer), arguments are matched to parameters as
follows.

For each positional argument, PA~i~,

- if a positional parameter PP~i~ exists, then PA~i~ matches PP~i~;
- otherwise, if there is a variadic parameter then PA~i~ matches the variadic
  parameter;
- otherwise, PA~i~ has no match.

For each designated argument, DA~d~,

- if a designated parameter DP~d~ exists, then DA~d~ matches DP~d~;
- otherwise, if the first unmatched positional parameter is a deduced template
  parameter, then DA~d~ matches that positional parameter;
- otherwise, if there is a variadic parameter then DA~d~ matches the variadic
  parameter (see
  [Interaction with Parameter Packs](#interaction-with-parameter-packs),
  below);
- otherwise, DA~d~ has no match.

To be a viable function,

- each argument must match exactly one parameter, and
- each non-variadic parameter must match exactly one argument or else match no
  arguments and have a default argument.

Note that a designated positional parameter can be matched either by position
or by designator, but if it matches more than one distinct argument, the
function would be nonviable.

The specification for the best viable function in section
[over.match.best]{.sref} would be modified to refer to the argument-parameter
matches determined by the algorithm above instead of to the *i^th^* argument
and *i^th^* parameter. Otherwise, there should be few other changes to the rest
of overload resolution section ([over.match]{.sref}).

As in C++26, overload resolution can result in an ambiguity at the call site:

```cpp
void f()
{
  Stream s;

  s.open(Stream::Mode::READ, .filename = "myinput.txt");    // OK
  s.open(Stream::Mode::READ, "https::/isocpp.org/myinput"); // Error: ambiguous
}
```

# Interaction with Parameter Packs

Consider a function template that simply preforms an action before and after
calling an invocable with supplied arguments:

```cpp
template <class F, class... Args>
auto verbose_call(F&& f, Args&&... args) -> decltype(auto) {
  std::cout << "Prologue\n";
  decltype(auto) ret = std::forward<F>(f)(std::forward<Args>(args)...);
  std::cout << "Epilogue\n";
  return ret;
}
```

It is extremely desirable that the above template, which is valid C++26,
continue to work unchanged in the world of designated parameters. Given the
following declaration and call:

```cpp
void func(int, ., const char* .name);
verbose_call(f, 15, .name="Fred");
```

We would want the invocation of f within `verbose_call` to expand as follows:

```cpp
  decltype(auto) ret = std::forward<void (&)(int, const char* .name)>(func)(
    std::forward<int>(15), .name=std::forward<const char (&&)[5]>("Fred"));
```

In the
[Interaction with Overload Resolution](interaction-with-overload-resolution)
section, above, the argument-to-parameter matching algorithm matches any
designated argument to the variadic template parameter if there is no
corresponding designated parameter.  Thus, `.name="Fred"` gets mapped to the
`args` parameter pack. The question then becomes, how is the designator
forwarded to further in the call tree?

One possible rule would be for the designator to follow the
argument, i.e., the last argument to `func` starts with `.name=` because the
designator is associated with the last argument of the parameter pack. This
rule is deficient, however, when the argument is not present, as in the case of
type traits or concepts:

```cpp
template <class F, class... Args>
  requires std::invocable<F, Args...>
auto verbose_call(F&& f, Args&&... args) -> decltype(auto) { ... }
```

To make the `invocable` concept work, `Args` must describe not only the type,
but also the designator (if any) associated with each argument at the point of
instantiation.  What we propose is to expand the type system with _designator
qualifiers_. In the expression, `verbose_call(f, 15, .name="Fred")`, the third
argument has type `const char (&)[5]`, with `.name` is its designator
qualifier. If we were to reify the designator qualifier as something that can
be uttered directly by the programmer, we might say that `decltype(.length =
5)` has type `int.length` or, more generally, that `decltype(.@_name_@ =
@_expr_@)` has type `decltype(@_expr_@).@_name_@`.  Such reification is
probably not essential, but it does make designated parameters easier to talk
about and it is useful to be able to designator-qualified type directly, e.g.,
to store an argument list in a `tuple` (`tuple<int.x, int.y>`), to express a
type trait (`is_constructible_v<T, float.name>`), etc.

There are no conversions between a type and its designator-qualified
variant. Given an expression `e` of type `T`, a function argument expressed as
`.d=e` will have type `T.d`.  Within the function, conversely, the variable `d`
will have type `T` (without the designator). The only implicit conversion-like
binding is that an expression without a designator can bind to a positional
parameter with a designator. A designator can also be stripped from an
expression or isolated from a parameter pack through idiomatic uses of lambdas:

```cpp
template <class... Args>
void f(Args&&... args)
{
  auto getX = [](auto&&..., auto .X) -> decltype(auto) {
    return X;
  }

  auto X = getX(std::forward<Args>(args)...);
}
```

The semantics of many uses of designated parameters and designated arguments
fall out naturally through the introduction of designator-qualified types and
obviate special-case handling in the Core Language, the Standard Library, and
in implementations.  Most generic code will continue to yield expected results
without modification.  For example:

```cpp
// `forward_as_tuple` returns a `tuple` instantiated with two designator-qualified types.
// `p.second` is initialized with arguments `.value=x, .offset=-1`.
std::pair<int, W> p(std::piecewise_construct, 99
                    std::forward_as_tuple(.value=x, .offset=-1));

// Yields `tuple<const char *, int.angle, double.radius>`
auto t = std::make_tuple("hello", .angle=30, .radius=2.5);

// Invokes `func("hello", .angle=30, .radius=2.5)`
apply(func, t);
```

# Interaction with Type System

Designated parameters are encoded in the type system.

# Interaction with Reflection

# Possible Enhancements

- Override arguments for replacing elements of a parameter pack

- Ignorable named arguments for function calls that might not have such a
   named parameter.

- Variadic designated parameters

- Named template parameters

# Impact on the Standard Library

# ABI Considerations

# Alternative Syntax and Terminology

Use the term *label* instead of *designator* (similarly *labeled parameter*,
*labeled argument*, *label-qualified type*).

`void func(T t:, U (&f:)());` instead of `void func(T .t, U (&.f)());` and
`func(t : expr1, f : expr2)` instead of `func(.t = expr1, .f = expr2)` (and
similarly `T:name` instead of `T.name` for a designator-qualified type).
Advantage: in the call syntax, the name is not preceded by a gratuitous dot.

The straw-man syntax for separating positional from nonpositional parameters is
arbitrary.  Alternatives to consider would be using a different delimiter,
e.g., `/` or `|` instead of `,.,`; grouping nonpositional parameters in square
or curly braces; or indicating positional and nonpositional designated
parameters with a different tokens sequence, e.g., a `.^` prefix or `^:` suffix
for designated parameters that are also positional.

# Previous Attempts and Proposing Named Arguments

[N0088R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/1992/WG21%201992/X3J16_92-0010R1%20WG21_N0088R1.pdf)

> All named parameters are designated parameters. Use of designated arguments
> is an option at call site.

[@N4172] Named Arguments ((discussion notes)[http://wiki.edg.com/bin/view/Wg21urbana-champaign/FunctionFacilities#N4172_Named_Function_Arguments])

> All named parameters are designated parameters. Use of designated arguments
> is an option at call site.

[@P0671R2] Self-explanatory Function Arguments (see References for more links)

> Naming arguments does not permit re-ordering them.  This is for making code
> easier to read and compiler diagnostics only; it does not increase the
> expressiveness of the language.

[@P1229R0] Labelled Parameters [British spelling of labeled]

> Not a well-written paper. After a garden-path walk through an unworkable
> library solution, it proposes putting a label before the parameter
> declaration and using `label : expr` for arguments.  For non-positional
> parameters, `explicit` is used ahead of the label, which is
> interesting. Also, it re-orders the parameter naming so that you would write
> `name : type` instead of `type .name` or `type name:`; I find that weird.
> Nevertheless, it is similar to our proposal, but does not go into detail
> about parameter packs, overloading, etc., despite saying that it is
> "compatible with `std::forward`ing."

[@P3405R0] Out-of-order designated initializers

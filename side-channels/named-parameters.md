---
title: "Designated Parameters"
document: named-parameters
# $TimeStamp$
date: 2025-09-24 18:06 EDT
# $
audience: EWGI
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
adopting a syntax for designating (named) parameters, provides a number of
motivating use cases, and provides a proposed syntax and semantics for the
proposed new designated-parameter feature. Though formal wording is not
proposed at this time, we thoroughly explore how the new feature would interact
with other aspects of C++, including overload resolution, template argument
deduction, and the type system. We also explore why previous proposals in this
area have failed and how this proposal overcomes the deficiencies of those
earlier proposals. This proposal is in the exploratory phase and its target
audience is EWGI.

Note to early reviewers: This document started out as a collection of Pablo's
musings but turned into something that is well on the way to being a complete
proposal.

# Motivation

Existing languages that provide the ability to name arguments at a function
call site (e.g., Python, Swift, and Ada), benefit from easier to read
(self-documenting) function calls, resistance to certain common errors (such as
out-of-order arguments), and easier-to-evolve function interfaces. Along with
the benefits enjoyed by other languages, designated parameters would benefit C++ in
ways peculiar to its syntactic weaknesses, such as potential overload
ambiguities and constraints on default- and variadic-argument
ordering. Additionally, designated parameters would make C++ more expressive by
expanding the range of potential overloads and supporting cleaner idioms for
generic programming.  Each of these benefits is described in detail in the
[Motivating Use Cases](#motivating-use-cases) section, below.

In the absence of designated parameters, programmers have devised a number of
library-only techniques attempting to get some of the benefits of designated
parameters in C++. The use of a `struct` parameter with designated initializers
(see
[Library Workaround Using Designated Aggregate Initializers](#library-workaround-using-designated-aggregate-initializers))
makes calls easier to read, less error prone, and/or making the interface
easier to evolve. Tagged parameters (see
[Simpler Code Evolution and Fewer Overloads](#simpler-code-evolution-and-fewer-overloads))
provide overload disambiguation and some measure of interface evolution. These
techniques are ad-hoc, inconsistent with one another, don't resemble normal
parameter declarations, and don't allow arguments to be reordered.

# A Very Short Syntax Summary

The syntax used in this document is not from fully baked; some alternatives are
described in the
[Alternative Syntax and Terminology](#alternative-syntax-and-terminology)
section, below. This section provides just enough description to understand the
[Motivating Use Cases](#motivating-use-cases) section. Syntactic constraints
and detailed semantics, including impact on the type system are described
later, in the [Proposal Details](#proposal-details) section.

We propose a function-parameter declaration syntax where a dot (`.`)
followed by a parameter name forms a _designator_, a name with semantic
significance that must be the same in every declaration of the function and is
used to identify the parameter at function invocation. A caret (`^`) before a
parameter name indicates that the designated parameter is _also_ a positional
parameter (see below). There would be no change to the meaning of parameter
names _not_ preceded by a dot:

```cpp
template <class Shape, class Transform>
Shape munge(const Shape&     shape,            // normal parameter
            const Transform& .^transform,      // @**positional**@ designator
            double           .scale     = 1.0, // @**nonpositional**@ designators
            const Point&     .translate = {0.0, 0.0});
```

The argument-passing syntax at the call site resembles designated member
initialization for aggregate types ([dcl.init.aggr]{.sref}). The `munge`
function would be called like this:

```cpp
// Munge `myShape` using the specified lambda and scale by 1.5. The translation
// is not specifed, so the default translation is used.
auto newShape = munge(myShape, // bound to first parameter (`shape`)
                      .transform = [](Shape& s){ ... },
                      .scale     = 1.5);
```

Arguments can be bound to positional designated parameters with or without a
designator whereas nonpositional parameters can be bound only by using the
designated-argument syntax. Thus, the following invocation of `munge` is
equivalent to the one above because `transform` is positional.

```cpp
auto newShape = munge(myShape,              // bound to `shape`
                      [](Shape& s){ ... },  // bound to `transform`
                      .scale     = 1.5);    // `.scale =` is required
```

<!-- JMB: A big question i'd have here is how the named parameters impact the
     functino type, especially if they can also be positional.
     Does adding a designator a paramter name change its type?
     Can I convert a pointer to `int (int .name)` to a pointer to `int (int)`?
     Can I convert the other way?
     What about interopartion betweeon `int (int .name)` and `int (., int .name)`?

     Overall, I feel like there's a lot more ambiguity and confusing cases to consider
     if we allow parameters to be both named and positional
  -->


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

Although some might consider the interface to be too large, it is at worst
mildly so. The interface is straightforward, but is nevertheless
inconvenient and error prone to use. A number of problems with this interface
are detailed below, followed by a description of how each is improved by using
designated parameters as proposed in this paper.

## Less Error Prone

The first 4 parameters all have type `int`. It is thus very easy to supply
arguments in the wrong order or even assume the wrong meaning:

::: cmptable

### Without Designated Parameters

```cpp
int top   = 100,   left  = 150;
int height = 1100, width = 1000;

// BUG, but compiles
auto w = makeWindow(top, left, height, width);
```

### With Designated Parameters

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
come before optional ones. The designated arguments can appear in any order, so
the version on the right yields the expected result.

In the second case, the arguments are not only in the wrong order, but are
ascribed the wrong meaning. Again, the code looks correct and compiles
successfully, but result is not what is desired. The version on the right will
flag the error at compile time, as the misunderstanding results in a mismatch
between the argument designator and parameter designator.

## More Convenient

Default arguments are a convenience feature, but to supply a nondefault value
for one parameter, the caller must provide nondefault values for all preceding
parameters. In the example below, the `INT_MIN` arguments on the left are pure
noise and make the call harder to write and read. The code on the right is
clean and minimal.

::: cmptable

### Without Designated Parameters

```cpp
auto w = makeWindow(width, height, INT_MIN, INT_MIN, Color::grey50);
```

### With Designated Parameters

```cpp
auto w = makeWindow(width, height, .foreground = Color::grey50);
```

:::


## Self Documenting

Most code is read more often than it's written. Reading code, e.g., during a
code review, is more convenient when the reader doesn't need to look up the
parameter list for many functions.

::: cmptable

### Without Designated Parameters

```cpp
// What are all those numbers?
auto w = makeWindow(1000, 1500, 10, 20);
```

### With Designated Parameters

```cpp
// Obvious meaning
auto w = makeWindow(.width = 1000, .height = 1500,
                    .left  = 10,   .top = 20);
```

:::

Designated arguments reduce the cognitive load for both reading and writing
functions having as few as two parameters, with an exponentially bigger benefit
on each additional parameter.

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
the original function or by adding an overload. One solution, used in a few
places in the standard library, is to create a tag type at namespace scope that
precedes the `ThreadPool` parameter, where the tag type is expected never to be
used for any other purpose. This technique is necessitates a new overload for
each updated function and is a clunky workaround for the lack of designated
parameters in the language. Designated parameters, conversely, allow us to
cleanly add the `ThreadPool` parameter on the existing function without
creating overload ambiguities:

::: cmptable

### Without Designated Parameters

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

### With Designated Parameters

```cpp









template <class T = double, class... Args>
T calculate(T x = {}, Args&&... args,
            ThreadPool& .threadpool = defaultThreadPool);

auto r1 = calculate(19.4, "encode", 1.0);
auto r2 = calculate(19.4, "encode", 1.0,
                    .threadpool = myThreadPool);
```

:::

## More Expressive Overloading

Consider a `Point` class representing a point in 2-dimensional space. A `Point`
can be constructed using either Cartesian coordinates or polar coordinates.  In
both cases, the constructor take two parameters of type `double`. Designated
parameters allow the two constructors to coexist without using a disambiguating
tag type:

::: cmptable

### Without Designated Parameters

```cpp
class Point
{
public:
  struct polar_coord_t { explicit polar_coord_t() = default; };
  static inline constexpr polar_coord_t polar_coord{};

  Point(double x, double y);
  Point(polar_coord_t, double radius, double angle);
};

Point p1(4.0, 3.0);
Point p2(Point::polar_coord, 5.0, 0.6435);
```

### With Designated Parameters

```cpp
class Point
{
public:



  constexpr Point(double .^x, double .^y);
  constexpr Point(double .radius, double .angle);
};

Point p1(4.0, 3.0); // same as Point p1(.x=4.0, .y=3.0);
Point p2(.radius=5.0, .angle=0.6435);
```

:::

<!-- JMB: So the case I find confusing that arises here because of having
     parameters that are both designated and postiional is that we can also
     now invoke `Point p3(3.0, .y=5)` and that woudl work.  Similarly,
     could we do `Point p4(.y=5.0, 3.0)`?    -->

## More Expressive Generic Programming

Although some version of designated arguments have been proposed several times
(see the [Previous Attempts](#previous-attempts) section) for promoting
code-clarity and preventing errors, the proximate reason that the authors are
revisiting this topic at this time is for generic programming. In particular,
it is often desirable to pass a contextual argument through multiple levels of
generic code to a function or constructor that needs it. Examples include
allocators, logging streams, and thread pools. Currently, passing such a
parameter requires rigid parameter-order rules that might conflict with one
another. Typically a function having one of these cross-cutting parameters must
put that parameter last (which conflicts with variadic parameters) or first
(which makes it difficult to give it a default argument value). Both
conventions can cause conflicts if two or more of these environmental
parameters are needed on the same function. All of those issues are obviated
through the use of designated parameters:

```cpp
template <class... Args, class Allocator = std::allocator<>>
class Service {
public:
  Service(int x, Args&&... args,
          Logger&   .logger    = global_logger(),
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
the `S` constructor is that the constructor takes a designated parameter named
"logger." Some of the enhancements described in the
[Possible Enhancements](#possible-enhancements) section would make even this
knowledge unnecessary.

A planned future proposal built on top of this designated-parameter proposal
would expand on this feature by allowing default arguments to get their values
from the caller's environment, replacing some current (unsafe) uses of global
and thread-local variables, while reducing the noise of passing arguments down
an arbitrarily deep function-call hierarchy.

# Proposal Details

## Designated and Positional Parameters

We introduce two new terms for describing function parameters:

- **Designated** - Indicates that the parameter's name applies not only to a
  local-variable declaration, but to a semantically significant name used to
  identify the matching argument at the call site. Designated parameters are
  sometimes refered to as _named_ parameters, but the term, _named_, does not
  distinguish between local names and semantically significant designators.
- **Positional** - Indicates that the position of the parameter or argument in
  the overall list of parameters is meaningful. C++26 parameters and arguments
  are always positional.

In this proposal, a function parameter can be

1. Positional and undesignated (the only option in C++26)
2. Nonpositional and designated
3. Positional and designated

## Divergence in Co-author Opinions

One of the co-authors of this paper, Joshua Berne, is not sure that the third
combination is needed for a successful feature, while the other co-author,
Pablo Halpern, is convinced that it is needed to simplify wide adoption in
existing libraries. For example, consider the `makeWindow` example from earlier
in this paper. We can _modernize_ `makeWindow` by providing designators for
each argument, but we want to ensure that existing calls using purely
positional arguments continue to work while also enabling the use of designated
arguments:

```cpp
auto w1 = makeWindow(w, h, x, y);
auto w2 = makeWindow(.top = y, .left = x, .width = w, .height = h);
```

Let's see how we might do this with and without positional designated
parameters:

::: cmptable

### Without Positional Designated Parameters

```cpp
WinHandle makeWindow(int   width,                     int   height,
                     int   left,                      int   top = INT_MIN,
                     Color background = Color::white, Color foreground = Color::black);
WinHandle makeWindow(int   .width,                     int   .height,
                     int   .left         = INT_MIN,    int   .top = INT_MIN,
                     Color .background = Color::white, Color .foreground = Color::black);
```

### With Positional Designated Parameters

```cpp



WinHandle makeWindow(int   .^width,                     int   .^height,
                     int   .^left         = INT_MIN,    int   .^top = INT_MIN,
                     Color .^background = Color::white, Color .^foreground = Color::black);
```

:::

The difference in effort between the two versions might seem minimal: simply
add an extra overload in nonpositional case, but the transformation is
nontrivial. On the left side of the table, notice that, to prevent ambiguity,
we needed to remove the default argument from `left` in the nondesignated
overload --- a change that makes the process more error prone. If there are
many such functions, adding designated-parameter overloads could double the
size of in the interface. Templates and variadic parameters complicate things
further.

One of the benefits of designated arguments is that it is possible to add new
parameters to a function without disrupting existing calls, provided the new
parameter has a default argument value. Assume we add a `.keep_on_top = false`
parameter to `makeWindow` function. Using positional designated parameters
results in a smaller minimal change to use the new parameter at existing call
sites:

::: cmptable

### Without Positional Designated Parameters

```cpp
// Before:
auto w = makeWindow(w, h, l, t);

// After
auto w = makeWindow(.width = w, .height = h, .left = l, .top = t,
                    .keep_on_top = true);
```

### With Positional Designated Parameters

```cpp
// Before:
auto w = makeWindow(w, h, l, t);

// After
auto w = makeWindow(w, h, l, t, .keep_on_top = true);
```

:::

The change on the right is not only less work, but is also less error prone
than on the left because it avoids errors from incorrectly naming the arguments
as a result of forgetting the (less than totally intuitive) order of
parameters.

<!-- JMB: I guess if you're taking all of the defaulted arguments and making them
     designatable you need can't just do that, but I think we want such a disambiguation
     rule no matter what --- if two overloads match and one has defaulted
     designed parameters then prefer the one without them.  But with such
     disambugation we just need one alternate overload that has all of the
     optional arguments as defaulted designated parameters, and then we don't
     have super-confusing uses where we mix and match.
     -->
<!-- PGH: Your proposed disambiguation rule solves the first complication, but
     not the second, which I just added. It also feels like an arbitrary.
     workaround, but might still be worth considering, regardless of our
     decision on positional designated parameters. -->

Finally, overloading is not always an option. It might be desirable for
`auto p = &makeWindow` to unambiguously produce a pointer-to-function object,
something that would not be possible if `makeWindow` were an overload set. See
[Function-pointer Conversions](#function-pointer-conversions), below.

For now, we will explore a design space that includes positional designated
parameters, so that we can find any difficulties and possible ways of mitigating
them. A straw-man syntax indicates that a designated parameter is positional by
adding an caret before the parameter name:

```cpp
// `func` has positional designated parameters `x` and `y` and
// nonpositional designated parameters `a` and `b`.
void func(int .^x, int .^y, float .a, std::string .b = {});
```

Note that a separator such as this is no longer needed if we simply choose
to not support paramters that are both positional and designated.

## Parameter and Argument Order

Parameters in a function declaration must appear in the following order (any or
all of the following may be absent):

1. Positional undesignated parameters without default arguments
2. Positional designated parameters without default arguments
3. Positional designated and undesignated parameters with default arguments
4. An undesignated variadic parameter (with ellipsis)
5. Nonpositional designated parameters

For positional parameters, the order of types and position of designators is
significant; two declarations that differ in the order of positional parameters
declare different functions. For nonpositional parameters, order is not
significant; two declarations that differ only in the order of nonpositional
parameters refer to the same function.

Examples:

```cpp
void f1(int x, int y = 0, double .scale = 1.0);

template <class... V>
void f2(int x, int .^y = 0, int z = 0, V&&...v, int .origin, int .offset = 0);
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

## Interaction with Overload Resolution

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
  int open(Mode .mode, const std::string .^filename); // overload #1
  int open(Mode .mode, const std::string .^url);      // overload #2
  ...
};
```

Changes in overload resolution ([over.match]{.sref}) should be
modest, affecting mostly the first step of identifying viable functions in
section [over.match.viable]{.sref}. Rather than specifying this step based on
the number of arguments, we specify an algorithm for matching each argument to
a corresponding function parameter.

For a function call having _N_ positional arguments PA~i~, for integer _i_ in
the range 1..._N_, followed by _M_ designated arguments, DA~d~, where each _d_
is a designator name (not an integer), arguments are matched to parameters as
follows.

For each nondesignated argument, PA~i~,

- if a positional parameter PP~i~ exists, then PA~i~ matches PP~i~;
- otherwise, if there is a variadic parameter, then PA~i~ matches the variadic
  parameter;
- otherwise, PA~i~ has no match.

For each designated argument, DA~d~,

- if a designated parameter DP~d~ exists, then DA~d~ matches DP~d~;
- otherwise, if the first unmatched positional parameter has type (possibly
  cvref-qualified) `T`, where `T` is a deduced template parameter, then DA~d~
  matches that positional parameter;
  <!-- JMB:  Is this for some kind of forwarding reference?  -->

- otherwise, if there is a variadic parameter of type (possibly
  cvref-qualified) `Args...`, where `Args` is a deduced template parameter
  pack, then DA~d~ matches that variadic parameter (see
  [Interaction with Parameter Packs](#interaction-with-parameter-packs),
  below);
- otherwise, DA~d~ has no match.

To be a viable function,

- each argument must match exactly one parameter, and
- each nonvariadic parameter must match exactly one argument or else match no
  arguments and have a default argument.

Note that a positional designated parameter can be matched either by position
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

TBD: Need examples of overload resolution, including examples of errors where
positional argument and a designated argument refer to the same parameter.

## Interaction with Parameter Packs

Consider a function template that simply performs an action before and after
calling an invocable entity, `f`, with supplied arguments:

```cpp
template <class F, class... Args>
auto verbose_call(F&& f, Args&&... args) -> decltype(auto) {
  std::cout << "Prologue\n";
  auto on_scope_exit = [](const char* s){ std::cout << s; };
  std::unique_ptr<const char, decltype(on_scope_exit)> e("Epilogue\n",
                                                         on_scope_exit);
  return std::forward<F>(f)(std::forward<Args>(args)...);
}
```

It is extremely desirable for the above template, which is valid C++26, to
continue to work unchanged in the world of designated parameters. Given the
following declaration and call:

```cpp
void func(int, const char* .^name);
verbose_call(func, 15, .name="Fred");
```

We would want the invocation of f within `verbose_call` to expand to something
equivalent to the following.

```cpp
  decltype(auto) ret = std::forward<void (&)(int, const char* .^name)>(func)(
    std::forward<int>(15), .name = std::forward<const char (&)[5]>("Fred"));
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
qualifiers_. In the expression, `verbose_call(func, 15, .name="Fred")`, the third
argument has type `const char (&)[5]`, with `.name` as its designator
qualifier. If we were to reify the designator qualifier as something that can
be uttered directly by the programmer, we might say that `decltype(.length =
5)` has type `int.length` or, more generally, that `decltype(.@_name_@ =
@_expr_@)` has type `decltype(@_expr_@).@_name_@`.  Such reification is
probably not essential, but it does make designated parameters easier to talk
about and it is useful to be able to designator-qualified type directly, e.g.,
to store an argument list in a `tuple` (`tuple<int.x, int.y>`), to express a
type trait (`is_constructible_v<T, float.name>`), etc.. Given this approach,
the invocation of f within `verbose_call` would forward the `.name` argument as
a designator-qualified array of `const char`:

```cpp
  decltype(auto) ret = std::forward<void (&)(int, const char* .^name)>(func)(
    std::forward<int>(15), std::forward<const char (&.name)[5]>("Fred"));
```

The name-matching rules in the previous section
([Interaction with Overload Resolution](#interaction-with-overload-resolution))
should therefore be modified such that a designated parameter's type is
expressed as being designator-qualified and would match a designated parameter
having the same designator.

There are no conversions between a type and its designator-qualified variant. A
designator `.d` is appended to the type of an expression `e` of type `T` using
the expression `.d = e`. Conversely, an expression `de` of type `T.d` is
stripped of its designator by binding it to a parameter with designator `.d`.
We might also allow the same rules to apply to `auto` variables:

```cpp
void g()
{
  auto di = (.x = 5);  // decltype(di) is int.x
  auto .x = di;        // decltype(x) = int
  assert(x == 5);
}
```

The only implicit conversion-like binding is that an expression without a
designator can bind to a positional parameter with a designator.

A value of designator-qualified type could also be extracted from a parameter
pack using the designator as a pack index:

```cpp
auto v = args...[.x];
```

Many of the semantics of designated parameters and designated arguments flow
naturally from the introduction of designator-qualified types and obviate
special-case handling in the Core Language, the Standard Library, and in
compiler implementations.  Most generic code will continue to yield expected
results without modification.  For example:

```cpp
// `forward_as_tuple` returns a `tuple` instantiated with two designator-qualified types.
// `p.second` is initialized with arguments `.value=x, .offset=-1`.
std::pair<int, W> p(std::piecewise_construct,
                    std::forward_as_tuple(99),
                    std::forward_as_tuple(.value=x, .offset=-1));

// Yields `tuple<const char *, int.angle, double.radius>`
auto t = std::make_tuple("hello", .angle=30, .radius=2.5);

// Invokes `func("hello", .angle=30, .radius=2.5)`
std::apply(func, t);
```

## Interaction with Type System

Designated parameters are encoded in the type system. Because
designator-qualified types are part of a function's signature, a function's
type naturally includes the designators for its parameters.  Similarly, a
function pointer can be declared with designators:

```cpp
int (*fp1)(int x, int .^value);
```

## Function-pointer Conversions

We can introduce a new rule whereby a pointer to function having
positional designated parameters can be converted to a pointer to function
having the same positional parameters, but where some of them are not
designated:

```
int (*fp2)(int x, int v) = fp1;  // implicit conversion
```

For overload resolution, this conversion would have the rank of either a
promotion or a standard conversion.

TBD: How much more needs to be said about interaction with the type system?

<!-- JMB: I think the biggest gap is the interaction between
     functions where the parameters are designated and those where
     parametesr are both designated and posiional.  In particular, can I use
     `f(int .name)` with a function pointer of type `int (., int .name)` ---
     I can certainly call both of those functions with a designed name.

## Interaction with Reflection

TBD

<!-- JMB: We definitely need to say something more clear about how designators
     attach to a variable's type and how you can strip them off, and that inevitably
     gets exposed through reflection.

     We similarly need to be able to splice designators alongside a type, and posisbly
     we might want to splice just desginators (which might need a disambugator of some
     kind).  -->


# Possible Enhancements

TBD: Each of the enhancements listed here needs explanation.

- Special designated arguments that override same-named arguments in a
  parameter pack.

- Special designated arguments that are ignored on function calls that don't
  have such a designated parameter.

  <!-- JMB: I think this one is about calling a generic function when you have
       specified designated arguments it doesn't expect.   There are use
       cases for both directions --- reject, and ignore arguments we don't
       need to pass.  Ideally, if we have a syntax to say "here's extra
       designated params that you might not need" then we would also short-circuit
       the expressions in those params when they are not needed.
       -->

- Variadic designated parameters

- Designated template parameters (i.e., `template <class T, class .Allocator>`)

# Impact on the Standard Library

TBD: We would want to at least enhance _uses-allocator construction_ to
recognize parameters of the form `Allocator .allocator`.  We might also want to
tackle some cases of tagged parameters; providing a prettier alternative by way
of new overloads.

<!-- JMB: we should also call out places such as emplace functions where we think
  there won't be a need for any change to work with designated parameters.  -->


# ABI Considerations

TBD: We anticipate little or no impact on ABI for existing code.

<!-- JMB: If we have params that are both positional and designated then adding
     a designator is an abi change because that has to be mangled along with the
     function (because we can then overload on that designator).   That's an
     important consideration for any library changes to make use of the feature,
     but I agree it doesn't impact current functions.   -->

# Alternative Syntax and Terminology

**Alternative Nomenclature**: Consider using the term _label_ instead of
_designator_ (and corresponding _labeled parameter_, _labeled argument_,
_label-qualified type_).

**Alternative designator syntax**: The current proposed call syntax resembles
designated initializers for structs in ways that are both good and bad.
Although designated initializers are a somewhat-familiar syntax at the _call
site_, parameter passing is _not_ the same thing as struct initialization and
the resemblance could cause confusion, especially with nondesignated arguments
in the mix. Moreover, the parameter _declaration_ syntax is entirely new.

An alternative worth considering is the syntax, `func(t : expr1, f :expr2)` for
calling a function and `void func(T t:, U (&f:)());` for declaring it.
Advantage: in this alternative call syntax, the argument name is not preceded
by a gratuitous dot and a function argument does not look like an assignment
expression. _Editorial note: Pablo prefers this syntax. The term_ label _is
probably better than_ designator _for this syntax_.

**Alternative positional/nonpositional syntax**: The straw-man syntax (a
leading `^`) for distinguishing positional from nonpositional designated
parameters is arbitrary; a different prefix or suffix character could be chosen
or an entirely different syntax can be proposed.  One alternative to consider
would be using a delimiter, e.g., `,.,`, `/` or `|`, where positional
parameters come before the delimiter and nonpositional parameters after the
delimiter. Another alternative would be to group nonpositional parameters in
square or curly braces, where anything not within the braces would be
positional.

<!-- JMB: Considering that i consider parameters that are both positional and
     designatable to be frought with potentials for misuse and confusion I lean
     towards making that the harder syntax to write.   requirng that they
     use `f(int .^name)` and then not needing the confusing `.,` parameters
     would be fine, and would also make that whole aspect of the proposal something
     that can be layered on top of the rest.

     -->

# Previous Attempts at Designated Arguments

## Language Proposals

TBD: This section is incomplete.  In particular, we need to look up the
discussion notes on each proposal and see why the committee rejected them.

[@N0060] Keyword Parameters in C++

[@N4172] Named Arguments ((discussion notes)[http://wiki.edg.com/bin/view/Wg21urbana-champaign/FunctionFacilities#N4172_Named_Function_Arguments])

> Both of these papers propose using the argument name from the function
> declaration as the name of the argument that can be used at the call
> site. These proposals can produce inconsistent results when functions are
> declared more than once in a program and the declarations do not use the same
> parameter names. It has long been true (even in when N0050 was proposed in
> 1991), going back to C++'s roots in C, that parameter names in function
> declarations had no semantic significance, except in the function's
> definition, and it was not practical to suddenly make them significant.
> Moreover, the names of parameters in the Standard Library are not themselves
> standard, so such use of named parameters is not portable.

[@P0671R2] Self-explanatory Function Arguments (see References for more links)

> This proposal uses a new syntax for naming parameters but does not permit
> re-ordering arguments at the call site. Although it would have made client
> code easier to read and reduce the chance of certain errors, it falls short
> of providing the many benefits of true designated parameters.  Ultimately,
> this proposal would have improved compiler diagnostics, but would not have
> increased the expressiveness of the language.

[@P1229R0] Labelled Parameters [British spelling of labeled]

> This paper is the most similar to our proposal.  After a garden-path walk
> through an unworkable library solution, it proposes putting a label before
> the parameter declaration and using `label : expr` for arguments.  For
> nonpositional parameters, `explicit` is used ahead of the label, which is
> interesting. It also re-orders the parameter naming so that you would write
> `name : type` instead of `type .name` or `type name:`, which is a gratuitous
> change to the declaration syntax.  Ultimately, it is not a well-written paper
> and is very incomplete. It does not go into detail about parameter packs
> (despite saying that it is "compatible with `std::forward`ing."),
> overloading, or integration with the type system.

[@P3405R0] Out-of-order designated initializers

> This paper does not propose designated parameters _per se_, but does relax
> the restriction on the ordering of designated initializers for aggregates,
> making the use of designated initializers more suitable as a substitute for
> true designated parameters. It does nothing for solving the other problems
> with designated initializers, however, including their not working with
> template argument deduction.

**In Summary**, none of these previous attempts was a complete proposal that
addressed the full range of issues required to integrate a designated
parameters feature into modern C++.

## Library Workaround Using Designated Aggregate Initializers

One technique used to get the _illusion_ of designated parameters uses a
`struct` to aggregate a function's parameters. Rather than having _n_
parameters, the function has a single parameter of a custom `struct` type
having _n_ members:

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

**Shortcomings**

- Every function potentially needs a custom struct.
- The caller cannot provide arguments other than in the order declared,
  although [@P3405R0] would change that, if adopted.
- Does not work with template argument deduction.
- For default-constructible parameter types, there is no way to force the
  caller to supply a value. For example, in the declaration of `MakeWindow`
  above, we might want to force the caller to specify `width` and `height`, but
  there is no way to express that with this idiom.


---
references:
  - id: N0060
    citation-label: N0060 (X3J16_91-0127)
    author:
      - name: Roland Hartinger
      - name: Andreas Schmidt
      - name: Erwin Unruh
    title: "Keyword parameters in C++"
    date: 1991
    URL: https://www.open-std.org/JTC1/SC22/WG21/docs/papers/1991/WG21%201991/X3J16_91-0127%20WG21_N0060.pdf

  - id: N0088R1
    citation-label: N0088R1 (X3J16_92-0010R1)
    author: Bruce Eckle
    title: "Analysis of C++ Keyword Arguments"
    date: 1992
    URL: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/1992/WG21%201992/X3J16_92-0010R1%20WG21_N0088R1.pdf
---

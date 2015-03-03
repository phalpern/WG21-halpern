% D4393 | Noop Constructors and Destructors
% Pablo Halpern <phalpern@halpernwightsoftware.com>
% 2015-03-03

Abstract
========

This paper proposes a language feature for no-op constructor and
destructor invocations, i.e., the ability to invoke a constructor or
destructor without actually causing the state of memory to change. This
feature allows a collection of correctly-configured bits to become an object,
and an object to become a collection of bits without actually executing any
code.  Use cases and potential syntax for this feature are presented. This
paper does not present precise wording, for which I would seek collaboration
from someone in the Core Working Group if and when the Evolution Working Group
approves the idea.

Motivation
==========

The use case that inspired this feature was "destructive move", as proposed in
[N4158][]. That paper proposed a function template,
`unitialized_destructive_move`, that destroyed one object and created another
object, sometimes as a single, indivisible, operation, without invoking the
objects' destructor or constructor. The proposal was worded in such a way as
to give this template a special status in the core language, but it was
considered inelegant that an object's lifetime would start at the completion
of its constructor _and one other way_. This led me to search for a
generalized way of getting the desired effect without making
`destructive_move` special in the core language. In the processes, I
considered other situations in my career where I had wanted the ability to
skip a constructor or destructor invocation. For example, had this feature
been available in C++0x, the piecewise constructor for `pair` might have been
unnecessary.

Description of Proposed feature
===============================

The proposal is to use a special token sequence as a sort of "magic cookie" to
invoke a constructor or destructor such that the invocation has no effect on
the state of the program, but retains the quality of beginning or ending the
lifetime of an object.  To avoid beginning a premature bike-shed discussion on
the syntax of this magic cookie, let's just call it `__COOKIE__`. Later in
this paper, I list a number of combinations of tokens that could unambiguously
work for `__COOKIE__`. The important thing is that, whatever `__COOKIE__` is,
it is *not* an expression.

If `__COOKIE__` is passed as the sole argument to a constructor, then that
constructor invocation has no effect on the state of the program. However,
after the invocation, the lifetime of the object, and its base class and
member subobjects, is deemed to have begun, just as if a real constructor had
been invoked. Similarly, passing `__COOKIE__` to a destructor (yes, a
destructor call with arguments!) would have no effect on the state of program,
but the lifetime of the object, and its base class and member subobjects, is
deemed to have ended.

A class cannot declare `__COOKIE__` constructors or destructors -- they are
automatically available in every type.  An invocation of a `__COOKIE__`
constructor or destructor does not require overload resolution, since no
function is actually being called.

An invocation of a `__COOKIE__` constructor or destructor is valid before or
after the invocation of a real constructor or destructor, respectively, and is
idempotent with other, `__COOKIE__` invocations on the same object.  It is the
responsibility of the caller to ensure that the bytes that make up an object
constructed using the `__COOKIE__` constructor are valid; setting the bytes to
a valid pattern can be done either before or after the noop constructor is
invoked.

Use cases
=========

Destructive Move
----------------

Destructive move is the motivating use case for the noop constructor and
destructor feature.  Assume an implementation of `std::list` that has a
heap-allocated sentinel node. The move constructor is not `noexcept` because
it must allocate a sentinel node for the moved-from list in order to avoid the
"emptier than empty" condition.  However the *destructive move* operation
*can* be `noexcept` because there is no moved-from object left behind.  The
trick is to move the list without calling the move constructor.  The
implementation of `uninitialized_destructive_move` for such a list might look
like this:

    template <class T, class A>
    void uninitialized_destructive_move(std::list<T,A> *from,
                                        std::list<T,A> *to) noexcept
    {
        // Preconditions: `from` points to a valid list object;
        // `to` points to uninitialized memory.

        typedef std::list<T,A> list_t;

        // Move data members over. Note that no new sentinel node is allocated.
        to->m_begin = from->m_begin;
        to->m_end  = from->m_end;
        new (&to->m_allocator) A(std::move(from->m_allocator));

        from->m_begin = from->m_end = nullptr_t;  // unnecessary, but safe
        from->m_allocator.~A();

        // Bless the new list and unbless the old one
        new (to) list_t(__COOKIE__);
        from->~list_t(__COOKIE__);
        // Postconditions: `from` points to uninitialized memory;
        // `to` points to a valid list object
    }

If the allocator is trivially movable and trivially destructible, this
operation can be simplified to an invocation of `memcpy`:

    template <class T, class A>
    std::enable_if<is_trivially_destructive_movable_v<A>, void>
    uninitialized_destructive_move(std::list<T,A> *from,
                                   std::list<T,A> *to) noexcept
    {
        typedef std::list<T,A> list_t;
        std::memcpy(to, from, sizeof(list_t));
        new (to) list_t(__COOKIE__);
        from->~list_t(__COOKIE__);
    }

This idiom would work for the vast majority of value classes, and is
generalized in N4158 for all "trivially destructive movable" types.

Swizzle to disk
---------------

A noop constructor is useful any time the bits that compose an object are
arranged outside of the object's constructor.  A carefully-designed data
structure can be written straight to disk and read back again:

    // Type that uses relative pointers and is designed for storage on disk
    class record { ... };

    record *my_record = ...;
    ...
    file.write(my_record, sizeof(record));
    ...

    record *my_record2 = new record(__COOKIE__);
    file.read(my_record2, sizeof(record));

Choosing a constructor at run time
----------------------------------

Sometimes, it is necessary to choose a constructor at runtime, passing
arguments of different types or different number of arguments depending on
some condition:

    struct Y {
        Y(float) noexcept;
        Y(int, float) noexcept;
        ...
    };
    
    struct X {
        Y m_y;
        ...
        X(float a, int b);
    };

    X::X(float a, int b) : m_y(__COOKIE__) {
        // Choose one of two constructors for m_y
        if (b > 0)
            new (&m_y) Y(b, a); // Invoke Y(int, float)
        else
            new (&m_y) Y(a);    // Invoke Y(float)
        ...
    }

Dangers
=======

It should be obvious to anybody reading this paper that both the no-op
constructor and no-op destructor are both extremely dangerous operations. One
could easily write code that uses an object that has not been initialized or
which has already been destroyed.  If the no-op destructor is used on a
variable with static or automatic lifetime, the real destructor will still be
called.

The dangers of these constructs are not, however, any worse than the current
dangers of calling a destructor manually (`x.~T()`), allocating an object
using malloc without calling the constructor, or (deliberately) overwriting an
object using `memcpy`.  Just like these other programming techniques, proper
application should typically be left to expert programmers writing reusable
libraries (including standard library components such as
`uninitialized_destructive_move`).

Alternatives for `__COOKIE__`
=============================

There are an infinite number of existing tokens or token sequences that could
be used for `__COOKIE__` in the language definition.  I will list a few here,
and leave it up to your imagination to come up with better ones. In looking at
the aesthetics of each example, think of it in the context of declaring a
variable `x`.  For example, if `__COOKIE__` were `=0`, then a variable
declaration with a no-op constructor would look like `X x(=0)`.

Some possibilities are:

        =0
        =delete
        =void
        %
        /
        ^
        ;
        .
        ?
        <0>
        <void>
        ~ (requires look-ahead)
        ! (requires look-ahead)
        delete (requires look-ahead)
        =noop (context-sensitive keyword)

A non-serious suggestion: A number of emoticons would also work. `:-)`

Alternatives considered
=======================

Special Case for `uninitialized_destructive_move`
-------------------------------------------------

N4158 had weasel words that allowed `uninitialized_destructive_move` to start
and end the lifetimes of its arguments without saying how.

**Pros**:

 * Library-only change for most compilers. However, tools that track object
   lifetime would need to hook all specializations of
   `uninitialized_destructive_move`.

**Cons**:

 * Special-case weasel words are not elegant.
 * Does not support other use cases besides destructive move.

Function templates `bless`/`unbless`
------------------------------------

Instead of adding a magic cookie, we could add two magic function templates:

    namespace std {
        template <class T> void bless(T& obj) noexcept;
        template <class T> void unbless(T& obj) noexcept;
    }

The `bless` function would begin the lifetime of an object without invoking
its constructor.  The `unbless` function would end the lifetime of an object
without invoking its destructor.

**Pros**:

 * Trivial no-op functions with no compiler changes on most
   implementations. However, tools that track object lifetime would need to
   hook all specializations of `bless` and `unbless`.
 * General-purpose functions can be used for use cases other than
   destructive move, such as swizzling from disk.

**Cons**:

 * Changes the rule that an object's life begins at the end of its constructor
   invocation to a less elegant rule that an object's life begins at the end
   of its constructor invocation _or on return from std::bless_.  A similar
   change would be needed for the destructor/`std::unbless`.
 * Does not allow suppressing normal constructor invocation for automatic,
   static, and member variables and base class subobjects.  Hence does not
   support use cases like the "Choosing a constructor at run time" use case,
   above.

Future enhancements
===================

Although the `__COOKIE__` constructor suppresses automatic constructor
invocation for objects of any storage duration, this proposal does not provide
a mechanism for suppressing the implicit destructor invocations for automatic,
static, and member variables and base class subobjects. Such a feature might
be desirable, especially for member and base class subobjects, which might
need to be destroyed within the body of the class destructor, e.g., to obtain
an unorthodox order of destruction.  Local variables, too, might occasionally
need to be destroyed in other than reverse order of construction, or it might
be desirable to suppress destruction during exception unwinding in certain
cases.

The ability to suppress automatic destruction could be seen as a natural
extension of this proposal, but it is not central to the goal of providing a
way to begin or end the lifetime of an object without invoking its constructor
or destructor.

Implementation Concerns
=======================

We have no experience implementing this proposal.  We do have experience
faking it for some use cases (including `uninitialized_destructive_move`).

Nevertheless, it should be easy to implement in a compiler -- just recognize
the magic cookie and replace constructor or destructor invocation by a
no-op. Tools that track object lifetime would treat the special calls like
normal constructor or destructor calls with implicit construction or
destruction of subobjects.

References
==========
[N4158]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n4158.pdf
[N4158][] _Destructive Move_, Pablo Halpern, 2014-10-12

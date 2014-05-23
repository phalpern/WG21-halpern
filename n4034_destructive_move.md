% Destructive Move | D4034
% Pablo Halpern <phalpern@halpernwightsoftware.com>
% 2014-05-22

Abstract
========

This paper proposes a function template for performing _destructive move_
operations -- a type of move construction where the moved-from object, rather
than being left in a "valid but unspecified" state, is left instead in a
destructed state.  I will show that this operation can be made nothrow in a
wider range of situations than a normal move constructor and can be used to
optimize operations such as reallocations within vectors.  An array version of
the destructive move template is proposed specifically for moving multiple
objects efficiently and with the strong exception guarantee.

The facilities described in this paper are targeted for a future library
Technical Specification.


Motivation
==========

The main reason that rvalue references and move operations were introduced int
the standard was to improve performance by reducing expensive copy operations.
`noexcept` was added in order to extend the number of situations where move
operations could be used.  Specifically, an operation that moves multiple
objects and which is guaranteed to have not permanent effect when an exception
is thrown cannot use move operations unless those operations are known to not
throw exceptions.  An important client of `noexcept` is the reallocation
subroutine within the implementation of `vector`.

Unfortunately, the requirement that move constructors leave the moved-from
object in a valid (though unspecified) state results in a number of situations
where a move constructor cannot be decorated with `noexcept`.

many classes with throwing move constructors can have nothrow destructive move

many classes with non-trivial move constructors can have trivial destructive
move.



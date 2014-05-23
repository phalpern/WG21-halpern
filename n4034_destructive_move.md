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

Background
----------

The main reason that rvalue references and move operations were introduced int
the standard was to improve performance by reducing expensive copy operations.
`noexcept` was added in order to support a number of important use cases where
move operations could not otherwise be used. Because move constructors modify
the moved-from object, an operation that moves multiple elements, e.g., in a
container, could result both containers being in a half-moved state if one of
the move constructors throws an exception.  It is not possible to reliably
reverse this half-moved situation without risking another exception being
thrown.

Using `noexcept`, an implementation can detect whether it is possible that a
move constructor might throw, and can choose copy construction, instead.  In
fact, the standard provides the function template `move_if_noexcept`
specifically for this purpose.  The following implementation of a simplified
vector `push_back` uses this idiom to preserve the "strong exception
guarantee" whereby the moved-from vector remains unchanged if an exception is
thrown:

    template <class T, class A>
    void simple_vec<T, A>::push_back(const T& v)
    {
        typedef std::allocator_traits<A> alloc_traits;

        if (m_length == m_capacity) {
            // Grow the vector by creating a new one and swapping
            simple_vec temp(m_alloc);
            temp.m_capacity = (m_capacity ? 2 * m_capacity : 1);
            temp.m_data = alloc_traits::allocate(m_alloc, temp.m_capacity);

            T *from = m_data, *to = temp.m_data;
            for (temp.m_length = 0; temp.m_length < m_length; ++temp.m_length)
                alloc_traits::construct(m_alloc, to++,
                                        std::move_if_noexcept(*from++));
            temp.swap(*this);
            // destructor for 'temp' destroys moved-from elements
        }

        alloc_traits::construct(m_alloc, &m_data[m_length], v);
        ++m_length;
    }

Lost opportunities
------------------

Unfortunately, the requirement that move constructors leave the moved-from
object in a valid (though unspecified) state results in a number of situations
where a move constructor cannot be decorated with `noexcept`. For example,
some implementations of `list`, including at least one commercial
implementation, use a heap-allocated sentinel node in order to preserve the
stability of the `end()` iterator when using `swap` and `splice`. A moved from
`list` implemented this way must have a sentinel node in order to avoid an
"emptier than empty" violation of its class invariants. The default
constructor and move constructor for such a list might look like the following:

    // Default constructor for a simple list type (no allocator support)
    template <class T>
    void simple_list<T>::simple_list()
        : m_begin(nullptr), m_end(nullptr)
    {
        m_begin = m_end = new Node(nullptr, nullptr);  // might throw
    }
    

    // Move constructor for a simple list type (no allocator support)
    template <class T, class A>
    void simple_list<T, A>::simple_list(simple_list&& other)
    {
        simple_list temp;  // Default constructor might throw
        temp.swap(*this);
    }

Since the sentinel node requires a memory allocation which might throw,
neither the default constructor nor the move constructor can be decorated with
`noexcept`.  Such a type cannot participate in the `move_if_noexcept`
optimization -- it would need to be copied every time.

As you can see from the `push_back` code for `simple_vec` in the previous
section, after all of the elements have been moved from one place to the
other, the moved-from elements are destroyed. It is not truly necessary in
this and many similar situations to leave the moved-from object in a valid
state -- it would be sufficient to end its lifetime as part of the move
(i.e., as if the destructor had been called).

Why is this important? Many, if not most, classes that cannot offer a nothrow
move constructor _can_ offer a nothrow destructive move operation -- a move
combined with a destroy. In the case of our `simple_list`, above, the
destructive move operation would simply move the `m_begin` and `m_end`
pointers from the list being moved from to the list being moved to. Any
attempt to use (or destroy) the moved-from object would be undefined
behavior.  Thus, a destructive move operation could expand the set of cases
that could benefit from `move_if_noexcept`-like optimization.

Another benefit of destructive move is that it is often more efficient to
perform a destructive move operation than a non-destructive move construction.
In the case of a string, for example, a destructive move would simply copy
pointers.  Since pointers are trivially copyable, the entire move operation
becomes a trivial copy that can be implemented as a `memcpy`. This
optimization is magnified when operating on entire arrays of strings: the
entire array can be destructively moved with a single `memcpy`.  This
optimization was implemented at Bloomberg before move constructors were even
invented and has yielded significant performance gains. It turns out that a
large number of classes can be _trivially destructive-movable_. Note that
trivially destructive-movable does not require or imply trivially copyable.
After the destructive move is complete, the moved-from object must not be
accessed, since any pointers would be shared with the moved-to object.

Proposal summary
================

This proposal comprises two new function templates and two new traits.  The
first function template is called `destructive_move` and looks like this:

    template <class T>
      void destructive_move(T* to, T* from) noexcept(see below);

The preconditions are that `from` points to a valid object and `to` points to
raw memory.  The postconditions are that `to` points to a valid object and
`from` points to raw memory.  The default implementation is simply:

    ::new(to) T(std::move(*from));
    to->~T();

However, this default can be overridden in two ways:

 1. If the trait `is_trivially_destructive_movable<T>` is true, then
    the destructive move is implemented using byte copies. This trait is
    always true for types that are trivially movable, but can also be
    overridden for other types by the class authors.

 2. If `destructive_move` is overloaded for a specific type, then ADL will
    resolve to the overloaded version. Thus, a class author can implement an
    efficient destructive move (with its own `noexcept` clause) for his/her
    new type.

The `noexcept` specification for this function template is computed to be
`true` for as many types as possible.  Only if a type as a throwing move
constructor and does not override the default for destructive move is the
`noexcept` specification false.

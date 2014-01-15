% Task Region | N3832
% Pablo Halpern; Arch Robison
  {pablo.g.halpern, arch.robison}@intel.com
  Hong Hong; Artur Laksberg; Gor Nishanov; Herb Sutter
  {honghong, arturl, gorn, hsutter}@microsoft.com
% 2014-01-13

# Abstract

This paper introduces C++ library functions `task_region`, `task_run` and
`task_wait` that enable developers to write expressive and portable parallel
code.

This proposal subsumes and improves the previous proposal, N3711.


# Motivation and Related Proposals

The Parallel STL proposal [N3724][] augments the STL algorithms with the
inclusion of parallel execution policies. Programmers use these as a basis to
write additional high-level algorithms that can be implemented in terms of the
provided parallel algorithms. However, the scope of N3724 does not include
lower-level mechanisms to express arbitrary fork-join parallelism.

Over the last several years, Microsoft and Intel have collaborated to produce
a set of common libraries known as the Parallel Patterns Library ([PPL][]) by
Microsoft and the Threading Building Blocks ([TBB][]) by Intel.  The two
libraries have been a part of the commercial products shipped by Microsoft and
Intel.  Additionally, the paper is informed by Intel's experience with
[Cilk Plus][], an extension to C++ included in the Intel C++ compiler in the
Intel Composer XE product.

The `task_region`, `task_run` and the `task_wait` functions proposed in this
document are  based on the `task_group` concept that is a part of the common
subset of the PPL and the TBB  libraries. The paper also enables approaches to
fork-join parallelism that are not limited to  library solutions, by proposing a
small addition to the C++ language.

A previous proposal, [N3711][], was presented to the Committee at the Chicago
meeting in 2013. N3711 closely follows the design of the PPL/TBB with slight
modifications to improve exception safety.

This proposal adopts a simpler syntax than [N3711][] -- one that eschews a
named object in favor of three namespace-scoped functions.  It improves N3711
in the following ways:

* The exception handling model is simplified and more consistent with normal 
  C++ exceptions.
* Strict fork-join parallelism is now enforced at compile time. 
* The syntax allows scheduling approaches other than child stealing.

We aim to converge with the language-based proposal for low-level parallelism
described in [N3409][] and related documents.


# Introduction

Consider an example of a parallel traversal of a tree, where a user-provided
function `compute` is applied to each node of the tree, returning the sum of the
results:

    template<typename Func>
    int traverse(node*n, Func&& compute)
    {
        int left = 0, right = 0;

        task_region([&] {
            if (n->left)
                task_run([&] { left = traverse(n->left, compute); });
            if (n->right)
                task_run([&] { right = traverse(n->right, compute); });
        });

        return compute(n) + left + right;
    }

The example above demonstrates the use of two of functions proposed in this
paper, `task_region` and `task_run`.

The `task_region` function delineates a region in a program code potentially
containing invocations of tasks spawned by the `task_run` function.

The `task_run` function spawns a _task_, a unit of work that is allowed to
execute in parallel with respect to the caller.  Any parallel tasks spawned by
`task_run` within the `task_region` are joined back to a single thread of
execution at the end of the `task_region`.

`task_run` takes a user-provided function object `f` and starts it
asynchronously -- i.e. it may return before the execution of `f` completes.
The implementation's scheduler may choose to run `f` immediately or delay
running `f` until compute resources become available.

`task_run` can be invoked (directly or indirectly) only from a user-provided
function passed to `task_region`, otherwise the behavior is undefined:

    void g();

    void f() thread_switching
    {
        task_run(g);    // OK, invoked from within task_region in h
    }

    void h() thread_switching
    {
        task_region(f);
    }

    int main()
    {
        task_run(g);    // No active task_region. Undefined behavior.
        return 0;
    }


# Interface

The proposed interface is as follows.

## Header `<experimental/parallel_task>` synopsis

    namespace std {
    namespace experimental {
    namespace parallel {

        template<typename F>
        void task_region(F&& f) thread_switching;

        template<typename F>
        void task_run(F&& f) thread_switching noexcept;

        void task_wait() thread_switching;

        class task_cancelled_exception;

    }
    }
    }

## Function template `task_region`

    template<typename F>
    void task_region(F&& f) thread_switching;

_Requires_: The invocation operator for `F` shall be declared with the
_thread-switching-specification_, as described in
[Thread Switching][]. `F` shall be `MoveConstructible`. The expression,
`(void) f()`, shall be well-formed.

_Effects_: Invokes the expression `f()` on the user-provided object, `f`.

_Throws_: `exception_list`, as specified in [Exception Handling][].

_Postcondition_: All tasks spawned from `f` have finished execution.

_Notes_: It is expected (but not mandated) that `f`
will (directly or indirectly) call `task_run`.

## Function template `task_run`

    template<typename F>
    void task_run(F&& f) thread_switching;

_Requires_: The invocation of `task_run` shall be directly or indirectly
nested within an invocation of `task_region`.  `F` shall be
`MoveConstructible`. The expression, `(void) f()`, shall be well-formed.

_Effects_: Causes the expression `f()` to be invoked asynchronously at an
unspecified time prior to the next invocation of `task_wait` or completion of
the nearest enclosing `task_region`, where "nearest enclosing" means the
`task_region` that was most recently invoked and which has not yet returned.
[_Note:_ the relationship between a `task_run` and its nearest enclosing
`task_region` is similar to the relationship between a `throw` statement and
its nearest `try` block. -- _end note_]

_Throws_: `task_cancelled_exception`, as defined in [Exception Handling][].

_Notes_: The invocation of the user-supplied invocable, `f`, may be immediate
or may be delayed until compute resources are available.  `task_run` might or
might not return before invocation of `f` completes.

`task_run` may be called directly or indirectly from a function object passed
to `task_run`.  The nested task started in such manner is guaranteed to finish
with rest of the tasks started in the nearest enclosing `task_region` [*Note:*
An implementation is allowed to join tasks at any point, for example at the
end of the enclosing `task_run`. -- *end note*]

[_Example_:

    task_region([] {
        task_run([] {
            task_run([] {
                f();
            });
        });
    });

-- _end example_]

## Function `task_wait`

    void task_wait() thread_switching;

_Requires_: The invocation of `task_wait` shall be directly or indirectly
nested within an invocation of `task_region`.

_Effects_: Blocks until the tasks spawned by the nearest enclosing `task_region`
have finished.

_Throws_: `task_cancelled_exception`, as defined in [Exception Handling][].

_Postcondition_: All tasks spawned by the nearest enclosing `task_region`
have finished.

[_Example:_

    task_region([&]{
        task_run([&]{ process(a, w, x); }); // Process a[w] through a[x]
        if (y < x) task_wait();             // Wait if overlap between [w,x) and [y,z)
        process(a, y, z);                   // Process a[y] through a[z]
    });

-- _end example_]

<a id="Exception_Handling"></a>

# Exception Handling

Every `task_region` has an associated exception list. When the
`task_region` starts, its associated exception list is empty.

When an exception is thrown from the user-provided function object passed to
`task_region`, it is added to the exception list for that `task_region`.
Similarly, when an exception is thrown from the user-provided function object
passed into `task_run`, the exception object is added to the exception list
associated with the nearest enclosing `task_region`. In both cases, an
implementation may discard any pending tasks that have not yet been invoked.
Tasks that are already in progress are not interrupted except at a call to
task_run or task_wait, as described above.

If the implementation is able to detect that an exception has been thrown by
another task within the same nearest enclosing `task_region`, then `task_run` or
`task_wait` may throw `task_cancelled_exception`.

The class `task_cancelled_exception` is defined as follows:

    class task_cancelled_exception : public exception {
    public:
        task_cancelled_exception() noexcept;
        task_cancelled_exception(const task_cancelled_exception&) noexcept;
        task_cancelled_exception& operator=(const task_cancelled_exception&) noexcept;
        virtual const char* what() const noexcept;
    };

When `task_region` finishes with a non-empty exception list, the exceptions are
aggregated into an `exception_list` object (defined below), which is then thrown
from the `task_region`.

Instances of the `task_cancelled_exception` exception thrown from `task_run` or
`task_wait` are not added to the exception list of the corresponding
`task_group`.

The order of the exceptions in the `exception_list` object is unspecified.

The `exception_list` class was first introduced in [N3724][] and is defined as
follows:

    class exception_list : public exception
    {
    public:
        typedef exception_ptr value_type;
        typedef const value_type& reference;
        typedef const value_type& const_reference;
        typedef size_t size_type;
        typedef vector<exception_ptr>::iterator iterator;
        typedef vector<exception_ptr>::const_iterator const_iterator;

        exception_list(vector<exception_ptr> exceptions);

        size_t size() const;
        const_iterator begin();
        const_iterator end();
    private:
        // ...
    };


# Scheduling Strategies

A possible implementation of the `task_run` is to spawn individual tasks  and
immediately return to the caller. These _child_ tasks are then executed (or
_stolen_) by a scheduler based on the availability of hardware resources and
other factors. The parent thread  may participate in the execution of the tasks
when it reaches the join point (i.e. at the  end of the execution of the
function object passed to the `task_region`). This approach to scheduling is
known as _child stealing_.

Other approaches to scheduling exist. In the approach pioneered by Cilk, the
parent thread immediately executes the spawned task at the spawn point. The
execution of the rest of the function -- i.e., the _continuation_ -- is stolen
by the scheduler if there are hardware resources available.  Otherwise, the
parent thread returns from the spawned task and continues as if it had been a
normal function call instead of a spawn.  This approach to scheduling is known
as _continuation stealing_ (or _parent stealing_).

Both approaches have advantages and disadvantages. It has been shown that the
continuation stealing approach provides lower asymptotic space guarantees in
addition to other benefits. Child stealing is generally easier to implement
without compiler involvement.  [N3872][] provides a worthwhile primer that
addresses the differences between, and respective benefits of, these scheduling
approaches.

It is the intent of this proposal to enable both scheduling approaches and, in
general, to be as open as possible to additional scheduling approaches.


<a id="Thread_Switching"></a>

# Unresolved issues: Thread Switching and Returning with Unjoined Children

One of the properties of continuation stealing and greedy scheduling is that a
`task_region` or `task_run` call might return on a different thread than that
from which it was invoked, assuming scheduler threads are mapped 1:1 to standard
threads. This phenomenon, which is new to C++, can be surprising to
programmers and break programs that rely on the OS thread remaining the same
throughout the serial portions of the function (for example, in programs
accessing GUI objects, mutexes, thread-local storage and thread id).

Additionally, the _terminally strict_ semantics of the constructs proposed in
this paper allow a function to return to the caller while its child tasks are
still running.  Again, this behavior can be surprising to programmers and
break programs that rely on functions finishing their work before they
return. Although unstructured concurrency constructs such fire-and-forget
threads already violate these assumptions, we are attempting, in this paper,
to define much more structured constructs that operate at a much finer
granularity of work. Programmers writing _structured_ parallel code need to be
put on notice when a function invoked in their program might spawn parallel
tasks and return without joining them first.  Additionally, the compiler's
optimizer may be impaired in doing its job if it needs to defensively assume
that any function might return with child tasks still running.

In order to address these two concerns, we propose a new keyword,
`thread_switching` (bike shed TBD), to be applied on the declaration of
functions that are allowed to return on a different thread and/or with
unjoined child tasks.

A function declaration specifies whether it can return on another thread or with
unjoined tasks by using a _thread-switching-specification_ as a suffix of its
declarator:

    void f() thread_switching;

We have considered several alternative semantics for the 
_thread-switching-specification_. The following is a couple of alternative
solutions. We seek the Committee's guidance as to the best approach.

The first alternative is as follows: When a function declared with
the _thread-switching-specification_ is directly invoked in a function declared
without the _thread-switching-specification_, the compiler injects a
synchronization point that joins with the (inherited) child tasks and resumes
on the original thread:

    void f() thread_switching;

    int main() {
        auto thread_id_begin = std::this_thread::get_id();
        f();
        auto thread_id_end = std::this_thread::get_id();
        assert(thread_id_end == thread_id_begin);
        return 0;
    }

Another alternative calls for disallowing invocations of
thread-switching functions in regular functions. In other words, a function
declared with the _thread-switching-specification_ can be directly invoked only
in functions declared with the _thread-switching-specification_:

    void f() thread_switching;

    void g() thread_switching {
        f();                // OK
    }

    void h() {
        f();                // ill-formed
    }

In both alternatives, a lambda expression that directly calls a function with the 
_thread-switching-specification_ is considered to inherit the 
_thread-switching-specification_:

    void f() thread_switching;
    auto l1 = [] thread_switching { f(); };  // OK
    auto l2 = [] { f(); };                   // OK

This makes it possible to avoid the explicit _thread-switching-specification_
for a lambda expression passed into `task_region` while still invoking a
function with the _thread-switching-specification_:

    void f() thread_switching;
    task_region([] {
        f();            // OK
    });

The programmer is not required to decorate the entire call chain of the
program starting from the entry point to use the parallelism constructs
introduced in this proposal. We therefore introduce the function
`task_region_final` (more bike-shed discussion needed), declared without the
_thread-switching-specification_.  This function is the same as `task_region`
except it is guaranteed to return on the same thread on which it was called.

In implementations that do not support gready scheduling, the behavior of
`task_region_final` is identical to `task_region`.

The `thread_switching` keyword is not the only possible way to address the
issues of thread switching and unjoined children.  The authors continue to
research other alternatives, both with and without language support.  We
believe, however, that the core library interfaces described in this paper are
unlikely to change significantly as we work through these issues.


[TBB]: https://www.threadingbuildingblocks.org/
    "Treading Building Blocks (TBB)"

[PPL]: http://msdn.microsoft.com/en-us/library/dd492418.aspx
    "Parallel Patterns Library (PPL)"

[Cilk Plus]: http://cilkplus.org
    "Cilk Plus"

[N3724]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3724.pdf
    "A Parallel Algorithms Library"

[N3711]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3711.pdf
    "Task Groups As a Lower Level C++ Library Solution To Fork-Join Parallelism"

[3409]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2012/n3409.pdf
    "Strict Fork-Join Parallelism"

[N3872]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n3872.pdf
    "A Primer on Scheduling Fork-Join Parallelism with Work Stealing"

[Exception Handling]: #Exception_Handling

[Thread Switching]: #Thread_Switching

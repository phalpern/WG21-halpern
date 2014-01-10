% Task Region | N3832
% Pablo Halpern; Arch Robison
  {pablo.g.halpern, arch.robison}@intel.com
  Hong Hong; Artur Laksberg; Gor Nishanov; Herb Sutter
  {honghong, arturl, gorn, hsutter}@microsoft.com
% 2013-12-29

# Abstract

This paper introduces C++ library functions `task_region` and `task_run` that enable
developers to write expressive and portable parallel code. 

This proposal subsumes and improves the previous proposal, N3711.

# Motivation and Related Proposals

The Parallel STL proposal N3724 augments the STL algorithms with the inclusion 
of parallel execution policies. Programmers use these as a basis to write additional high-level 
algorithms that can be implemented in terms of the provided parallel algorithms. However, the 
scope of N3724 does not include lower-level mechanisms to express arbitrary fork-join parallelism. 

Over the last several years, Microsoft and Intel have collaborated to produce a set of common libraries
known as the Parallel Patterns Library (PPL) by Microsoft and the Threading Building Blocks (TBB) by Intel. 
The two libraries have been a part of the commercial products shipped by Microsoft and Intel.

The `task_region` and the `task_run` concepts proposed in this document are based on the `task_group` 
concept that is a part of the common subset of the PPL and the TBB.

The previous proposal, N3711, was presented to the Committee at the Chicago meeting in 2013. N3711 closely
follows the design of the PPL/TBB with slight modifications to improve the exception safety.

This proposal further improves the exception handling aspects of N3711, adopting a
simpler syntax that eschews a named object in favor of the two static functions. As well, the
new approach enables compile-time enforcement of strict fork-join parallelism.

The proposal also aims to converge with the language-based proposal for low-level parallelism 
described in N3409 and related documents.

TODO: now also supports parent stealing, with a keyword `placeholder` for that.

While the proposed syntax differs from PPL/TBB, it can still be implemented trivially,
for instance using PPL/TBB or a thread pool. A couple of reference implementations exist (for
example, refer to <https://goldenrod.codeplex.com>)

# Introduction

Consider an example of a parallel traversal of a graph, where a user-provided function
`f` is applied to each node of the graph, returning the sum of the results:

```
template<typename Func>
int traverse(node*n, Func && f)
{
    int left = 0, right = 0;

    task_region([&] {
        if (n->left)
            task_run([&] { left = traverse(n->left, f); });
        if (n->right)
            task_run([&] { right = traverse(n->right, f); });
    });

    return f(n) + left + right;
}
```

The example above demonstrates the use of the two proposed functions. 

The `task_region` function delineates a region of code potentially containing 
invocations of tasks spawned by the `task_run` function.

The `task_run` function spawns a _task_, a unit of work that is allowed to execute
in parallel with respect to the caller.

# Interface

The interface of the proposed API is as follows:

```
template<typename F>
void task_region(F && f);
```
_Effects_: applies the user-provided function object.

_Throws_: `exception_list`, as defined in [Exception Handling](#Exception_Handling).

_Postcondition_: all tasks spawned from `f` have finished execution.

_Notes_: it is expected (but not mandated) that the user-provided function can 
(directly or indirectly) call `task_run`.

The function `task_run` can only be invoked inside a user-provided function passed
to `task_region`. The function itself takes a user-provided function and starts it
asynchronously -- i.e. it may return before the execution of `f` completes.

```
template<typename F>
void task_run(F && f) noexcept;

```
_Requires_: invocation of the function must be inside the `task_region` context.

_Effects_: starts the user-provided function, potentially on another thread.

_Notes_: the function may or not return before the user-provided function completes.

TODO: do we need `task_wait`?

# Exception Handling

Every `task_region` has an exception list associated with it. When the `task_region` starts,
the exception list associated with it is empty.

When an exception is thrown during the execution of the user-provided function object
passed to `task_region`, it is added to the exception list for that `task_region`.

Similarly, when an exception happens during the execution of the user-provided function object
passed into `task_run`, the exception object is added to the exception list associated
with the closest enclosing `task_region`.

When `task_region` finishes with a non-empty exception list, the exceptions are
aggregated into an `exception_list` object (defined below), which is then thrown
from the `task_region`.

The order of the exceptions in the `exception_list` object is unspecified.

TODO: this is debatable, but it is hard to define strict ordering between exceptions
thrown by child vs. parent contexts. We could define at least partial ordering, if
it is useful.

The `exception_list` class was first introduced in N3724 and is defined as follows:

```
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
```

# Scheduling Strategies

A possible implementation of the `task_run` is to spawn individual tasks 
and immediately return to the caller. These _child_ tasks are then executed (or _stolen_) by a scheduler 
based on the availability of hardware resources and other factors. The parent thread 
may participate in the execution of tasks when it reaches the join point (i.e. at the 
end of the execution of the function object passed to the `task_region`). This approach
to scheduling is known as the _child stealing_.

Other approaches to scheduling exist. In the approach pioneered by Cilk (TODO: reference), the parent
thread proceeds executing the task at the spawn point. The execution of the rest of the function 
-- i.e. the _continuation_ -- is left to a scheduler. This approach to scheduling is known 
as the _continuation stealing_ <!-- I like the term better than 'parent stealing' because it describes what is being stolen -->
(or _parent stealing_).

Both approaches have advantages and disadvantages. It has been shown that the _continuation stealing_ 
approach provides lower asymptotic space guarantees in addition to other benefits. 

It is the intent of the proposal to enable both scheduling approaches.

# Thread Switching

One of the properties of the continuation stealing is that a part of the user function may
execute on a thread different from the one that invoked the `task_run` or the `task_region` methods.
This phenomenon is not allowed by C++ today. For this reason, this behavior can be surprising 
to programmers and break programs that rely on the thread remaining the same throughout the
execution of the function (for example, in programs accessing GUI objects, mutexes, thread-local storage and thread id).

Additionally, programmers writing structured parallel code need to be put on notice when a function 
invoked in their program spawns parallel tasks and returns before joining them. 

We propose a new keyword, the `placholder` to be applied on the declaration of functions that are allowed
to return on a different thread and with unjoined parallel tasks.

TODO: explain transitive calls

TODO: rules for lambdas.

# Open Issues

TODO: consider renaming task_region, task_run to parallel_region, parallel_spawn.

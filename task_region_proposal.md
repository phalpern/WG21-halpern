% Task Region | N3832
% TBD
<!-- Will add authors later
  Pablo Halpern; Arch Robison
  {pablo.g.halpern, arch.robison}@intel.com
  Artur Laksberg; Herb Sutter
  {arturl, hsutter}@microsoft.com
-->
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

TODO: note about thread switching.

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

# Open Issues

TODO: thread switching

TODO: consider renaming task_region, task_run to parallel_region, parallel_spawn.

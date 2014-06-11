% An Abstract Model of Simd Parallelism in C and C++
% Pablo Halpern (<pablo.g.halpern@intel.com>), Intel Corp.
% 2014-06-10

Motivation
==========

When trying to define portable C and C++ constructs to enable a programmer to
write loops that take advantage simd-parallel hardware, we at Intel have,
until now, tried to use a model similar to that used for enabling
task-parallel (multicore) loops.  In Robert Geva's proposal to the C++
standards committee, [N3831][], a simd-parallel loop is distinguished from a
task-parallel loop only by the ordering dependencies that are and are not
allowed in each.  For a simd-parallel loop, the proposal says this:

> For all expressions X and Y evaluated as part of a SIMD loop, if X is
> sequenced before Y in a single iteration of the serialization of the loop
> and i <= j, then X~i~ is sequenced before Y~j~ in the SIMD loop.

There are a number of problems with this definition of a SIMD loop:

 1. The definition itself is an arcane set of constraints on a program -- it
    derived from our (Intel's) internal mental model of how simd execution
    works.  It is not itself a model of simd execution and does little to
    help the programmer decide whether simd execution is appropriate to
    his/her problem.

 2. It encourages people to think of task parallelism and simd parallelism
    as being the same thing, with only subtle differences, and it invites them
    to find ways to smooth over those differences.  Indeed, in the C++
    standards committee, we are finding it difficult to convince people that a
    loop with a simd-only policy (as opposed to a simd + parallel policy)
    is an important top-level concept.

 3. The model is not rich enough to express cross-lane operations such as
    compress and expand operations and localized reductions. These operations
    were deemed important for maximizing the benefit of simd hardware, but
    proposals to add such features do not fit well with the limited
    description of simd execution implied by the above dependency statement.

For these reasons, I have attempted in this paper to describe a model for
simd execution.  This model subsumes the dependency statement from N3831 but
is much more complete and provides a framework for describing cross-lane
dependencies.  The model attempts to describe modern SIMD hardware without
being specific to any one architecture or CPU manufacturer.

There is precedent for abstract descriptions of common hardware patterns.  The
C standard describes a fairly detailed memory and execution model that applies
to most, but not all, computation hardware.  For example, the memory footprint
of a single `struct` is flat -- all of the bytes making up the `struct` are
consecutive (except for alignment and padding, which is also part of the
model). The C model also specifies that integers must be represented in
binary, that positive integers must have the same representation as unsigned
integers, etc.. This model has withstood the test of time, describing
virtually every CPU on the market, regardless of word size, instruction set,
or manufacturing technology.  Although I don't expect that the model described
here can be quite as general, my hope is that it is general enough to describe
a wide range of present and future hardware.

This paper does not propose any syntax. Rather, it is a description of
concepts that can be rendered using many different possible language
constructs.

Simd execution
================

Simd Lanes and Chunks
-----------------------

A loop executes in chunks of VL contiguous iterations at a time.  Each
iteration within a chunk is handled by an execution unit called a "lane".
Within the abstraction VL is *unspecified* and might be as small as 1 or as
large as the loop count. (In rare cases, it may be necessary to pierce the
abstraction.) The loop executes chunks until all of the iterations in the loop
have been executed.  The chunks need not all be the same size; it is common
for the first and/or last chunk to be smaller than the rest.

Each chunk is broken into steps.  There is a logical barrier at the end of
each step, with all of the lanes completing one step before any lane starts
the next step.  Each step is further subdivided into smaller steps,
recursively applying the same barrier rule until we come to a primitive step
that cannot be subdivided.  If a local variable (including any temporary
variable) is needed by any step, then a separate instance of that local
variable is created for each lane.  Except as described later, modifying a
non-local variable yields undefined behavior (i.e., it is a data race).

Individual lanes within a chunk are not expected to make independent progress.
If any lane within a chunk blocks (e.g., on a mutex), then it is likely (but
not required) that all lanes will stop making progress.

Control-flow divergence
-----------------------

A _selection-statement_ (`if` or `switch`) may create a control-flow
divergence between lanes. In this case, the correct branch is computed for
each lane and the lanes are divided into sets, called _branch sets_
corresponding to each computed branch. Each branch set can be represented as a
bit mask with a `1` bit for each lane in the set and a `0` bit for each lane
not in the set. Each possible branch is then evaluated with only those lanes
in the corresponding branch set participating in the execution.  This process
is repeated recursively until control flow converges again.  Each
_selection-statement_ is a step and all branches within a
_selection-statement_ comprise a single step. The compiler is thus free to
schedule multiple branches simultaneously.  In the following example, the
optimizer might condense two branches into a single hardware step by computing
a vector of `+1` and `-1` values and performing a single simd addition.

    for simd (int i = 0; i < N; ++) {
        if (c[i])
            a[i] += 1;
        else
            a[i] -= 1;
    }

Processing of an _iteration-statement_ (`while`, `do` or `for`) is similar to
a that of a _selection-statement_.  In this case, there are two branch sets:
the set of lanes for which the loop condition is false (loop is complete) and
the set of lanes for which the loop condition is true (loop must iterate
again).  The loop body is executed for each lane in the latter set, then the
sets are recomputed.  The loop terminates for all lanes (and control flow
converges) when the set of lanes with a true loop condition is empty.

Serial Steps
------------

It is important to note that any step or series of steps within a simd loop 
can be executed serially provided the end result is indistinguishable from
simd execution for all intents and purposes; the as-if rule applies.

A loop may contain a "simd-off" region, which is has the following
characteristics:

 1. Only one lane executes the simd-off region at a time.
 2. It is considered a single step, regardless of how many statements or
    expressions are within it.
 3. For each iteration within a chunk entering a simd-off region, the
    executions of the region are sequenced according to the serial ordering of
    the iterations. 

A call to a simd-enabled function proceeds as if that function were part of
the simd loop body.  A call to a non-simd-enabled function procedes as if the
call were in a simd-off region.  (Again, the as-if rule applies.  If the
compiler can prove that a non-simd function can be vectorized without changing
the observable behavior, it is free to do so.)

Varying, linear, and uniform
============================

Any variable or expression within a simd context can be described as
_varying_, _linear_, or _uniform_, as follows:

 * _varying_: The variable or expression can have a different, unrelated,
   value in each lane.
 * _linear_ (integral and pointer values only): The value of the variable or
   expression increases linearly between adjacent lanes.  The difference in
   value between one lane and the next is the _stride_.  A common special case
   is a _unit stride_ -- i.e., a stride of one -- which is often more efficient
   for the hardware, especially for pointers.
 * _uniform_: The value of the variable or expression is the same for all
   lanes.

When describing a variable as varying, linear, or uniform, it is important to
distinguish whether we are referring to the _value_ or the _address_ of the
variable.

In the case of uniform variables, there is an important difference between
reading and writing.  Reading a uniform value is, as a rule, safe and
well-defined.  Writing, on the other hand, can produce undefined behavior if
multiple lanes write different values.  A variable described outside of a loop
can be assumed to be uniform.  However, to avoid subtle run-time bugs, it
might be beneficial to have a syntax for indicating that only one lane is
expected to modify a uniform variable or that all lanes that touch it will set
the same value.  The compiler can then help diagnose common accidental race
conditions.

Lane-specific and cross-lane operations
=======================================

There are situations where the simd model is exposed very directly in the
code.  A short list of operations that might be useful are:

 * Compress: Put the results of a computation on a (non-contiguous) branch set
   into a contiguous set of memory locations.
 * Expand: Read values from a contiguous set of memory locations for use in a
   (non-contiguous) branch set.
 * Chunk reduction: Combine values from multiple lanes into one uniform value.
 * Lane ID: Get the ID number for the current lane.
 * Number of Lanes: Get the number of lanes in this chunk or in this branch
   set.

Note that many of these operations could be used in such a way as to break
serial equivalence; i.e., cause the result of a computation to differ
depending on how many simd lanes exist.  However, these operations are
meaningful within the model and, used carefully, can produce deterministic
results.

Conclusion
==========

The model presented here is intended to directly express the concept of
lock-step execution as implemented by x86 and other CPUs.  It is more powerful
than the model that has been presented to the C and C++ standards committee
because it allows for cross-lane operations.  The dependency constraints
described in N3831 fall out implicitly from this model, so that it is the
model that drives the constraints rather than the constraints attempting to
define a model.

[N3831]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n3831.pdf
[N3831][]: _Language Extensions for Vector level parallelism_, Robert Geva and
Clark Nelson, 2014-01-14

<!--

------------------------------------------------------------------------------


   compound statement
      statement
      statement
      ...

   Conditional statements:
      condition expression
      then part
      else part

   Expression statement
      Expression
         sub-expression
         sub-expression
         ...

Special cases
   Reductions
   Uniform variables
-->

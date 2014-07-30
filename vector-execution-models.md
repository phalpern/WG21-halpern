% An Abstract Model of Simd Parallelism in C and C++
% Pablo Halpern (<pablo.g.halpern@intel.com>), Intel Corp.
% 2014-06-11

Motivation
==========

When trying to define portable C and C++ constructs to enable a programmer to
write loops that take advantage simd-parallel hardware, we at Intel have,
until now, tried to promote a model similar to that used for enabling
task-parallel (multicore) loops.  In Robert Geva's proposal to the C++
standards committee, [N3831][], a simd-parallel loop is distinguished from a
task-parallel loop only by the ordering dependencies that are and are not
allowed in each.  For a simd-parallel loop, the proposal says this:

> For all expressions X and Y evaluated as part of a SIMD loop, if X is
> sequenced before Y in a single iteration of the serialization of the loop
> and i <= j, then X~i~ is sequenced before Y~j~ in the SIMD loop.

There are a number of problems with this definition of a SIMD loop:

 1. The description itself is a set of constraints on the execution -- it is
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
constructs. An important use of this model is as a set of fundamental concepts
that we can use to judge the appropriateness of future language and library
constructs and to help document those constructs that we choose to adopt.

Simd execution
==============

Simd Lanes and Steps
--------------------

A block of code is divided into one or more program steps (statements and
expressions), each of which can be decomposed of smaller sub-steps,
recursively until we reach a primitive operation.  In serial code, the steps
are executed one at a time by a single execution unit until control leaves the
block.

In SIMD code, the steps within a block are executed simultaneously by multiple
execution units called *lanes*.  The number of lanes, VL, is an *unspecified*
implementation quantity and might be as small as 1 for code that has not been
vectorized. The number of lanes can be queried by the program, but such a
query weakens the abstraction.  A SIMD loop executes in chunks of VL
sequential iterations, with each iteration assigned to a lane. The
chunks need not all be the same size (have the same VL); it is common for the
first and/or last chunk to be smaller than the rest.

Within a SIMD block, there is a logical barrier at the end of
each step, with all of the lanes completing one step before any lane starts
the next step.  Each step is further subdivided into smaller steps,
recursively, with a barrier at the end of each sub-step until we come to a
primitive step 
that cannot be subdivided.  If a local variable (including any temporary
variable) is needed by any step, then a separate instance of that local
variable is created for each lane.  Except for uniform variables, as described
later, modifying a
non-local variable yields undefined behavior (i.e., it is a data race).

There is no requirement or expectation that individual lanes within a chunk
will make independent progress.
If any lane within a chunk blocks (e.g., on a mutex), then it is likely (but
not required) that all lanes will stop making progress.

This _lock-step execution_ style, with a barrier at the end of every step, is
one of the key qualities that distinguishes SIMD from other vector models
(e.g., the pipelined vector model).  The SIMD model is optimized for modern
microprocessors but does not preclude the use of other types of vector
hardware, i.e., by compiler transforms that cause the program to behave _as
if_ using SIMD hardware.

Control-flow divergence
-----------------------

A _selection-statement_ (`if` or `switch`) or conditional operator (`&&`,
`||`, or `?:`) may create a control-flow divergence between lanes. In these
cases, a _mask_ of lanes is computed for each possible branch. Each mask is
conceptually a bit mask with a one bit for each lane that should take a
particular branch and a zero bit for each lane that should not take that
branch.  The code in each branch is then executed with only those lanes having
ones in the corresponding mask participating in the execution.  Execution
proceeds with some lanes "masked off" until control flow converges again at
the end of the selection statement or conditional operator.  This process is
repeated recursively for conditional operations within each branch.  Each
conditional operation is a step and all branches within a conditional
operation comprise a single step. The compiler is free to schedule multiple
branches simultaneously.  In the following example, the optimizer might
condense two branches into a single hardware step by computing a vector of
`+1` and `-1` values and performing a single simd addition.

    for simd (int i = 0; i < N; ++) {
        if (c[i])
            a[i] += 1;
        else
            a[i] -= 1;
    }

Processing of an _iteration-statement_ (`while`, `do` or `for`) is similar to
that of a _selection-statement_.  In this case, there is single mask
containing a one for each lane in which the loop condition is true (i.e., the
loop must iterate again).  The loop body is executed for each lane in this
set, then the mask is recomputed.  The loop terminates (and control flow
converges) when the set of lanes with a true loop condition is empty (i.e.,
the mask is all zeros).

Serial Steps
------------

A loop may contain a "simd-off" region, which has the following
characteristics:

 1. Only one lane executes the simd-off region at a time.

 2. It is considered a single step, regardless of how many statements or
    expressions are within it; no portion of a simd-off region on one lane is
    interleaved with any code on another lane.

 3. For each lane entering a simd-off region, the
    executions of the region are sequenced according to the serial ordering of
    lane indexes.

A call to a simd-enabled function proceeds as if that function were part of
the simd block.  A call to a non-simd-enabled function procedes as if the
call were in a simd-off region.  (Again, the as-if rule applies.  If the
compiler can prove that a non-simd function can be vectorized without changing
the observable behavior, it is free to do so.)

Varying, linear, and uniform
============================

Any variable or expression within a simd context can be described as
_varying_, _linear_, or _uniform_, as follows:

 * _varying_: The variable or expression can have a different, unrelated,
   value in each lane.
 * _linear_: The value of the variable or
   expression increases linearly between adjacent lanes.  The difference in
   value between one lane and the next is the _stride_.  A common special case
   is a _unit stride_ -- i.e., a stride of one -- which is often more efficient
   for the hardware, especially in the case of pointers.
 * _uniform_: The value of the variable or expression is the same for all
   lanes.

When describing a variable as varying, linear, or uniform, it is important to
distinguish whether we are referring to the _value_ or the _address_ of the
variable. For example, if a variable's _address_ is linear with unit stride,
it indicates that the variable has consecutive addresses in adjacent lanes,
whereas if its _value_ is linear (integral and pointer values only), it
indicates that it's value increases linearly in adjacent lanes.  The former is
useful for avoiding scatter-gather operations on the variable; the latter is
useful for avoiding scatter-gather operations on an array _indexed_ by the
value. Uniform and varying usually refer to a variable's address, but _can_
refer to an expression's value.  A uniform expression does not need to be
evaluated in each lane -- it can be evaluated once and broadcast to all
lanes. A varying expression (the default) potentially has a different value
in each lane.

In the case of uniform variables, there is an important difference between
reading and writing.  Reading a uniform value is, as a rule, safe and
well-defined.  Writing to a variable with a uniform address, on the other
hand, can produce undefined behavior if 
multiple lanes write different values.  A variable defined outside of a loop
can be assumed to be uniform.  However, to avoid subtle run-time bugs, it
might be beneficial to have a syntax for indicating that only one lane is
expected to modify a uniform variable or that all lanes that touch it will set
it to the same value.  The compiler can then help diagnose common accidental
race conditions.

Lane-specific and cross-lane operations
=======================================

There are situations where the simd model is exposed very directly in the
code.  A short list of operations that might be useful are:

 * Compress: Put the results of a computation from a non-contiguous set of
   lanes selected by a conditional mask into a contiguous sequence of memory
   locations.
 * Expand: Read values from a contiguous sequence of memory locations for use
   in a non-contiguous set of masked lanes.
 * Chunk reduction: Combine values from multiple lanes into one uniform value.
 * Lane index: Get the index for the current lane.
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
lock-step execution as implemented by x86, ARM, Power, and other CPUs.  It is
more powerful 
than the model that has been presented to the C and C++ standards committee
because it allows for cross-lane operations.  The dependency constraints
described in N3831 fall out implicitly from this model, so it is the
model that drives the constraints rather than the constraints attempting to
define a model.

References
==========

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

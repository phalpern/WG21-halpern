% An Abstract Model of Vector Parallelism
% Pablo Halpern (<pablo.g.halpern@intel.com>), Intel Corp.
% 2014-10-08

Abstract
========

This document presents a software model of vector execution, with three
identified variants.  It is intended to foster and inform discussion of
software vector models within Intel.  However, much of the content is not
Intel-specific and is likely to be re-used for similar documents in external
contexts, especially standards bodies where we are trying to influence the
design of parallel languages.

The model presented here is intended to help us create software for our
vector-enabled CPUs.  In addition to SIMD instructions, this paper mentions
GPUs, as many of the concepts apply in that realm.  The primary goal is not to
boost the programmability of GPUs, however, nor to make SIMD units and GPUs
look the same.  In fact, by carefully describing the model of vector
execution, we can more clearly articulate the differences between GPUs and
vector CPUs.

The model presented here itself is a software abstraction.  However, its
usefulness is predicated on having an efficient mapping to hardware concepts.
This paper, therefore, dips into implementation concerns, but it should not be
seen as an implementation specification or design document.

Motivation
==========

When trying to define portable C and C++ constructs to enable a programmer to
write loops that take advantage of simd-parallel hardware, we at Intel have,
until now, tried to promote a model similar to that used for enabling
task-parallel (multicore) loops.  In Robert Geva and Clark Nelson's proposal
to the C++ standards committee, [N3831][], a vector-parallel loop is
distinguished from a task-parallel loop only by the ordering dependencies that
are and are not allowed in each.  For a vector-parallel loop, the proposal
says this:

> For all expressions X and Y evaluated as part of a SIMD loop, if X is
> sequenced before Y in a single iteration of the serialization of the loop
> and i <= j, then X~i~ is sequenced before Y~j~ in the SIMD loop.

There are a number of problems with this definition of a SIMD loop:

 1. The description itself is a set of constraints on the execution -- it is
    derived from our (Intel's) internal mental model of how vector execution
    works.  It is not itself a model of vector execution and does little to
    help the programmer decide whether vector execution is appropriate to
    his/her problem.

 2. It encourages people to think of task parallelism and vector parallelism
    as being the same thing, with only subtle differences, and it invites them
    to find ways to smooth over those differences.  Indeed, in the C++
    standards committee, we are finding it difficult to convince people that a
    loop with a vector-only policy (as opposed to a vector + parallel policy)
    is an important top-level concept.

 3. The model is not rich enough to express cross-lane operations such as
    compress and expand operations and localized reductions. These operations
    were deemed important for maximizing the benefit of simd hardware, but
    proposals to add such features do not fit well with the limited
    description of vector execution implied by the above dependency statement.

For these reasons, I have attempted in this paper to describe more complete
models for vector execution.  These models subsume the dependency statement
from N3831 but are more descriptive and provide a framework for
describing cross-lane dependencies.  All of the models are intended to be
implementable on modern SIMD hardware without being specific to any one
architecture or CPU manufacturer.

There is precedent for abstract descriptions of common hardware patterns.  The
C standard describes a fairly detailed memory and execution model that applies
to most, but not all, computation hardware.  For example, the memory footprint
of a single `struct` is flat -- all of the bytes making up the `struct` are
consecutive (except for alignment and padding, which are also part of the
model). The C model also specifies that integers must be represented in
binary, that positive integers must have the same representation as unsigned
integers, etc.. This model has withstood the test of time, describing
virtually every CPU on the market, regardless of word size, instruction set,
or manufacturing technology.  Although I don't expect that the models
described here can be quite as general, my hope is that they are general
enough to describe a wide range of present and future hardware.


Overview of the Vector-Model Variants
=====================================

This paper describes three variants on a vector model:

 1. *Lockstep execution*: The variant used by ISPC
 2. *Wavefront execution*: The variant used by the Intel compiler
 3. *Explicit-barrier execution*: The variant used by CUDA*.

The variants presented here differ from one another in the kind of interaction
that is permitted across lanes and, consequently, the cross-lane dependencies
that can be expressed by the programmer.  Each variant is strictly weaker
(makes fewer ordering guarantees) than the one before it, so, for example, an
implementation that supports lockstep execution fully conforms to wavefront
and explicit-barrier execution semantics.

It is my understanding that the Intel compiler's vectorization model matches
the second ("wavefront") variant. The other two variants are described for
completeness and to provide a context for evaluating proposed vectorization
features coming from both inside and outside Intel.  Each variant has a
different set of pros and cons, so comparing them helps us understand the
strengths and weaknesses of our current variant.

This paper does not propose any syntax. Rather, it is a description of
concepts that can be rendered using many different possible language
constructs. An important use of these variants is as a set of fundamental
concepts that we can use to judge the appropriateness of future language and
library constructs and to help document those constructs that we choose to
adopt.

Concepts that are Common to All Vector Model Variants
=====================================================

The concepts of _program steps_, _vector lanes_, _control divergence_,
_vector-enabled function_, and _scalar region_, described in this section, are
common to all three variants.  Some of the details vary by variant.

Program Steps
-------------

A block of code is divided into one or more _program steps_ (statements and
expressions), each of which can be decomposed into smaller sub-steps,
recursively until we reach a primitive operation.  The decomposition of a
larger step into smaller steps follows the syntactic and semantic structure of
the language such that, e.g., a compound statement is decomposed into a series
of (nested) statements and an `if` statement is decomposed into a conditional
expression, a statement to be executed in the **true** case, and a statement to
be executed in the **false** case.

Vector Lanes and Gangs
----------------------

In serial code, the steps within a block are executed one at a time by a
single execution unit until control leaves the block.

In vectorized code, the steps within a single iteration of a loop are also
executed one at a time by a single execution unit.  However, multiple
iterations of that loop can be executed simultaneously, each by a separate
execution unit called a *lane*.  Borrowing from the ISPC documentation, we
will refer to a group of simultaneously-executing lanes as a *gang*.  The
number of lanes in a gang, VL, is an *unspecified* implementation quantity and
might be as small as 1 for code that has not been vectorized. VL can be
queried by the program, but such a query weakens the abstraction.  A vector
loop executes in chunks of VL sequential iterations, with each iteration
assigned to a lane. The chunks need not all be the same size (have the same
VL); it is common for the first and/or last chunk to be smaller than the rest
(e.g., for peeling and remainder loops).

The lanes in a gang are indexed from 0 to VL-1.  Lanes with smaller indexes
within the same gang are referred to as _earlier than_ or _previous to_ lanes
with larger indexes.  Conversely, lanes with larger indexes are _later than_
or _subsequent to_ those with smaller indexes.  In a vectorized loop,
iterations are assigned to lanes in increasing order so that earlier
iterations in the serial version of the loop map to earlier lanes in each
gang.

In all of the variants, there is neither a requirement nor an expectation that
individual lanes within a gang will make independent progress.  If any lane
within a gang blocks (e.g., on a mutex), then it is likely (but not required)
that all lanes will stop making progress.

Control-flow divergence and active lanes
----------------------------------------

Control flow _diverges_ between lanes when a _selection-statement_ (`if` or
`switch`), _iteration-statement_ (`while` or `for`), conditional operator
(`&&`, `||`, or `?:`), or polymorphic function call (virtual function call or
indirection through a function pointer) causes different lanes within a gang
to execute different code paths.  For each control-flow branch, those lanes
that should execute that branch are called _active_ lanes.  Control flow
_converges_ when the control paths come back together at the end of the
construct construct, e.g., at the end of an `if` statement.  Although
different variants have different ways of dealing with control-flow divergence,
it is generally true of all of the variants that large amounts of control-flow
divergence has a negative impact on performance.  If the control flow within a
loop has irreducible constructs (e.g., loops that have multiple entry points
via gotos), it may be impractical for the compiler to vectorize those
constructs at all.

Vector-enabled functions
------------------------

A vector-enabled function (also called a simd-enabled function) is a function
that that, when called within a vector loop, executes as if it were part of
the vector loop.  Multiple lanes can execute the same vector-enabled function
simultaneously, and all of the attributes of vector execution apply to the
body of the function.  In Cilk Plus and OpenMP, a vector-enabled function must
be specially declared (using `declspec(vector)` or `#pragma omp declare simd`,
respectively) and a scalar version of that function is implicitly generated
along with one or more vector versions.

In a SIMD implementation, a vector-enabled function called within a vector
loop is passed arguments and returns results via SIMD registers, where each
element of each SIMD register corresponds to a lane in the calling context.

Scalar regions
----------------

The language may provide a syntax to declare a segment of code within a vector
loop to be a _scalar_ region. This region has the following characteristics:

 1. Only one lane executes the scalar region at a time.

 2. It is considered a single, indivisible step, regardless of how many
    statements or expressions are within it; no portion of a scalar region on
    one lane is interleaved with any code on another lane.

An implicit scalar region surrounds each call to a non-vector-enabled function
within a vectorized loop.  (The as-if rule applies; if the compiler can
prove that a non-vector function can be vectorized without changing the
observable behavior, it is free to do so.)

Barriers
========

Overview
--------

A _barrier_ is a point of synchronization, adding specific
happens-before dependencies between steps executed on otherwise-independent
lanes in a gang.  Two types of barriers are described here: a _full barrier_
and a _wavefront barrier_.  A barrier need not correspond to a hardware
operation -- it is an abstraction that allows us to reason about what side
effects are known to have occurred, and what side effects are known to have
not yet occurred.  A compiler is not permitted to reorder instructions across
a barrier unless it can prove that such reordering will have no effect on the
results of the program.

Full Barriers
-------------

Upon encountering a _full barrier_, execution of an active lane blocks until
all other active lanes have reached the same barrier.  Put another way, all of
the active lanes within a gang synchronize such that they all have the same
program counter at the same time.

A step following a full barrier may depend on all lanes having completed all
steps preceding the barrier.  For VL greater than one, iterations of a vector
loop can therefore rely on results of computations that would have been
computed in "future" iterations if the loop were expressed serially.  This
feature is occasionally useful, especially for short-circuiting unneeded
computations, but it causes the computation to proceed differently (and
potentially produce different results) depending on the value of VL, and it
makes it difficult to reason about a program by comparing it to a serial
equivalent.

Wavefront Barriers
------------------

Upon encountering a _wavefront barrier_, execution of an active lane blocks
until all _previous_ active lanes have reached the same barrier. Intuitively,
no lane can get ahead of any previous lane.  A wavefront barrier is strictly
weaker than a full barrier, so a full barrier can always be inserted where a
wavefront barrier is needed without breaking the semantics of a program.

A step following a wavefront barrier may depend on all previous lanes having
completed all steps preceding the barrier.  Unlike the case of a full
barrier, a step cannot depend on values that would logically be computed in
the future.  Thus, a programmer could not use a wavefront to violate serial
semantics.


Lockstep Execution
==================

Overview
--------

Intuitively, each step in the lockstep execution variant occurs simultaneously
on all lanes within a gang.  More precisely, there is a logical full barrier
at the end of each step, with all of the lanes completing one step before any
lane starts the next step.  Each step is further subdivided into smaller
steps, recursively, with a full barrier at the end of each sub-step until we
come to a primitive step that cannot be subdivided.  If a local variable
(including any temporary variable) is needed by any step, then a separate
instance of that local variable is created for each lane.

The lockstep execution variant maps directly to modern SIMD microprocessors.
For code that cannot be translated directly into SIMD instructions the
compiler must ensure that the program behaves _as if_ every operation, down to
a specific level of granularity, were executed simultaneously on every lane.
In ISPC, the level of granularity is the code that executes between two
_sequence points_ as defined in the C standard.  Thus, for three consecutive
steps, A, B, C, that cannot be translated into SIMD instructions, the compiler
must generate A0, A1, A2, A3, B0, B1, B2, B3, C0, C1, C2, C3 (or use three
loops) in order to get correct execution on four lanes.  Of course, this can
often be optimized to something simpler.

Handling of control-flow divergence
-----------------------------------

When lockstep execution is mapped to SIMD hardware,
a _mask_ of lanes is computed for each possible branch of a selection
statement, conditional operator, or polymorphic function call. Each mask is
conceptually a bit mask with a one bit for each lane that should take a
particular branch and a zero bit for each lane that should not take that
branch.  The code in each branch is then executed with only those lanes having
ones in the corresponding mask participating in the execution.  Execution
proceeds with some lanes "masked off" until control flow converges again at
the end of the selection statement or conditional operator.  This process is
repeated recursively for conditional operations within each branch.  Each
conditional operation is a step and all branches within a conditional
operation comprise a single step.

In theory, the compiler would be free to schedule multiple branches
simultaneously.  In the following example, the optimizer might condense two
branches into a single hardware step by computing a vector of `+1` and `-1`
values and performing a single simd addition.

    for simd (int i = 0; i < N; ++) {
        if (c[i])
            a[i] += 1;
        else
            a[i] -= 1;
    }

In ISPC, however, the language specification specifically requires that the
_then_ branch of the `if` be executed for all applicable lanes before the
`else` branch is executed.  The optimization above is still valid because of
the "as-if" rule, but unless the ISPC compiler can prove the absence of a
cross-lane dependency in both branches it would be required to execute the
first branch before the second.

Processing of an _iteration-statement_ (`while`, `do` or `for`) is similar to
that of a _selection-statement_.  In this case, there is single mask
containing a one for each lane in which the loop condition is true (i.e., the
loop must iterate again).  The loop body is executed for each lane in this
set, then the mask is recomputed.  The loop terminates (and control flow
converges) when the set of lanes with a true loop condition is empty (i.e.,
the mask is all zeros).

Wavefront Execution
===================

Overview
--------

Unlike the lockstep variant, the steps in the wavefront execution variant can
proceed at different rates on different lanes.  However, although two lanes
may execute the same step *simultaneously*, a step on one lane cannot be
executed *before* the same step on an earlier lane.  Intuitively, then,
execution proceeds from top to bottom and left to right, producing a ragged
diagonal "wavefront".  More precisely, there is a logical wavefront barrier
between any two consecutive steps.

Typically, steps that can be executed using SIMD hardware are executed
simultaneously on all lanes.  Steps that do not map cleanly to SIMD hardware
are executed for one lane at a time.  Since there is not a full barrier at the
end of every step, the compiler can generate an arbitrarily-long series of
non-vector instructions for a single lane before generating the same
instructions (or using a loop) for the other lanes.  Thus, for three
consecutive steps, A, B, C, that cannot be translated into SIMD instructions,
the compiler has the option to generate A0, B0, C0, A1, B1, C1, A2, B2, C2,
A3, B3, C3, or use a single loop to maintain a wavefront.  The lockstep order
would also be valid.  Note that there is no need to define the minimum
granularity for a step, since any sized step may be executed serially without
disturbing the model.

Handling of control-flow divergence
-----------------------------------

For operations that rely on SIMD instructions, control-flow divergence in the
wavefront variant is handled the same way as in the lockstep variant.  A segment
of code that cannot be translated into SIMD instructions is serialized and
executed for each active lane within a branch.  By executing the segment for
earlier lanes before later lanes, the wavefront dependencies are automatically
preserved.  Thus, control-flow divergence is easier to handle in the wavefront
variant than in the lockstep variant, though it may be a bit more complicated to
describe.

Explicit-Barrier Execution
==========================

Overview
--------

This execution variant is the most like a parallel execution model.  All lanes
can progress at different rates with no requirement that earlier lanes stay
ahead of later lanes.  When an inter-lane dependency is needed, an explicit
barrier must be issued to cause the lanes to synchronize.

Because the lanes are not required to stay in sync, this variant is the
preferred variant for most GPUs, giving a group of hardware threads the
appearance of very wide vector units.  On a GPU, a full barrier is a fairly
cheap operation, although it does reduce parallelism by forcing some threads
to stall while waiting for other threads.

This variant is also useful on SIMD hardware.  Like the wavefront variant, the
compiler uses SIMD instructions where possible and processes one lane at a
time otherwise.  There are no hardware instructions corresponding to a
barrier, but a barrier in the code does restrict the order of execution of
steps as generated by the compiler.  For example, given three consecutive
steps, A, B, C, that cannot be translated into SIMD instructions, the compiler
can generate A0, B0, C0, A1, B1, C1, A2, B2, C2, A3, B3, C3, or use a single
loop.  However, if a full barrier is inserted between steps B and C, the
compiler would have to generate A0, B0, A1, B1, A2, B2, A3, B3, (barrier) C0,
C1, C2, C3 or use two loops (AB) and (C).  Moreover, the compiler is given
total freedom of optimization between barriers.


Handling of control-flow divergence
-----------------------------------

GPUs handle control-flow divergence in hardware.  Each thread acts
independently.  If control-flow divergence causes a pipeline stall, other
threads tend to smooth over the latency.

On SIMD hardware, control-flow divergence can be handled the same way as for
the wavefront variant.  The lack of implicit dependencies also allows the
compiler to re-order instructions in a way that is not possible with a pure
wavefront variant.


Varying, linear, and uniform
============================

An expression within a vector context can be described as _varying_, _linear_,
or _uniform_, as follows:

 * _varying_: The expression can have a different, unrelated,
   value in each lane.
 * _linear_: The value of the variable or
   expression is linear with respect to the lane index.  The difference in
   value between one lane and the next is the _stride_.  A common special case
   is a _unit stride_ -- i.e., a stride of one -- which is often more efficient
   for the hardware.
 * _uniform_: The value of the expression is the same for all lanes.

A variable or expression within a vector loop is assumed to be _varying_
unless the compiler can determine otherwise, either through data flow analysis
or through explicit attributes in the language. A variable declared outside of
the vector loop, for example, is implicitly uniform because there is only one
copy for the entire loop.  (The OpenMP `private` and `reduction` clauses
modify this attribute for specified variables.)

If the compiler can determine that that an expression is _linear_ (e.g.,
because it is declared such using a language attribute), it can improve
performance when that expression is used as an array index (integral values)
or an address (pointer values).  In most SIMD hardware, instructions that
access evenly-spaced memory addresses, especially if they are contiguous, are
less expensive than scatter/gather instructions on varying addresses.

If the compiler can determine that that an expression is _uniform_, it can
improve performance further by using scalar instructions instead of vector
instructions. For example, when the expression is an array index or an
address, a single memory lookup is needed instead of one per lane.  When the
uniform expression is a conditional expression, the compiler can avoid
generating code to handle control-flow divergence because all lanes will take
the same branch.

Writing to a variable with uniform address can cause correctness problems if
not all lanes write the same value or if one lane writes a value and another
lane reads it without an appropriate barrier in between.  For this reason, the
rules for writing to a uniform variable need to be carefully specified in any
vector language and should ideally be enforced by the compiler.


Lane-specific and cross-lane operations
=======================================

There are situations where the simd model is exposed very directly in the
code.  A short list of operations that might be useful are:

 * Lane index: Get the index for the current lane.
 * Number of Lanes: Get the number of lanes in this chunk.
 * Number of Active Lanes: Get the number of lanes in this chunk that are
   active within this branch of code.  (Implies a full barrier.)
 * Compress: Put the results of a computation from a non-contiguous set of
   active lanes into a contiguous sequence of memory locations.  (Implies a
   wavefront barrier.)
 * Expand: Read values from a contiguous sequence of memory locations for use
   in a non-contiguous set of active lanes.  (Implies a wavefront barrier.)
 * Prefix-sum (or other parallel prefix): The value of the expression in each
   lane is the value of the same expression in the previous active lane
   summed with a lane-specific value (or combined using some other
   associative operation).  (Implies a wavefront barrier.)
 * Chunk reduction: Combine values from multiple lanes into one uniform value.
   (Implies a full barrier.)
 * Shuffle: Re-order elements across lanes. (Implies a full barrier.)

Note that many of these operations, especially those that imply full barriers,
could be used in such a way as to break serial equivalence; i.e., cause the
result of a computation to differ depending on how many simd lanes exist.
Used carefully, however, these operations can produce deterministic results.


Comparing the Variants
======================

The lockstep variant might be the most intuitive for those who understand SIMD
hardware, but is arguably the least intuitive for those with experience with
more general forms of parallelism.  It is fairly easy to reason about, but the
divergence from serial semantics could cause scalability and maintainability
problems. The lockstep variant limits optimization options by the compiler and
there is concern that
that the lockstep variant is inefficient on some hardware.  GPUs, in particular,
are not well suited to lockstep execution.  This problem with GPUs is not a
reason in and of itself to reject lockstep execution as a valuable model but,
all things being equal, a model that works well with both CPUs and GPUs will
gain better support with the standards communities.

The wavefront variant is a bit more abstract than the lockstep variant and
retains 
serial semantics in more cases.  On the other hand, it has more of a feel of
auto-vectorization; someone looking for SIMD execution might be concerned
about what the compiler is actually generating.  (It can be argued that such
concerns are irrational, but they are real nonetheless.)  The wavefront variant
can be augmented with explicit full barriers (which do violate serial
semantics) in order to support certain cross-lane operations and get the best
of both worlds. With fewer dependency constraints, compiler has more
opportunities for optimization than it does for the lockstep variant. The
wavefront variant suffers most of the same implementation problems with GPUs
as the lockstep variant and, thus, evokes some standards committee
resistance.  In addition, it is difficult to implement the wavefront model in
LLVM (see [Robison14][]).

The explicit barrier variant is the most flexible variant for both the
programmer 
and the compiler, allowing the programmer to choose where dependencies are
needed and (if both full and wavefront barriers are provided) what type of
dependency is needed.  The compiler is free to optimize aggressively,
including reordering instructions between barriers.  This variant is the least
imperative of the variants, which may make some programmers wonder if
vectorization is actually happening (as in the case of the wavefront
variant). The explicit barrier variant maps well to both CPUs (which require no
hardware barrier instructions) and GPUs (which would use actual barrier
instructions).

Conclusion
==========

The model presented here is intended to directly express the concepts of
SIMD execution as implemented by x86, ARM, Power, and other CPUs and, to a
lesser degree, by some GPGPUs.  It is more powerful than the model that has
been presented to the C and C++ standards committee because it provides a
basis for cross-lane operations.

My hope is that by defining a model and choosing a variant, we can
use that model to drive the program and
implementation constraints rather than choosing the constraints and attempting
to use them to define a model.  The dependency constraints described in N3831,
for example, follow directly from the wavefront variant of the vector model
presented here.


Appendix: Matching Steps and Barriers Across Iterations
=======================================================

The definition of "same barrier" is simple for straight-line code: it can be
defined by the program counter or by a location in the source code.  In the
presence of control-flow divergence, especially loops, the definition becomes
more complicated. For example, given a barrier that is executed a different
number of times on different lanes based on a conditional expression, how does
one match a barrier encountered in one line to the "same barrier" encountered
in another lane and what can be said about synchronization of code between
occurrences of the barrier? Reasonably intuitive answers exist for these
questions, but defining it precisely can be complex. See the appendix for some
ways to tackle this problem. It is interesting to note that CUDA does not
allow barriers to be executed in the presence of control divergence; if any
lane (thread) executes a barrier, then all of the others must execute that
barrier or else deadlock results.

**[The description below is incomplete and might be replaced entirely]**

Although intuitively easy to grasp, linguistically we must devise a way of
numbering steps (including steps in multiple iterations of inner loops) such
that it is always easy to identify "corresponding" steps from separate
iterations of the outer vector loop.  For loop without branches, steps are
simply numbered consecutively.  At the start of an inner loop, we start
appending a sub-step number comprising the iteration number (starting from
zero) and a step number within the loop (also starting at zero). Two steps in
different lanes correspond to each other if they have the same step number.
One step precedes another step if its step number is lexicographically
smaller.  For example, the code below is shown with numbers assigned to each
step.  (Note that same numbers apply to each lane executing this code.):

    0           a += b;
    1           c *= a;
    2.i.0       for (int i = 0;
    2.i.1            i < c;
    2.i.3            ++i)
    2.i.2            x[b] += f(i);
    3           y[b] = g(x[b]);

With this numbering scheme, step 2.1.1 (first step in the second iteration of
the loop) precedes step 2.1.3 (third step in the same iteration) but comes
after 2.0.1 (first step in the first iteration).  The compiler is permitted to
interleave steps from different lanes anyway it wants, provided that the
wavefront respects these relationships.

References
==========

[N3831]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n3831.pdf
[N3831][]: _Language Extensions for Vector level parallelism_, Robert Geva and
Clark Nelson, 2014-01-14

[Robison14]: http://lists.cs.uiuc.edu/pipermail/llvmdev/2014-September/077290.html
[Robison14][]: _Proposal for ""llvm.mem.vectorize.safelen"_ (Discussion thread
on LLVMdev email list).

<!--

SNIPPETS

  Except for uniform variables, as
described later, modifying a non-local variable yields undefined behavior
(i.e., it is a data race).



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

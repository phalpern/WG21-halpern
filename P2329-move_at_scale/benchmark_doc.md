Notes on the benchmark implementation,
<!-- $TimeStamp$ --> 2021-12-06 17:49 EST <!-- $TimeStamp$ -->:

* `Element` type is vector of char using a scoped monotonic allocator.

* `Subsystem` is a struct containing a vector of elements (using the same
allocator)

* `Subsystem` is initialized with a subsystem size and element size
    * Each element is filled with a random char (all chars within an element
      are the same, but each element is different)
    * The subsystem size is the number of elements

* `Subsystem::access` reads each byte of every Element and modifies the first
  byte of each element, repeating `iterationCount` times.  This sequence
  simulates a common access pattern of iterating over arrays of elements,
  preforming mostly reads but a few writes.  The writes are strategically
  designed to prevent the compiler from optimizing away the read-only parts of
  the loop.

* The "system" is a vector of `vector<Subsystem>`.  The entire system uses the
  same allocator.

* The `churn` function shuffles elements of the same index between subsystems,
  using either move-assignment or copy-assignment.  Because the elements are
  all of the same size, copy assignment works by copying characters, with no
  allocations or deallocations.  Every element of every subsystem participates
  in the shuffle.  The process is repeated `churnCount` times, but, because the
  churn is not what we're measuring and because the shuffle uses a high-quality
  RNG, there should be no need to churn more than once.

* The number of subsystems in the system, number of elements in each subsystem,
  and number of bytes in each element are passed as parameters that are
  interpreted as exponents of powers of 2.  Thus, if the subsystem-size
  parameter is 10, then each subsystem contains 1024 elements.

* `main` reads command-line parameters, initializes the system, churns it, then
  calls `access` on each subsystem `iterationCount` times.

   * The allocator used is a sequential pool allocator and the order of
     construction is such that, prior to churning, all elements within each
     subsystem would be contiguous in memory (or have few discontinuities)

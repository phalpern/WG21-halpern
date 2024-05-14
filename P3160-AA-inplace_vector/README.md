Fixed-sized and Small Vectors
=============================

This directory contains a partial implementation of
[P0843 `inplace_vector`](http://wg21.link/P0843) and
[P2667 *Support for static and SBO vectors by allocators*](http://wg21.link/P2667).
In particular, allocator support is explored.

Contents
--------

* `P3160.md` -- Paper proposing allocator support for `inplace_vector`
* `P3160R0.html` -- R0 published version of P3160.md
* `compile-time_test.py` -- Script for testing effects of allocator support on
  compile time of `inplace_vector`.
* `inplace_vector.h` -- Partial implementation of `inplace_vector` with
  conditional allocator support.
* `inplace_vector.t.cpp` -- Test program for `inplace_vector`, designed to test
  the effect of allocators on compile time.
* `sbo_and_static_vector.h` -- Interface and implementation of P2667
* `sbo_and_static_vector.t.cpp` -- Incomplete test driver for P2667

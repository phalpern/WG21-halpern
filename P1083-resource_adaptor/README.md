Sample implementations of P1083
===============================

This directory contains:

* An definition of `std::max_align_v` (in `aligned_type.h`)
* An implementation of `std::aligned_object_storage` (in `aligned_type.h`)
* An implementation of `std::aligned_type` (in `aligned_type.h`)
* Three different implementations of `std::pmr::resource_adaptor`:
   1. One that uses linear search to find the correct rebound allocator type
      for the runtime alignment value (in `resource_adaptor_linear.h`)
   2. One that uses binary search to find the correct rebound allocator type
      for the runtime alignment value (in `resource_adaptor_binsearch.h`)
   3. One that find the log2 of the runtime alignment value, then uses a
      (constant-time) switch statement to find the correct rebound allocator
      type (in `resource_adaptor_switch.h`)
* The text of P1083 in markdown format (`P1083_resource_adaptor_to_WP.md`)

All new features have fairly complete test drivers (in `aligned_type.t.cpp`,
`resource_adaptor_*.t.cpp`, and `resource_adaptor.t.h`).  Typing `make` will
build and run the test drivers and will also produce optimized and demangled
assembly files for visual comparison of the different algorithms. The generated
files are put into the `obj` subdirectory.

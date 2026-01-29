---
title: "Document title"
document: DxxxxR0
# $TimeStamp:
date: 2026-01-12 18:27 EST
# $
audience: LEWGI
author:
  - name: Pablo Halpern
    email: <phalpern@halpernwightsoftware.com>
working-draft: Nxxxx
toc: true
toc-depth: 2
---

Abstract
========

I discovered, upon rereading [@P3390R0], the Sean Baxter described more
practical ways of integrating new (safe) C++ with existing C++, despite still
having a long way to go.

Change Log
==========


Straw-man Syntax for Rust-style Borrow Checking
===============================================

For the purpose of this paper, I will assume a superset of C++26 having
features borrowed from Circle, as described in [@P3390R0].  However, instead of
defining a completely new syntax for borrow-checked references, the rest of
this paper will assume the following syntax:

Safe-function Annotation
------------------------

Borrow-checked References
-------------------------


Owning References and Drop Semantics
====================================


New Features Can Make Old Code Safer
====================================

`string_view` example



New Safe Library (and Builtin?) types
=====================================

The Many Meanings of a Data Pointer
-----------------------------------

* Pointer to uninitialized storage
* Pointer to static, thread_local, dynamic, or auto object
* Pointer to array (via decay)
* Pointer to an element of an array
* Pointer to a member of a class or union

A raw pointer does not tell you what is valid:

* Can you construct an object at the pointed-to location?
* Can you destroy an object through the pointer?
* Can you deallocate it?
* Can you access an object at p + 1 or p - 1?
* Can you safely cast away const?
* Can you compare it to another pointer?

Owning Pointers
---------------

Borrow-checked Non-owning Pointers
----------------------------------

Bounds-checking Pointers (Slices, Iterators)
--------------------------------------------

Raw Pointers
------------

Either disallowed in safe code, or given a default semantic (e.g., non-owning,
non-array-element).


Add Safe Interfaces to Existing Library Types
=============================================

Ownership-safe Iterators

Bounds-checking Overloads for Safe Pointers

---
references:
  - id: P3874R1
    citation-label: P3874R1
    author: Jon Bauman et al.
    title: "Safety Strategy Requirements for C++"
    URL: TBD
---

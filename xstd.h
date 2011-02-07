/* xstd.h                  -*-C++-*-
 *
 * Copyright (C) 2009 Halpern-Wight Software, Inc. All rights reserved.
 */

#ifndef INCLUDED_XSTD_DOT_H
#define INCLUDED_XSTD_DOT_H

#include <utility>
#include <type_traits>

#define XSTD xstd
#define BEGIN_NAMESPACE_XSTD namespace XSTD {
#define END_NAMESPACE_XSTD   }
#define USING_NAMESPACE_XSTD using namespace XSTD
#define USING_FROM_NAMESPACE_XSTD(sym) using XSTD::sym

// Define namesapsce
BEGIN_NAMESPACE_XSTD

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_XSTD_DOT_H)

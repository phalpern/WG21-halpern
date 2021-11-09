/* xstd.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

// Define an "extended std" namespace for proposed new library feature.

#ifndef INCLUDED_XSTD_DOT_H
#define INCLUDED_XSTD_DOT_H

#define XSTD xstd
#define BEGIN_NAMESPACE_XSTD namespace XSTD {
#define END_NAMESPACE_XSTD   }
#define USING_NAMESPACE_XSTD using namespace XSTD
#define USING_FROM_NAMESPACE_XSTD(sym) using XSTD::sym

#define XPMR XSTD::pmr
#define BEGIN_NAMESPACE_XPMR namespace XPMR {
#define END_NAMESPACE_XPMR   }

namespace std::pmr { }

// Define namesapsce
BEGIN_NAMESPACE_XSTD

using namespace std;

END_NAMESPACE_XSTD

BEGIN_NAMESPACE_XPMR

using namespace std::pmr;

END_NAMESPACE_XPMR

#endif // ! defined(INCLUDED_XSTD_DOT_H)

/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_X86_64_H
#define BOOST_PREDEF_ARCHITECTURE_X86_64_H

#include <futures/detail/bundled/boost/predef/version_number.h>
#include <futures/detail/bundled/boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_X86_64`

http://en.wikipedia.org/wiki/Ia64[Intel IA-64] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__x86_64+` | {predef_detection}
| `+__x86_64__+` | {predef_detection}
| `+__amd64__+` | {predef_detection}
| `+__amd64+` | {predef_detection}
| `+_M_X64+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_ARCH_X86_64 BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__amd64__) || defined(__amd64) || \
    defined(_M_X64)
#   undef BOOST_ARCH_X86_64
#   define BOOST_ARCH_X86_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_X86_64
#   define BOOST_ARCH_X86_64_AVAILABLE
#endif

#define BOOST_ARCH_X86_64_NAME "Intel x86-64"

#include <futures/detail/bundled/boost/predef/architecture/x86.h>

#endif

#include <futures/detail/bundled/boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_X86_64,BOOST_ARCH_X86_64_NAME)
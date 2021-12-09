// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#define FUTURES_RANGES_DETAIL_PROLOGUE_HPP
#include <futures/algorithm/detail/traits/range/detail/config.h>
#endif

#ifdef RANGES_PROLOGUE_INCLUDED
#error "Prologue already included!"
#endif
#define RANGES_PROLOGUE_INCLUDED

RANGES_DIAGNOSTIC_PUSH

#ifdef RANGES_FEWER_WARNINGS
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_INTERNAL
RANGES_DIAGNOSTIC_IGNORE_INDENTATION
RANGES_DIAGNOSTIC_IGNORE_CXX17_COMPAT
#endif

RANGES_DIAGNOSTIC_KEYWORD_MACRO

#define template(...)                                                                                                  \
    CPP_PP_IGNORE_CXX2A_COMPAT_BEGIN                                                                                   \
    template <__VA_ARGS__ CPP_TEMPLATE_AUX_

#define AND CPP_and

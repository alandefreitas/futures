//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_DETAIL_EXECUTION_HPP
#define FUTURES_ALGORITHM_DETAIL_EXECUTION_HPP

/*
 * Include <execution> only if we can be sure it's available
 */

#include <futures/config.hpp>
#include <futures/detail/deps/boost/config/workaround.hpp>

#ifdef __has_include
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

#ifdef __cpp_lib_execution
#    if defined(BOOST_GCC)
#        if BOOST_WORKAROUND(BOOST_GCC_VERSION, >= 100000)
#            include <execution>
#            define FUTURES_HAS_STD_POLICIES
#        endif
#    elif !defined(BOOST_MSVC)
#        include <execution>
#        define FUTURES_HAS_STD_POLICIES
#    endif
#endif

#endif // FUTURES_ALGORITHM_DETAIL_EXECUTION_HPP
